# Spresense-Playground
Let's Play Spresense!

Spresenseを使った遊び場です。 + 
SPRESENSEで使える各種ライブラリ、サンプルコードを開発・提供していきます。

本線リポジトリに取り込まれたものは、削除していきます。

--------------------
## What's new.

12/5 
・楽器に向けての @gokankenichi さん作AutoTuneをライブラリ化しました。

※ライセンスちゃんと書きましょうか…。


11/25 
・高速レンダリングのサンプル追加。

```
Spresense-Playground
 |
 ├ sketchies : Arduinoスケッチを置く場所。
     |
     ├ AudioRendering
```

・楽器プロジェクトのサンプル追加。

```
Spresense-Playground
 |
 ├ sketchies : Arduinoスケッチを置く場所。
     |
     ├ Instruments
```


・不要になった以下のライブラリ・サンプルを削除。

```
Spresense-Playground
 |
 ├ libraries : Arduinoライブラリを置く場所。
 |    |
 |    ├ VoiceChanger（SignalProcessingに移行済み）
 | 
 ├ sketchies : Arduinoスケッチを置く場所。
     |
     ├ wf931 (本家立ち上げにつき)
```


wf931は、以下。
https://github.com/SMK-RD/WF931-Sigfox-module


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

### センサドライバ関連
* BSEC（BME680）

### 信号処理関連
* SignalProcessing(FFT/IIR)

### 音声処理関連
* AutoTune
* MIDI

---
## Arduino スケッチリスト
- AudioPlayers
    - audio_with_sensing
    - diy_player
    - diy_player_wo_sensing
    - NetRadio_w_sensing

- AudioRendering
    - rendering_objif

- Instruments
    - SmartCajon
    - YuruHorn

- USB_UART
- I2cScanner
- LowPowerSensing
    - bme680_ulp_plus_via_sigfox
    - bme680_uulp_plus_via_sigfox


