# AccessPoint/wifi-lte

Spresense の SubCore で GS2200 を Limited AP として起動し、
受信した TCP データを MainCore 側へ渡して LTE 経由で MQTT Publish するサンプルです。

## 構成

- `Spresense/MainCore/MainCore.ino`
- `Spresense/LimitedAP/LimitedAP.ino` (SubCore1)
- `Spresense/LimitedAP/config.h` (WiFi AP 設定)

## 事前設定

### 1. LTE 設定 (MainCore)
`Spresense/MainCore/MainCore.ino` の以下を環境に合わせて変更してください。

- `APP_LTE_APN`
- `APP_LTE_USER_NAME`
- `APP_LTE_PASSWORD`
- 必要に応じて `APP_LTE_AUTH_TYPE`, `APP_LTE_RAT`

### 2. MQTT 設定 (MainCore)
`Spresense/MainCore/MainCore.ino` の以下を変更してください。

- `BROKER_NAME`
- `BROKER_PORT`
- `MQTT_TOPIC`

### 3. WiFi AP 設定 (SubCore)
`Spresense/LimitedAP/config.h` の以下を変更してください。

- `AP_SSID`
- `PASSPHRASE`
- `AP_CHANNEL`
- `TCPSRVR_PORT`

## 実行手順

1. `Spresense/LimitedAP/LimitedAP.ino` を SubCore1 に書き込みます。
2. `Spresense/MainCore/MainCore.ino` を MainCore に書き込みます。
3. シリアルモニタで MainCore ログを確認します。
4. AP (`AP_SSID`) にクライアントを接続し、`TCPSRVR_PORT` に TCP 送信します。
5. 受信文字列が MQTT ブローカーへ Publish されます。

## 補足

- `LimitedAP.ino` は `SUBCORE == 1` 前提です。
- MainCore は起動時に `MP.begin(wifi_core)` で SubCore1 を起動します。
- 文字列処理は終端 `\0` を含む前提で転送しています。
