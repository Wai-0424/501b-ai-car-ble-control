package com.bestmodules.ai_bluetooth_car_test

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import com.bestmodules.ai_bluetooth_car_test.ble.BleConnectionState
import com.bestmodules.ai_bluetooth_car_test.ble.CarBleController
import com.bestmodules.ai_bluetooth_car_test.ble.TARGET_DEVICE_NAME
import com.bestmodules.ai_bluetooth_car_test.ui.theme.Ai_bluetooth_car_testTheme
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlin.math.roundToInt

class MainActivity : ComponentActivity() {

    private val bleController by lazy { CarBleController(applicationContext) }

    private val requiredPermissions: Array<String>
        get() = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            Ai_bluetooth_car_testTheme {
                val permissionLauncher = rememberLauncherForActivityResult(
                    ActivityResultContracts.RequestMultiplePermissions()
                ) { results ->
                    if (results.values.all { it }) {
                        bleController.startScanAndConnect()
                    }
                }

                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    CarControlScreen(
                        modifier = Modifier.padding(innerPadding),
                        bleController = bleController,
                        onConnectClick = {
                            if (!bleController.isBluetoothEnabled()) {
                                startActivity(
                                    android.content.Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
                                )
                                return@CarControlScreen
                            }
                            permissionLauncher.launch(requiredPermissions)
                        },
                        onDisconnectClick = { bleController.disconnect() }
                    )
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        bleController.disconnect()
    }
}

@Composable
fun CarControlScreen(
    modifier: Modifier = Modifier,
    bleController: CarBleController,
    onConnectClick: () -> Unit,
    onDisconnectClick: () -> Unit
) {
    val state by bleController.state.collectAsState()
    val log by bleController.log.collectAsState()
    var freeText by remember { mutableStateOf("") }
    var throttle by remember { mutableFloatStateOf(0f) }
    var steer by remember { mutableFloatStateOf(0f) }

    LaunchedEffect(state) {
        if (state != BleConnectionState.READY) return@LaunchedEffect
        while (isActive) {
            bleController.sendMove(throttle, steer)
            delay(50)
        }
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = "藍牙遙控車",
            style = MaterialTheme.typography.headlineSmall
        )
        Spacer(Modifier.height(8.dp))
        Text(text = "目標裝置：$TARGET_DEVICE_NAME")
        Text(text = "狀態：${state.toDisplayText()}")

        Spacer(Modifier.height(12.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            Button(onClick = onConnectClick, enabled = state == BleConnectionState.DISCONNECTED) {
                Text("連線")
            }
            Button(
                onClick = onDisconnectClick,
                enabled = state != BleConnectionState.DISCONNECTED,
                colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.error)
            ) {
                Text("斷線")
            }
        }

        Spacer(Modifier.height(24.dp))
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceEvenly
        ) {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                AxisJoystick(
                    orientation = JoystickOrientation.VERTICAL,
                    enabled = state == BleConnectionState.READY,
                    onValueChange = { throttle = it }
                )
                Spacer(Modifier.height(4.dp))
                Text(text = "油門", style = MaterialTheme.typography.bodySmall)
            }
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                AxisJoystick(
                    orientation = JoystickOrientation.HORIZONTAL,
                    enabled = state == BleConnectionState.READY,
                    onValueChange = { steer = it }
                )
                Spacer(Modifier.height(4.dp))
                Text(text = "轉向", style = MaterialTheme.typography.bodySmall)
            }
        }

        Spacer(Modifier.height(24.dp))
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            OutlinedTextField(
                value = freeText,
                onValueChange = { freeText = it },
                modifier = Modifier.weight(1f),
                label = { Text("自訂指令") }
            )
            Spacer(Modifier.width(8.dp))
            Button(
                onClick = {
                    if (freeText.isNotEmpty()) {
                        bleController.sendCommand(freeText)
                        freeText = ""
                    }
                },
                enabled = state == BleConnectionState.READY
            ) {
                Text("送出")
            }
        }

        Spacer(Modifier.height(16.dp))
        Text(text = "紀錄", style = MaterialTheme.typography.titleMedium)
        Surface(
            modifier = Modifier
                .fillMaxWidth()
                .heightIn(min = 120.dp),
            color = MaterialTheme.colorScheme.surfaceVariant,
            shape = RoundedCornerShape(8.dp)
        ) {
            LazyColumn(modifier = Modifier.padding(8.dp)) {
                items(log) { line ->
                    Text(text = line, style = MaterialTheme.typography.bodySmall)
                }
            }
        }
    }
}

