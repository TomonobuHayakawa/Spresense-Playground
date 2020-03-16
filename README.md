# Spresense-Playground
Let's Play Spresense!

Spresenseを使った遊び場です。 + 
SPRESENSEで使える各種ライブラリ、サンプルコードを開発・提供していきます。

本線リポジトリに取り込まれたものは、削除していきます。

--------------------
## What's new.

3/16

・FFTライブラリの修正。

FFTライブラリのFFTのTap数、最大のチャンネル数が固定でしか運用できなかったため、
非常に使いづらかったです。
ですが、同時に、CMSIS-DSPのライブラリがTAP数ごとにデータを持つため、
すべてのTAP数をリンクするとサイズが大きくなってしまう問題がありました。
そのため、今回の実装で、templateで実装し回避しました。
※苦肉の策…。

不評であれば、再度考えます。


3/8

・M11SのコードのUpdate。連続稼働時のバグ修正。

2/10

・M11S向けのスケッチを追加。

  *sketches/M11S*

の下に、SD書き込みサンプルとWiFiカメラのサンプルがあります。

M11S向けには、M11S向けのパッケージを利用する必要があります。
※違いは、SPI5のデータ転送をDMAを使って行う部分です。

Arduinoのファイル / 環境設定 の中の <br>
追加ボードマネージャのURLに、 <br>
https://github.com/TomonobuHayakawa/spresense-arduino-compatible/releases/download/common/package_spresense_M11S_index.json
 <br>
を追加してください。  

これを追加すると、ボードマネージャの中に、  

*Spresense M11S*  

というボードが新たに選択できます。  

M11Sを使う場合は、これを利用してください。

1/28

・スマートドラムを小川君に代わって入れておきました。

12/27

・シンセサイザ機能の更新
   - オシレータにエンベロープジェネレータを追加しました。
   - 最大8chの波形を出力できるようにpost処理のMixerを拡張しました。

12/18

・ゆる楽器に向けてSpresenseのSDKのαリリースをします。
   - レコグナイザー機能
   - シンセサイザ機能
   がArduinoで呼べるようになります。

α版のinstallは、

Arduinoのファイル / 環境設定 の中の <br>
追加ボードマネージャのURLに、 <br>
https://github.com/TomonobuHayakawa/spresense-arduino-compatible/releases/download/common/package_spresense_instrument_index.json <br>
を追加してください。  

これを追加すると、ボードマネージャの中に、  

*Spresense Instrument*  

というボードが新たに選択できます。  

これを利用することで、上記機能を使用することができます。

注) 現時点で評価ができているものではありません。バグ等に関しての責任は負いかねます。
注) 現在提供のAPIは、暫定版です。正式版で変更される可能性があります。

*sketches/Instruments*

は、こちらのボードで作成されています。

12/5 

・楽器に向けての @gokankenichi さん作 PitchScaleAdjuster をライブラリ化しました。


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
* PitchScaleAdjuster
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
    - SmartDrum
    - YuruHorn

- M11S
    - Jpeg_ESP8266_CamServer
    - Jpeg_shot_with_SD

- USB_UART
- I2cScanner

- LowPowerSensing
    - bme680_ulp_plus_via_sigfox
    - bme680_uulp_plus_via_sigfox


