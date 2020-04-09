#�M���������C�u������MultiCore�̃T���v��

-------------------------

v1.3.0 �ŁAArduino�ł�MultiCore���\�ɂȂ����̂ŁA
FFT�AIIR���͂��߂Ƃ���M�������̃��C�u��������V���܂����B


## FFT
FFT���C�u�����́ACMSIS-DSP����荞�܂ꂽ�̂ŁA����������̂܂܌ĂԂ��ƂŁA
�J���҂����R�Ɏ����ł���悤�ɃR�[�h�ł̒񋟂����܂��B
����ɂ��ADSP�̃C���X�g�[���Ȃǂ͂Ȃ��Ȃ�܂����B

�������T���v�����쐬��Up���Ă��܂��B

�E�s�[�N���g���̌��o
�E������g�������̌��o
�EFFT/iFFT��p����VoiceChanger


CMSIS-DSP�̎g�p�������̊֌W�ŁA���ׂĂ�TAP���̃R�[�h��build����
�A�[�J�C�u�������Ƃ�����������߁AFFT�̃^�b�v���ƍő�`�����l������
template�����ɂ��āA�A�v���R���p�C�����Ɋm�肷��悤�ɂ��܂����B

FFT�̃��C�u�������g�p����ꍇ�A

FFTClass<"�ő�`�����l����", "FFT��TAP��"> FFT;

�����Ē�`���Ă��������B


### API

#### begin

  �����F
  	windowType_t type,			: Window�^�C�v�i�n�~���O�A�n�j���O�A�t�B���^�Ȃ�(��`)�j
  	int channel,				: �g�p����`�����l�����B���ő�`�����l�����ȉ��Œ�`
  	int overlap					: FFT���I�[�o�[���b�v�����ĉ�͂���T���v�����iFFT��TAP���̔����ȉ��j

  �߂�l�F
	bool						: OK/NG
  
  �����F
	���C�u�����̏��������s���܂��B
	�����Ȃ��̏ꍇ�A�n�~���O�A�ő�`�����l�����AFFT�̔�����Window���炵�Œ�`����܂��B


#### put

  �����F
  	q15_t* pSrc,				: ���̓f�[�^�̃|�C���^�B�i�`�����l���C���^�[���[�u�f�[�^�j
  	int size					: ���̓T���v����

  �߂�l�F
	bool						: OK/NG
  
  �����F
	��͂������f�[�^�����C�u�����ɓ��͂��܂��B


#### get

  �����F
  	float* out,					: �o�͐�̂̃|�C���^�B�i�`�����l�����Ɓj
  	int channel					: ���o�������`�����l���ԍ����w��

  �߂�l�F
	int							: ��͂��ς񂾃T���v����
  
  �����F
	FFT�ł̉�͌��ʂ��擾���܂��B��͌��ʂ̓`�����l�����ƂɂȂ�܂��B
	�e���g���̃p���[�X�y�N�g�����Ԃ�܂��B
	
	
#### get_raw

  �����F
  	float* out,					: �o�͐�̂̃|�C���^�B�i�`�����l�����Ɓj
  	int channel					: ���o�������`�����l���ԍ����w��

  �߂�l�F
	int							: ��͂��ς񂾃T���v����
  
  �����F
	FFT�ł̉�͌��ʂ��擾���܂��B��͌��ʂ̓`�����l�����ƂɂȂ�܂��B
	FFT���|�������̂܂܂̕��f���f�[�^�����o���܂��B
	iFFT���������ăf�[�^�����H�������ꍇ�͂�������g���Ă��������B


#### clear

  �����F �Ȃ�

  �߂�l�F �Ȃ�
  
  �����F
	�����̃o�b�t�@���N���A���܂��B


#### end

  �����F �Ȃ�

  �߂�l�F �Ȃ�
  
  �����F
	���C�u������j�����܂��B

#### empty

  �����F
  	int channel					: ���ׂ����`�����l���ԍ����w��

  �߂�l�F
	bool						: �ǂݏo���f�[�^���󂩂ǂ���

  �����F
	�ǂݏo���f�[�^�����邩�ǂ����𒲂ׂ܂��B
	

### �g����

	�ȉ��̃T���v���R�[�h���Q�l�ɂ��Ă��������B
	
	examples/PeakFetector/SubFFT/SubFFT.ino
	examples/SoundFetector/SubFFT/SubFFT.ino
	examples/Voice_changer/SubFFT/SubFFT.ino


## IIR
IIR���C�u�����́A�V�K�ɒǉ����܂����B
��{�AFFT�Ɠ����\���ɂ��Ă��܂��B
�܂��́A16bit�̂ݑΉ��B
���̌�A32bit���Ή����܂��B

�T���v���́A�ȉ��B

�ELPF
�EHPF


���C�u�����������N�����IIR�Ƃ����N���X�C���X�^���X���ÓI�ɍ쐬����܂��B

### API

#### ������



  �����F
  	filterType_t type,			:�t�B���^�̃^�C�v�ł��BLPF/HPF/BPF���I�ׂ܂��B
  	int channel, 				:�`�����l�����ł��B
  	int cutoff, 				:�J�b�g�I�t�̎��g���ł��B
  	float q						:Q�l�ł��B

  �߂�l�F �Ȃ�

  �����F
	���C�u�����̏��������s���܂��B
	
	

  bool put(q15_t* pSrc, int size);
  int  get(q15_t* pDst, int channel);

#### put

  �����F
  	q15_t* pSrc,				: ���̓f�[�^�̃|�C���^�B�i�`�����l���C���^�[���[�u�f�[�^�j
  	int size					: ���̓T���v����

  �߂�l�F
	bool						: OK/NG
  
  �����F
	��͂������f�[�^�����C�u�����ɓ��͂��܂��B


#### get

  �����F
  	float* out,					: �o�͐�̂̃|�C���^�B�i�`�����l�����Ɓj
  	int channel					: ���o�������`�����l���ԍ����w��

  �߂�l�F
	int							: �o�̓T���v����
  
  �����F
	IIR�������ĕ]���������ʂ��o�͂��܂��B

#### end

  �����F �Ȃ�

  �߂�l�F �Ȃ�
  
  �����F
	���C�u������j�����܂��B

#### empty

  �����F
  	int channel					: ���ׂ����`�����l���ԍ����w��

  �߂�l�F
	bool						: �ǂݏo���f�[�^���󂩂ǂ���

  �����F
	�ǂݏo���f�[�^�����邩�ǂ����𒲂ׂ܂��B
	

### �g����

	�ȉ��̃T���v���R�[�h���Q�l�ɂ��Ă��������B
	
	examples/HighPassSound/SubFFT/SubFFT.ino
	examples/LowPassSound/SubFFT/SubFFT.ino

