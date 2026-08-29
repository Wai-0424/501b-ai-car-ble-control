# 501b-ai-car-ble-control

AI 自走車 BLE 藍牙遙控系統 — Android App + BMduino-UNO 韌體。

手機 App 透過 BLE 送出雙搖桿指令（油門／轉向），BMduino-UNO 韌體解碼後驅動轉向伺服與直流馬達，用來驗證「手機 → 藍牙 → 韌體 → 機構動作」這條控制鏈路，作為之後加裝感測器、走向自動駕駛的基礎平台。

完整架構說明、通訊協定、校正細節請看 [docs/AI自走車_BLE遙控系統技術報告.docx](docs/AI自走車_BLE遙控系統技術報告.docx)（教學向報告，含系統架構圖）。

## 資料夾結構

```
501b-ai-car-ble-control/
├─ app/                          Android App（Kotlin + Jetpack Compose）
├─ firmware/
│  ├─ car_control/               正式韌體（唯一會燒錄到車上的程式）
│  ├─ tests/                     校正／除錯用草稿（i2c_scan、motor_*、servo_* 等）
│  └─ build_out/                 arduino-cli 編譯輸出（.gitignore 排除，不進版本控制）
├─ docs/
│  ├─ CALIBRATION.md             硬體測試/校正紀錄
│  ├─ datasheets/BMC77M001/      BLE 模組原廠手冊（BestModules）
│  └─ AI自走車_BLE遙控系統技術報告.docx
└─ .gitignore
```

## 硬體

- 主控板：BMduino-UNO（Holtek HT32F52367）
- BLE 模組：BM7701-00-1 / BMC77M001（BestModules，UART 介面）
- PWM 驅動：PCA9685（I2C 0x40）— CH0 轉向伺服、CH1/CH2 控制 XY-160D 馬達驅動板方向
- 轉速控制：BMduino D5 (PWM) → 74HC14 位準轉換 → XY-160D ENA

目前的伺服校正基準（機械中心 / 可轉範圍）以 [docs/CALIBRATION.md](docs/CALIBRATION.md) 為準，機構若重新拆裝需要回去更新。

## 燒錄方式

```bash
arduino-cli compile --fqbn HT32:HT32:BM53A367A firmware/car_control
arduino-cli upload -p COMx --fqbn HT32:HT32:BM53A367A firmware/car_control
```

板上內建 e-Link32 Lite（USB），不需要外接 SWD 探棒；FQBN 固定用 `HT32:HT32:BM53A367A`，不是 `_IAP` 變體。詳見 [docs/CALIBRATION.md](docs/CALIBRATION.md)。

## App

`app/` 是標準 Android Studio 專案（minSdk 24），用 Android 原生 BLE API 掃描裝置名稱 `BMC_CAR` 並連線，畫面提供雙軸搖桿即時控制。用 Android Studio 開啟 `app/` 資料夾即可建置。

## License

[MIT](LICENSE)
