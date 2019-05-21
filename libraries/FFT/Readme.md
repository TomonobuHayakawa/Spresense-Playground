#FFTライブラリとMultiCoreのサンプル

-------------------------

センサデータのF特を取りたいとのことで、
あわてて、FFTのライブラリとその処理をDSPに
オフロードするライブラリとサンプルスケッチを作りました。

今回、SDK側のFFTライブラリの更新に伴い、Arduino側も修正しました。
本線には、近いうちにマージ予定。

SDカードからのDSPロードにしていますが、
Flashからも可能。dsp_installerがまだ。
現在はDSPのelfが置いてあります。



sound_detecter.ino
特定の周波数帯の音声を検出するサンプル。

spectrum_analyzer.ino
周波数分布をLCDにリアルタイムで表示するサンプル。


-------------------------

本ライブラリのDSPバイナリは、Keil社のCMSIS DSP Software Libraryを使用しています。

CMSIS DSP Software Libraryのライセンスファイルは、同フォルダの、"LICENSE.txt" にあります。

