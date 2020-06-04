# Spresense-Playground
Let's Play Spresense!

Spresenseを使った遊び場です。 + 
SPRESENSEで使える各種ライブラリ、サンプルコードを開発・提供していきます。

本線リポジトリに取り込まれたものは、削除していきます。

--------------------
## 現在の各ボードパッケージのバージョン

Spresense HighSpeedADC : v1.5.0 (Base v1.5.0) <br>
Spresense Instrument   : v1.5.3 (Base v1.5.1) <br>
Spresense M11S         : v1.5.1 (Base v1.5.0) <br>

-------------------- 
## What's new.

4/23

・オシレータライブラリの更新。<BR>
   - 波形タイプの動的変更<BR>
   - monoチャンネルのバグ修正<BR>
   

4/10

・SignalProcessingを本線に入れたのに合わせて、こちらも更新。
※次のリリース（v2.00）に入ります。

4/2

・CO-Spresense向けのサンプルを追加。

3/27

・オシレータのバグを修正。

・楽器向けボード(Spresense Instrument)の更新 (1.5.3) を行いました。 <br>
    - 本線のv1.5.1を取り込みました。 <br>
    - マイク入力を加工して出力するサンプルを追加しました。 <br>
    - MP3音声を再生しながら、マイク入力を加工して出力するサンプルを追加しました。 <br>
      - single_core : MainCoreのみ使用。Arduinoで加工可能。 <br>

※今回更新のスケッチは、メモリレイアウトなどの関係から、通常のSpresenseのボードでは動作しません。 <BR>
  *Spresense Instrument* のボードを使用してください。（使用方法は下記参照）

```
Spresense-Playground
 ｜
 ├ sketchies : Arduinoスケッチを置く場所。
     ｜
     ├ AudioPlayers
     ｜   ｜
     ｜   ├ player_with_effect_mic
     ｜
     ├ SoundEffector
         ｜
         ├ single_core

```

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

3/18

・シンセサイザ機能むけオシレータ&エンベロープジェネレータのライブラリを作成しました。

3/16

・FFTライブラリの修正。

FFTライブラリのFFTのTap数、最大のチャンネル数が固定でしか運用できなかったため、
非常に使いづらかったです。<br>
ですが、同時に、CMSIS-DSPのライブラリがTAP数ごとにデータを持つため、
すべてのTAP数をリンクするとサイズが大きくなってしまう問題がありました。<br>
そのため、今回の実装で、templateで実装し回避しました。<br>
※苦肉の策…。<br>

不評であれば、再度考えます。<br>


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

### シンセサイザ機能関連
* Oscillator

---
## Arduino スケッチリスト
- AudioPlayers
    - audio_with_sensing
    - diy_player
    - diy_player_wo_sensing
    - NetRadio_w_sensing
    - player_with_effect_mic
    - player_with_mic

- AudioRendering
    - rendering_objif

- Co-Spresense

- Instruments
    - SmartCajon
    - SmartDrum
    - YuruHorn

- M11S
    - Jpeg_ESP8266_CamServer
    - Jpeg_shot_with_SD

- SoundEffector
    - multi_core
    - single_core

- USB_UART
- I2cScanner

- LowPowerSensing
    - bme680_ulp_plus_via_sigfox
    - bme680_uulp_plus_via_sigfox

