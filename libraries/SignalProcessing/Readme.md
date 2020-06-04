#信号処理ライブラリとMultiCoreのサンプル

-------------------------

v1.3.0 で、ArduinoでのMultiCoreが可能になったので、
FFT、IIRをはじめとする信号処理のライブラリを一新しました。


## FFT
FFTライブラリは、CMSIS-DSPが取り込まれたので、こちらをそのまま呼ぶことで、
開発者が自由に実装できるようにコードでの提供をします。
これにより、DSPのインストールなどはなくなりました。

いくつかサンプルを作成しUpしています。

・ピーク周波数の検出
・特定周波数音声の検出
・FFT/iFFTを用いたVoiceChanger


CMSIS-DSPの使用メモリの関係で、すべてのTAP長のコードをbuildした
アーカイブを持つことが難しかったため、FFTのタップ長と最大チャンネル数を
template引数にして、アプリコンパイル時に確定するようにしました。

FFTのライブラリを使用する場合、

FFTClass<"最大チャンネル数", "FFTのTAP数"> FFT;

を入れて定義してください。


### API

#### begin

  引数：
  	windowType_t type,			: Windowタイプ（ハミング、ハニング、フィルタなし(矩形)）
  	int channel,				: 使用するチャンネル数。※最大チャンネル数以下で定義
  	int overlap					: FFTをオーバーラップさせて解析するサンプル数（FFTのTAP長の半分以下）

  戻り値：
	bool						: OK/NG
  
  説明：
	ライブラリの初期化を行います。
	引数なしの場合、ハミング、最大チャンネル数、FFTの半分のWindowずらしで定義されます。


#### put

  引数：
  	q15_t* pSrc,				: 入力データのポインタ。（チャンネルインターリーブデータ）
  	int size					: 入力サンプル数

  戻り値：
	bool						: OK/NG
  
  説明：
	解析したいデータをライブラリに入力します。


#### get

  引数：
  	float* out,					: 出力先ののポインタ。（チャンネルごと）
  	int channel					: 取り出したいチャンネル番号を指定

  戻り値：
	int							: 解析が済んだサンプル数
  
  説明：
	FFTでの解析結果を取得します。解析結果はチャンネルごとになります。
	各周波数のパワースペクトルが返ります。
	
	
#### get_raw

  引数：
  	float* out,					: 出力先ののポインタ。（チャンネルごと）
  	int channel					: 取り出したいチャンネル番号を指定

  戻り値：
	int							: 解析が済んだサンプル数
  
  説明：
	FFTでの解析結果を取得します。解析結果はチャンネルごとになります。
	FFTを掛けたそのままの複素数データを取り出します。
	iFFT等をかけてデータを加工したい場合はこちらを使ってください。


#### clear

  引数： なし

  戻り値： なし
  
  説明：
	内部のバッファをクリアします。


#### end

  引数： なし

  戻り値： なし
  
  説明：
	ライブラリを破棄します。

#### empty

  引数：
  	int channel					: 調べたいチャンネル番号を指定

  戻り値：
	bool						: 読み出しデータが空かどうか

  説明：
	読み出すデータがあるかどうかを調べます。
	

### 使い方

	以下のサンプルコードを参考にしてください。
	
	examples/PeakFetector/SubFFT/SubFFT.ino
	examples/SoundFetector/SubFFT/SubFFT.ino
	examples/Voice_changer/SubFFT/SubFFT.ino


## IIR
IIRライブラリは、新規に追加しました。
基本、FFTと同じ構造にしています。
まずは、16bitのみ対応。
その後、32bitも対応します。

サンプルは、以下。

・LPF
・HPF


ライブラリをリンクするとIIRというクラスインスタンスが静的に作成されます。

### API

#### 初期化



  引数：
  	filterType_t type,			:フィルタのタイプです。LPF/HPF/BPFが選べます。
  	int channel, 				:チャンネル数です。
  	int cutoff, 				:カットオフの周波数です。
  	float q						:Q値です。

  戻り値： なし

  説明：
	ライブラリの初期化を行います。
	
	

  bool put(q15_t* pSrc, int size);
  int  get(q15_t* pDst, int channel);

#### put

  引数：
  	q15_t* pSrc,				: 入力データのポインタ。（チャンネルインターリーブデータ）
  	int size					: 入力サンプル数

  戻り値：
	bool						: OK/NG
  
  説明：
	解析したいデータをライブラリに入力します。


#### get

  引数：
  	float* out,					: 出力先ののポインタ。（チャンネルごと）
  	int channel					: 取り出したいチャンネル番号を指定

  戻り値：
	int							: 出力サンプル数
  
  説明：
	IIRをかけて評価した結果を出力します。

#### end

  引数： なし

  戻り値： なし
  
  説明：
	ライブラリを破棄します。

#### empty

  引数：
  	int channel					: 調べたいチャンネル番号を指定

  戻り値：
	bool						: 読み出しデータが空かどうか

  説明：
	読み出すデータがあるかどうかを調べます。
	

### 使い方

	以下のサンプルコードを参考にしてください。
	
	examples/HighPassSound/SubFFT/SubFFT.ino
	examples/LowPassSound/SubFFT/SubFFT.ino

