#VoiceChangerのサンプル

-------------------------

Voice Changerを作ってみようとのことで、
オフロードするライブラリとサンプルスケッチを作りました。

時間があったら、ブラッシュアップしたいんだけど、時間がなかなか取れないので、
誰か作ってくれたらいいな…。


SDカードからのDSPロードにしていますが、
Flashからも可能。dsp_installerがまだ。
現在はDSPのelfが置いてあります。


pictch_change_with_FFT.ino
を実行すると、加速度センサの向きに合わせて、ピッチを変更します。
FFTなので、かなりノイジー。
今後、修正しようと思っています。


-------------------------

本ライブラリのDSPバイナリは、Keil社のCMSIS DSP Software Libraryを使用しています。

CMSIS DSP Software Libraryのライセンスファイルは、同フォルダの、"LICENSE.txt" にあります。