private fun BleConnectionState.toDisplayText(): String = when (this) {
    BleConnectionState.DISCONNECTED -> "未連線"
    BleConnectionState.SCANNING -> "掃描中..."
    BleConnectionState.CONNECTING -> "連線中..."
    BleConnectionState.DISCOVERING -> "搜尋服務中..."
    BleConnectionState.READY -> "已連線"
}

enum class JoystickOrientation { VERTICAL, HORIZONTAL }

private const val JOYSTICK_TRACK_LENGTH_DP = 160
private const val JOYSTICK_KNOB_SIZE_DP = 48
private const val JOYSTICK_SNAP_BACK_MS = 150

/**
 * Single-axis joystick: the knob can only move along [orientation]'s axis, matching the car's
 * hardware which has exactly one throttle degree of freedom and one steering degree of freedom
 * (single drive motor + single steering servo, no per-wheel differential). Reports the knob's
 * position as -1f..1f via [onValueChange] on every drag update, and snaps back to 0f (with an
 * animation) on release or when [enabled] turns false.
 */
@Composable
fun AxisJoystick(
    orientation: JoystickOrientation,
    enabled: Boolean,
    trackLength: Dp = JOYSTICK_TRACK_LENGTH_DP.dp,
    knobSize: Dp = JOYSTICK_KNOB_SIZE_DP.dp,
    onValueChange: (Float) -> Unit
) {
    val density = LocalDensity.current
    val maxOffsetPx = remember(trackLength, knobSize) {
        with(density) { (trackLength.toPx() - knobSize.toPx()) / 2f }
    }
    val offset = remember { Animatable(0f) }
    val scope = rememberCoroutineScope()

    fun normalize(offsetPx: Float): Float {
        val raw = offsetPx / maxOffsetPx
        return if (orientation == JoystickOrientation.VERTICAL) -raw else raw
    }

    LaunchedEffect(enabled) {
        if (!enabled) {
            offset.animateTo(0f, animationSpec = tween(JOYSTICK_SNAP_BACK_MS))
            onValueChange(0f)
        }
    }

    val trackModifier = if (orientation == JoystickOrientation.VERTICAL) {
        Modifier.width(knobSize).height(trackLength)
    } else {
        Modifier.width(trackLength).height(knobSize)
    }

    Box(
        modifier = trackModifier
            .background(MaterialTheme.colorScheme.surfaceVariant, RoundedCornerShape(50)),
        contentAlignment = Alignment.Center
    ) {
        Box(
            modifier = Modifier
                .size(knobSize)
                .offset {
                    if (orientation == JoystickOrientation.VERTICAL) {
                        IntOffset(0, offset.value.roundToInt())
                    } else {
                        IntOffset(offset.value.roundToInt(), 0)
                    }
                }
                .background(
                    color = if (enabled) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.outline,
                    shape = CircleShape
                )
                .pointerInput(enabled, orientation) {
                    if (!enabled) return@pointerInput
                    detectDragGestures(
                        onDrag = { change, dragAmount ->
                            change.consume()
                            val delta = if (orientation == JoystickOrientation.VERTICAL) dragAmount.y else dragAmount.x
                            val newOffsetPx = (offset.value + delta).coerceIn(-maxOffsetPx, maxOffsetPx)
                            scope.launch { offset.snapTo(newOffsetPx) }
                            onValueChange(normalize(newOffsetPx))
                        },
                        onDragEnd = {
                            scope.launch { offset.animateTo(0f, animationSpec = tween(JOYSTICK_SNAP_BACK_MS)) }
                            onValueChange(0f)
                        },
                        onDragCancel = {
                            scope.launch { offset.animateTo(0f, animationSpec = tween(JOYSTICK_SNAP_BACK_MS)) }
                            onValueChange(0f)
                        }
                    )
                }
        )
    }
}
