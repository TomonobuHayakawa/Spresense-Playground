# SpreSynth Project

Spresense を使って Moduler Synthesizer のモジュールを作成できないか？の実証検証プロジェクト

-------------------- 
## What's new.

2023/5/30

1st release.

--------------------
## ディレクトリ構成

ディレクトリ構成は以下になります。

```
SpreSynth
 |
 ├ schematic : 回路図
 |
 ├ Layout : レイアウト図
 |
 ├ Arduino
     |
     ├ libraries : Arduinoライブラリ
     |
     ├ examples : Arduinoスケッチ

```

--------------------
## Arduino IDEでのライブラリ及びサンプルコード

SpreSynth 基板上で動作するソフトウエアです。<BR>
Arduino IDE 環境でのコードになります。<BR><BR>


--------------------
### Arduino IDEのパッケージ
Arduinoパッケージは、以下を使ってください。

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

--------------------
### Arduino の外部ライブラリ

こちらのボードを制御するにあたり、以下の Arduinoライブラリのインストールが必要です。

https://github.com/destrudo/CY8C95X0-Arduino<br>
https://github.com/kzhioki/Adafruit-GFX-Library<br>
https://github.com/kzhioki/Adafruit_ILI9341<br>

こちらをインストールしてください。

--------------------
### Arduino のサンプルスケッチ

サンプルスケッチは以下のものがあります。

・oscillator + sequencer サンプル

　簡単な波形生成+シーケンス再生のサンプルです。


### oscillator + sequencer サンプル

以下、ディレクトリ構成です。

```
 ├ Arduino
     |
     ├ examples : Arduinoスケッチ
         |
         ├ Ocillator : MainCore のスケッチ
             |
             ├ ControllerCore : 各種スイッチを制御するコードの SubCore のスケッチ (Core1で動かす)
             |
             ├ ViewerCore : LCDへの描画などを行うコードの Subcore のスケッチ (Core2で動かす)
             |
             ├ worker : 信号生成を行うworker の elf

```

以下、使い方です。

SDカードの `/BIN/` の下に `worker` の下の `OSCPROC` を配置してください。

`Ocillator.ino` を `MainCore` <br>
`ControllerCore.ino` を `SubCore1` <br>
`ViewerCore.ino` を `SubCore2` <br>

にそれぞれロードし、実行してください。