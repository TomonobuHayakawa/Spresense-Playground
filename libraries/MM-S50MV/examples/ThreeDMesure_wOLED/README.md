# Readme for ThreeDMesure_wOLED.ino
Readme for ThreeDMesure_wOLED.ino


--------------------
#ThreeDMesure_wOLED.ino の使い方

OLED での SSD1327表示を行っています。
Spresenseでは、このOLEDには Qwiic を使って簡単に接続が可能です。


・Zio Qwiic - 1.5インチ OLEDディスプレイ（128 x 128）
https://www.switch-science.com/catalog/6110/

・SPRESENSE用Qwiic接続基板
https://www.switch-science.com/catalog/6318/

これを使うために、以下のライブラリをインストールしてください。

https://github.com/olikraus/U8g2_Arduino


これらの準備ができれば、動作します。


OLED上にマッピングされた座標の中で、近距離に障害物を検知した場合、
その座標が塗りつぶされます。

