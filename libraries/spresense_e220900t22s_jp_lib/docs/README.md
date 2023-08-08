# spresense_e220900t22s_jp_lib

## ライブラリの機能
このライブラリでは、以下の機能を提供します。
* E220-900T22S(JP)へのパラメータ初期設定
* LoRa送受信

## 利用方法
このライブラリを利用するには、Arduinoのライブラリフォルダに配置し、スケッチで次のようにインクルードしてください。
```
#include "spresense_e220900t22s_jp_lib.h"
```
また、上記ヘッダーファイル内の「E220-900T22S(JP)へのピンアサイン」を回路に合わせて変更してください。

## サンプルコード実行例
受信時
```
recv data:
hello world
hex dump:
68 65 6c 6c 6f 20 77 6f 72 6c 64 
RSSI: -113 dBm
```

送信時
```
000000hello
send succeeded.
```

## API

## Methods

### `InitLoRaModule()`
E220-900T22S(JP)をコンフィグモード(M0=1,M1=1)へ移行させ、引数`config`の構造体メンバをもとにLoRa初期設定を行います。
#### Syntax
```
int InitLoRaModule(struct LoRaConfigItem_t &config)
```
#### Parameters
* _config_: 設定値の格納先
#### Returns
`0`: 成功; `1`: 失敗

### `RecieveFrame()`
LoRa受信を行い、引数`recv_frame`の構造体メンバへ値を格納します。
#### Syntax
```
int RecieveFrame(struct RecvFrameE220900T22SJP_t *recv_frame)
```
#### Parameters
* _recv_frame_: LoRa受信データの格納先
#### Returns
`0`: 成功; `1`: 失敗

### `SendFrame()`
引数`send_data`のLoRa送信を行います。
#### Syntax
```
int SendFrame(struct LoRaConfigItem_t &config, uint8_t *send_data, int size)
```
#### Parameters
* _config_: 設定値の格納先
* _send_data_: 送信データ
* _size_: 送信データサイズ
#### Returns
`0`: 成功; `1`: 失敗

### `SwitchToNormalMode()`
ノーマルモード(M0=0,M1=0)へ移行します。
#### Syntax
```
void SwitchToNormalMode(void)
```
#### Parameters
none
#### Returns
none

### `SwitchToWORSendingMode()`
WOR送信モード(M0=1,M1=0)へ移行します。
#### Syntax
```
void SwitchToWORSendingMode(void)
```
#### Parameters
none
#### Returns
none

### `SwitchToWORReceivingMode()`
WOR受信モード(M0=0,M1=1)へ移行します。
#### Syntax
```
void SwitchToWORReceivingMode(void)
```
#### Parameters
none
#### Returns
none

### `SwitchToConfigurationMode()`
コンフィグモード(M0=1,M1=1)へ移行します。
#### Syntax
```
void SwitchToConfigurationMode(void)
```
#### Parameters
none
#### Returns
none
