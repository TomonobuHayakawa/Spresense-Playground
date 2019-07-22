# Spresense-Playground
Let's Play Spresense!

Spresenseを使った遊び場です。 + 
SPRESENSEで使える各種ライブラリ、サンプルコードを開発・提供していきます。

本線リポジトリに取り込まれたものは、削除していきます。

--------------------
## ディレクトリ構成

```
Spresense-Playground
 |
 ├ examples : SDK上のサンプルコードを置く場所。
 |
 ├ libraries : Arduinoライブラリを置く場所。
 |
 ├ sketchies : Arduinoスケッチを置く場所。
```

---
## Arduino ライブラリリスト


### 信号処理関連
* BSEC（BME680）
* SignalProcessing(FFT/IIR)
* VoiceChanger（SignalProcessingに移行後、廃棄）

### 音声処理関連
* MIDI


---
## Arduino スケッチリスト
- AudioPlayers
    - audio_with_sensing
    - diy_player
    - diy_player_wo_sensing
    - NetRadio_w_sensing

- USB_UART
- I2cScanner
- LowPowerSensing
    - bme680_ulp_plus_via_sigfox
    - bme680_uulp_plus_via_sigfox
- wf931

