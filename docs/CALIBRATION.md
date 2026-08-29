# 硬體測試/校正紀錄

## 版本基準點（2026-08-25）
App（`app/`）與韌體（`firmware/car_control/car_control.ino`）已完整測過一輪（I2C、伺服極限、馬達方向/調速、BLE 配對、搖桿即時控制），暫定為目前的基準版本，用 git commit + tag `v1.0-baseline` 存檔。接下來要動前輪轉向機構（連桿/舵臂角度），如果調整後發現實際轉向死點跟現在記錄的 130°±30° 不一樣，屬於正常現象，回來更新下面「轉向伺服」那段的 `CENTER`／`STEER_OFFSET` 並同步改 `car_control.ino` 就好，不用懷疑是程式退步。

## I2C
- PCA9685 實際接在主排針的 I2C0（D18/A4=SDA, D19/A5=SCL），程式用 `Wire`，不是 `Wire1`。
- 位址掃描結果：0x40（PCA9685 本體）、0x70（All Call，正常）。

## 燒錄方式
- FQBN: `HT32:HT32:BM53A367A`（不是 `_IAP` 那個變體）
- 走板上內建 e-Link32 Lite（USB），不用外接 SWD 探棒。
- `arduino-cli compile --fqbn HT32:HT32:BM53A367A <sketch>`
- `arduino-cli upload -p COMx --fqbn HT32:HT32:BM53A367A <sketch>`（COM 埠依裝置管理員實際顯示為準）

## 馬達方向控制（PCA9685 → XY-160D）
- **實際通道分配：CH0=伺服、CH1=IN1、CH2=IN2**（不是之前規劃/程式裡誤用的 CH2/CH3，2026-08-20 已修正所有測試草稿）。
- ENA 仍是 BMduino D5 → 74HC14 → XY-160D ENA，跟 PCA9685 無關。

## 轉向伺服（PCA9685 CH0）
- **2026-08-25 重新校正**：舵臂拆下來重裝，機械中心改成 **90°**（原本 130° 是舊舵臂角度下量出來的，已作廢，不要再用）。
- 目前脈寬對應：SERVO_MIN=205(~1000us)=0°、SERVO_MAX=410(~2000us)=180°，50Hz。
- 死點測試（新舵臂角度下）：以 90° 為中心，正負 70°（20°~160°）都測過沒有卡死/異音，`STEER_OFFSET` 已定案為 **70**，`car_control.ino` 同步更新。

> 之後整合韌體／KiCad 文件裡的伺服角度，都要用「90° = 直行、±70° 轉向範圍」這個基準。
