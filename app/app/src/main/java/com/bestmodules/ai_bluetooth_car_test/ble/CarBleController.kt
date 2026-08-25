package com.bestmodules.ai_bluetooth_car_test.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import java.util.UUID

/**
 * Talks to the BMduino car over the BM7701-00-1 (BMC77M001) BLE module.
 *
 * GATT layout, taken from BestModules' BLE_API_User_s_Guide.pdf:
 *   Service           0xFFF0
 *   Notify (RX, module -> phone)         0xFFF1
 *   Write-without-response (TX, phone -> module) 0xFFF2
 *
 * Caller (MainActivity) is responsible for checking/requesting the
 * BLUETOOTH_SCAN / BLUETOOTH_CONNECT (API 31+) or ACCESS_FINE_LOCATION
 * (API <31) runtime permissions before calling startScanAndConnect().
 */
const val TARGET_DEVICE_NAME = "BMC_CAR"

private val SERVICE_UUID: UUID = UUID.fromString("0000FFF0-0000-1000-8000-00805F9B34FB")
private val NOTIFY_CHAR_UUID: UUID = UUID.fromString("0000FFF1-0000-1000-8000-00805F9B34FB")
private val WRITE_CHAR_UUID: UUID = UUID.fromString("0000FFF2-0000-1000-8000-00805F9B34FB")
private val CCCD_UUID: UUID = UUID.fromString("00002902-0000-1000-8000-00805F9B34FB")

private const val SCAN_TIMEOUT_MS = 15_000L
private const val TAG = "CarBleController"

enum class BleConnectionState { DISCONNECTED, SCANNING, CONNECTING, DISCOVERING, READY }

@Suppress("DEPRECATION") // targeting minSdk 24, the pre-API33 GATT methods are used uniformly on purpose
@SuppressLint("MissingPermission")
class CarBleController(private val context: Context) {

    private val bluetoothManager =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val adapter: BluetoothAdapter? get() = bluetoothManager.adapter

    private val mainHandler = Handler(Looper.getMainLooper())

    private var gatt: BluetoothGatt? = null
    private var writeChar: BluetoothGattCharacteristic? = null

    private val _state = MutableStateFlow(BleConnectionState.DISCONNECTED)
    val state: StateFlow<BleConnectionState> = _state

    private val _lastReceived = MutableStateFlow("")
    val lastReceived: StateFlow<String> = _lastReceived

    private val _log = MutableStateFlow<List<String>>(emptyList())
    val log: StateFlow<List<String>> = _log

    private fun appendLog(msg: String) {
        Log.d(TAG, msg)
        _log.value = (_log.value + msg).takeLast(80)
    }

    fun isBluetoothEnabled(): Boolean = adapter?.isEnabled == true

    fun startScanAndConnect() {
        val bleAdapter = adapter
        if (bleAdapter == null || !bleAdapter.isEnabled) {
            appendLog("藍牙未開啟")
            return
        }
        if (_state.value != BleConnectionState.DISCONNECTED) {
            appendLog("已經在連線/掃描中")
            return
        }
        val scanner = bleAdapter.bluetoothLeScanner
        if (scanner == null) {
            appendLog("此裝置沒有 BLE 掃描器")
            return
        }
        _state.value = BleConnectionState.SCANNING
        appendLog("開始掃描 $TARGET_DEVICE_NAME ...")
        scanner.startScan(scanCallback)

        mainHandler.postDelayed({
            if (_state.value == BleConnectionState.SCANNING) {
                scanner.stopScan(scanCallback)
                _state.value = BleConnectionState.DISCONNECTED
                appendLog("掃描逾時，找不到裝置")
            }
        }, SCAN_TIMEOUT_MS)
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val name = result.device.name ?: result.scanRecord?.deviceName ?: return
            if (name != TARGET_DEVICE_NAME) return

            adapter?.bluetoothLeScanner?.stopScan(this)
            appendLog("找到 $TARGET_DEVICE_NAME，連線中...")
            _state.value = BleConnectionState.CONNECTING
            gatt = result.device.connectGatt(context, false, gattCallback)
        }

        override fun onScanFailed(errorCode: Int) {
            appendLog("掃描失敗，錯誤碼 $errorCode")
            _state.value = BleConnectionState.DISCONNECTED
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    appendLog("已連線，開始搜尋服務...")
                    _state.value = BleConnectionState.DISCOVERING
                    g.discoverServices()
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    appendLog("已斷線")
                    _state.value = BleConnectionState.DISCONNECTED
                    writeChar = null
                    g.close()
                    gatt = null
                }
            }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                appendLog("服務搜尋失敗，狀態碼 $status")
                return
            }
            val service = g.getService(SERVICE_UUID)
            if (service == null) {
                appendLog("找不到 FFF0 服務")
                return
            }
            val notifyChar = service.getCharacteristic(NOTIFY_CHAR_UUID)
            val wChar = service.getCharacteristic(WRITE_CHAR_UUID)
            if (notifyChar == null || wChar == null) {
                appendLog("找不到 FFF1/FFF2 特徵值")
                return
            }
            writeChar = wChar

            g.setCharacteristicNotification(notifyChar, true)
            val cccd = notifyChar.getDescriptor(CCCD_UUID)
            if (cccd != null) {
                cccd.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                g.writeDescriptor(cccd)
            }

            _state.value = BleConnectionState.READY
            appendLog("準備完成，可以送指令了")
        }

        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            val text = characteristic.value?.toString(Charsets.UTF_8) ?: return
            _lastReceived.value = text
            appendLog("收到: $text")
        }
    }

    fun sendCommand(text: String) {
        writeBytes(text.toByteArray(Charsets.UTF_8), "送出: $text")
    }

    /**
     * Packs the dual-joystick state into the 3-byte move packet ['M', throttleByte, steerByte]
     * (see car_control.ino's handleMovePacket for the decode side) and sends it.
     * [throttleNorm]/[steerNorm] are normalized joystick positions in -1f..1f; 0f maps to the
     * neutral byte 128 (stopped / centered).
     */
    fun sendMove(throttleNorm: Float, steerNorm: Float) {
        val throttleByte = normToByte(throttleNorm)
        val steerByte = normToByte(steerNorm)
        writeBytes(
            byteArrayOf('M'.code.toByte(), throttleByte.toByte(), steerByte.toByte()),
            "移動: throttle=$throttleByte steer=$steerByte"
        )
    }

    private fun normToByte(v: Float): Int =
        (128 + v.coerceIn(-1f, 1f) * 127).let { Math.round(it) }.coerceIn(0, 255)

    private fun writeBytes(bytes: ByteArray, logMsg: String) {
        val g = gatt
        val c = writeChar
        if (g == null || c == null) {
            appendLog("尚未連線，無法送出")
            return
        }
        c.value = bytes
        c.writeType = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        g.writeCharacteristic(c)
        appendLog(logMsg)
    }

    fun disconnect() {
        adapter?.bluetoothLeScanner?.stopScan(scanCallback)
        gatt?.disconnect()
        gatt?.close()
        gatt = null
        writeChar = null
        _state.value = BleConnectionState.DISCONNECTED
    }
}
