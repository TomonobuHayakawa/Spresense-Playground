/****************************************************************************
 * audio_player/audio_player_main.cxx
 *
 *   Copyright (C) 2017 Sony Corporation. All rights reserved.
 *   Author: Tomonobu Hayakawa<Tomonobu.Hayakawa@sony.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor Sony nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <sdk/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include "system/readline.h"
#include <mqueue.h>
#include <asmp/mpshm.h>
#include <sys/wait.h>
#include <arch/board/board.h>

#include <arch/chip/cxd56_audio.h>
#include <arch/chip/pm.h>
#include <sys/stat.h>

#include <audio/audio_high_level_api.h>
#include <audio/utilities/playlist.h>
#include <memutils/simple_fifo/CMN_SimpleFifo.h>
#include <memutils/memory_manager/MemHandle.h>
#include <memutils/message/Message.h>
#include "include/msgq_id.h"
#include "include/mem_layout.h"
#include "include/memory_layout.h"
#include "include/msgq_pool.h"
#include "include/pool_layout.h"
#include "include/fixed_fence.h"

#if !defined(CONFIG_SDK_AUDIO) || !defined(CONFIG_AUDIOUTILS_PLAYER)
#error "Configs [SDK audio] and [Audio player] are required."
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define AUDIO_TASK_PRIORITY    (150)
#define AUDIO_TASK_PLAYER_DAEMON_PRIORITY    (100)
#define AUDIO_TASK_PLAYER_DAEMON_STACK_SIZE  (1024 * 3)

#define BIT16_DATA  0
#define BIT24_DATA  1
/* Use data bit length.
 *   Set BIT16_DATA or BIT24_DATA as type
 */
#define USE_DATA_BIT_LENGTH  BIT24_DATA

#define WRITE_16BIT_FIFO_SIZE  2560
#define WRITE_24BIT_FIFO_SIZE  4096
#if USE_DATA_BIT_LENGTH == BIT16_DATA
#  define WRITE_SIMPLE_FIFO_SIZE  WRITE_16BIT_FIFO_SIZE
#else
#  define WRITE_SIMPLE_FIFO_SIZE  WRITE_24BIT_FIFO_SIZE
#endif
#define SIMPLE_FIFO_FRAME_NUM   (9)
#define SIMPLE_FIFO_BUF_SIZE    (WRITE_SIMPLE_FIFO_SIZE * SIMPLE_FIFO_FRAME_NUM)
#define SIMPLE_FIFO_FRAME_SUB_NUM   (9)
#define SIMPLE_FIFO_BUF_SUB_SIZE    (WRITE_SIMPLE_FIFO_SIZE * SIMPLE_FIFO_FRAME_SUB_NUM)

#define AUDIO_CODEC_DSP_VOL_STEP  5

#ifndef CONFIG_TEST_AUDIO_PLAYER_FILE_MOUNTPT
#  define CONFIG_TEST_AUDIO_PLAYER_FILE_MOUNTPT "/mnt/sd0/AUDIO"
#endif

#define AS_VOLUME_MAX  120
#define AS_VOLUME_MIN  -1020

/****************************************************************************
 * Private Data
 ****************************************************************************/
uint32_t m_simple_fifo_buf[SIMPLE_FIFO_BUF_SIZE/sizeof(uint32_t)];
uint8_t m_ram_buf[WRITE_SIMPLE_FIFO_SIZE];
CMN_SimpleFifoHandle m_simple_fifo_handle;
AsPlayerInputDeviceHdlrForRAM m_cur_input_device_handler;
static pid_t s_player_daemon_pid = -1;
int32_t m_main_file_size = 0;

uint32_t m_simple_fifo_buf_sub[SIMPLE_FIFO_BUF_SUB_SIZE/sizeof(uint32_t)];
uint8_t m_ram_buf_sub[WRITE_SIMPLE_FIFO_SIZE];
CMN_SimpleFifoHandle m_simple_fifo_handle_sub;
AsPlayerInputDeviceHdlrForRAM m_cur_input_device_handler_sub;
static pid_t s_sub_daemon_pid = -1;
bool sub_task_suspend = false;
int32_t m_sub_file_size = 0;

#ifdef CONFIG_AUDIOUTILS_PLAYLIST
Playlist* p_playlist_ins;
#else
char s_track_info[128];
#endif

typedef struct
{
  uint32_t  state;
  uint32_t  subplayer_state;
  uint8_t   output_device;
  int32_t   audio_codec_dsp_vol;
  DIR       *dirp;
  int32_t   fd;
} AudioPlayer;

static  AudioPlayer *p_player = NULL;

static mqd_t s_mq_cui_cmd;
static mqd_t s_mq_cui_cmd_sub;

static mpshm_t s_shm;

static struct pm_cpu_freqlock_s g_player_lock;
static struct pm_cpu_freqlock_s g_file_open_lock;

typedef enum
{
  BootedState = 0,
  ReadyState,
  PlayState,
  EofState,
  StoppedState,
  NumPlayerState
} PlayerState;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int app_create_context(AudioPlayer** player);
static int app_delete_context(AudioPlayer* player);
static int app_create_dir(void);
static int app_power_on(void);
static int app_power_off(void);
static int app_set_ready_status(void);
static int app_init_output_select(uint8_t output_device);
static int app_init_i2s_param(void);
static int app_set_volume(int *audio_codec_dsp_vol, int master_db);
static int app_set_incVol(int step_cnt, int *audio_codec_dsp_vol);
static int app_set_decVol(int step_cnt, int *audio_codec_dsp_vol);
static int app_set_player_status(void);
static int app_init_player(uint8_t codec_type, uint32_t sampling_rate, uint8_t channel_number, uint8_t bit_length);
static int app_play_player(void);
static int app_stop_player(int fd, int mode);
static int app_pause_player(void);
static int app_init_subplayer(void);
static int app_play_subplayer(void);
static int app_stop_subplayer(void);
static int app_adjust_clkrecovery(int dir, uint32_t times);
static int app_set_lr_gain(uint16_t l_gain, uint16_t r_gain);
static int app_init_simple_fifo(CMN_SimpleFifoHandle* handle, uint32_t* fifo_buf, int32_t fifo_size, AsPlayerInputDeviceHdlrForRAM* dev_handle);
static int app_first_push_simple_fifo(int fd);
static int app_push_simple_fifo(int fd);
#ifdef CONFIG_AUDIOUTILS_PLAYLIST
static int app_open_playlist(void);
static int app_get_next_track(Track* track);
#else
static int app_parse_track_info(Track* track, char* track_info);
#endif /* CONFIG_AUDIOUTILS_PLAYLIST */

static void player_freq_lock(uint32_t mode);
static void player_freq_release(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static int app_create_context(AudioPlayer** player)
{
  *player = (AudioPlayer *)malloc(sizeof(AudioPlayer));

  if (*player == NULL)
    {
      printf("player context creation failed.\n");
      return 1;
    }

  memset(*player, 0 , sizeof(AudioPlayer));
  
  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_delete_context(AudioPlayer* player)
{
  free(player);
  player = NULL;

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_create_dir(void)
{
  DIR *dirp;
  const char *name = CONFIG_TEST_AUDIO_PLAYER_FILE_MOUNTPT;
  
  dirp = opendir(name);

  if (!dirp)
    {
      printf("%s directory path error. check the path!\n",name);
      return 1;
    }

  p_player->dirp = dirp;

  return 0;
}

/*--------------------------------------------------------------------------*/
static void app_input_device_callback(uint32_t size)
{
    /* do nothing */
}

/*--------------------------------------------------------------------------*/
static void app_attention_callback(const ErrorAttentionParam *attparam)
{
  printf("Attention!! %s L%d ecode %d subcode %d\n",
          attparam->error_filename,
          attparam->line_number,
          attparam->error_code,
          attparam->error_att_sub_code);
}

/*--------------------------------------------------------------------------*/
static int app_power_on(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_POWERON;
  command.header.command_code  = AUDCMD_POWERON;
  command.header.sub_code      = 0x00;
  command.power_on_param.enable_sound_effect = AS_DISABLE_SOUNDEFFECT;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_STATUSCHANGED)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_power_off(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SET_POWEROFF_STATUS;
  command.header.command_code  = AUDCMD_SETPOWEROFFSTATUS;
  command.header.sub_code      = 0x00;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_STATUSCHANGED)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_set_ready_status(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SET_READY_STATUS;
  command.header.command_code  = AUDCMD_SETREADYSTATUS;
  command.header.sub_code      = 0x00;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_STATUSCHANGED)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_init_output_select(uint8_t output_device)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_INITOUTPUTSELECT;
  command.header.command_code  = AUDCMD_INITOUTPUTSELECT;
  command.header.sub_code      = 0;
  command.init_output_select_param.output_device_sel =
    (output_device == AS_SETPLAYER_OUTPUTDEVICE_SPHP)
      ? AS_OUT_SP : AS_OUT_I2S;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_INITOUTPUTSELECTCMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_init_i2s_param(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_INITI2SPARAM;
  command.header.command_code  = AUDCMD_INITI2SPARAM;
  command.header.sub_code      = 0;
  command.init_i2s_param.i2s_id = AS_I2S1;
  command.init_i2s_param.rate = 48000;
  command.init_i2s_param.bypass_mode_en = AS_I2S_BYPASS_MODE_DISABLE;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_INITI2SPARAMCMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_set_volume(int *audio_codec_dsp_vol, int master_db)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SETVOLUME;
  command.header.command_code  = AUDCMD_SETVOLUME;
  command.header.sub_code      = 0;
  command.set_volume_param.input1_db = 0;
  command.set_volume_param.input2_db = 0;
  command.set_volume_param.master_db = master_db;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_SETVOLUMECMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  *audio_codec_dsp_vol = AS_AC_CODEC_VOL_DAC;

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_set_incVol(int step_cnt, int *audio_codec_dsp_vol)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SETVOLUME;
  command.header.command_code  = AUDCMD_SETVOLUME;
  command.header.sub_code      = 0;
  if (AS_VOLUME_MAX >= *audio_codec_dsp_vol + (AUDIO_CODEC_DSP_VOL_STEP * step_cnt))
    {
      *audio_codec_dsp_vol += (AUDIO_CODEC_DSP_VOL_STEP * step_cnt);
      command.set_volume_param.master_db = *audio_codec_dsp_vol;
    }
  else
    {
      command.set_volume_param.master_db = AS_VOLUME_HOLD;
    }
  command.set_volume_param.input1_db = 0;
  command.set_volume_param.input2_db = AS_VOLUME_HOLD;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_SETVOLUMECMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_set_decVol(int step_cnt, int32_t *audio_codec_dsp_vol)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SETVOLUME;
  command.header.command_code  = AUDCMD_SETVOLUME;
  command.header.sub_code      = 0;
  if (AS_VOLUME_MIN <= *audio_codec_dsp_vol - (AUDIO_CODEC_DSP_VOL_STEP * step_cnt))
    {
      *audio_codec_dsp_vol -= (AUDIO_CODEC_DSP_VOL_STEP * step_cnt);
      command.set_volume_param.master_db = *audio_codec_dsp_vol;
    }
  else
    {
      command.set_volume_param.master_db = AS_VOLUME_HOLD;
    }
  command.set_volume_param.input1_db = 0;
  command.set_volume_param.input2_db = AS_VOLUME_HOLD;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_SETVOLUMECMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_set_player_status(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SET_PLAYER_STATUS;
  command.header.command_code = AUDCMD_SETPLAYERSTATUS;
  command.header.sub_code = 0x00;
  command.set_player_sts_param.active_player  = AS_ACTPLAYER_BOTH;
  command.set_player_sts_param.player0.input_device   = AS_SETPLAYER_INPUTDEVICE_RAM;
  command.set_player_sts_param.player0.ram_handler    = &m_cur_input_device_handler;
  command.set_player_sts_param.player0.output_device  = 0;/*don't care*/
  command.set_player_sts_param.player1.input_device   = AS_SETPLAYER_INPUTDEVICE_RAM;
  command.set_player_sts_param.player1.ram_handler    = &m_cur_input_device_handler_sub;
  command.set_player_sts_param.player1.output_device  = 0;/*don't care*/
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_STATUSCHANGED)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_init_player(uint8_t codec_type, uint32_t sampling_rate, uint8_t channel_number, uint8_t bit_length)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_INIT_PLAYER;
  command.header.command_code = AUDCMD_INITPLAYER;
  command.header.sub_code = 0x00;
  command.player.player_id                = AS_PLAYER_ID_0;
  command.player.init_param.codec_type    = codec_type;
  command.player.init_param.bit_length    = bit_length;
  command.player.init_param.channel_number= channel_number;
  command.player.init_param.sampling_rate = sampling_rate;
  snprintf(command.player.init_param.dsp_path, AS_AUDIO_DSP_PATH_LEN, "%s", "/mnt/sd0/BIN");
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_INITPLAYERCMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_play_player(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_PLAY_PLAYER;
  command.header.command_code = AUDCMD_PLAYPLAYER;
  command.header.sub_code = 0x00;
  command.player.player_id = AS_PLAYER_ID_0;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_PLAYCMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x) Error subcode(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code,
             result.error_response_param.error_sub_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_stop_player(int fd, int mode)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_STOP_PLAYER;
  command.header.command_code = AUDCMD_STOPPLAYER;
  command.header.sub_code = 0x00;
  command.player.player_id            = AS_PLAYER_ID_0;
  command.player.stop_param.stop_mode = mode;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_STOPCMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  if (mode == AS_STOPPLAYER_NORMAL)
    {
      CMN_SimpleFifoClear(&m_simple_fifo_handle);
      close(fd);
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_pause_player(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_STOP_PLAYER;
  command.header.command_code = AUDCMD_STOPPLAYER;
  command.header.sub_code = 0x00;
  command.player.player_id            = AS_PLAYER_ID_0;
  command.player.stop_param.stop_mode = AS_STOPPLAYER_NORMAL;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_STOPCMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
             command.header.command_code,
             result.header.result_code,
             result.error_response_param.module_id,
             result.error_response_param.error_code);

      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_init_subplayer(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_INIT_PLAYER;
  command.header.command_code  = AUDCMD_INITPLAYER;
  command.header.sub_code      = 0x00;
  command.player.player_id                = AS_PLAYER_ID_1;
  command.player.init_param.codec_type    = AS_CODECTYPE_MP3;
  command.player.init_param.bit_length    = AS_BITLENGTH_16;
  command.player.init_param.channel_number= AS_CHANNEL_STEREO;
  command.player.init_param.sampling_rate = AS_SAMPLINGRATE_48000;
  snprintf(command.player.init_param.dsp_path, AS_AUDIO_DSP_PATH_LEN, "%s", "/mnt/sd0/BIN");
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_INITPLAYERCMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
                command.header.command_code, result.header.result_code, result.error_response_param.module_id, result.error_response_param.error_code);
      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_play_subplayer(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_PLAY_PLAYER;
  command.header.command_code  = AUDCMD_PLAYPLAYER;
  command.header.sub_code      = 0x00;
  command.player.player_id = AS_PLAYER_ID_1;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_PLAYCMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
                command.header.command_code, result.header.result_code, result.error_response_param.module_id, result.error_response_param.error_code);
      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_stop_subplayer(void)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_STOP_PLAYER;
  command.header.command_code  = AUDCMD_STOPPLAYER;
  command.header.sub_code      = 0x00;
  command.player.player_id            = AS_PLAYER_ID_1;
  command.player.stop_param.stop_mode = AS_STOPPLAYER_NORMAL;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_STOPCMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
                command.header.command_code, result.header.result_code, result.error_response_param.module_id, result.error_response_param.error_code);
      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_adjust_clkrecovery(int dir, uint32_t times)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_CLK_RECOVERY;
  command.header.command_code  = AUDCMD_CLKRECOVERY;
  command.header.sub_code      = 0x00;
  command.clk_recovery_param.player_id = AS_PLAYER_ID_0;
  command.clk_recovery_param.direction = dir;
  command.clk_recovery_param.times     = times;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_CLKRECOVERY_CMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
                command.header.command_code, result.header.result_code, result.error_response_param.module_id, result.error_response_param.error_code);
      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_set_lr_gain(uint16_t l_gain, uint16_t r_gain)
{
  AudioCommand command;
  command.header.packet_length = LENGTH_SET_GAIN;
  command.header.command_code  = AUDCMD_SETGAIN;
  command.header.sub_code      = 0x00;
  command.player.player_id = AS_PLAYER_ID_0;
  command.player.set_gain_param.l_gain = l_gain;
  command.player.set_gain_param.r_gain = r_gain;
  AS_SendAudioCommand(&command);

  AudioResult result;
  AS_ReceiveAudioResult(&result);
  if (result.header.result_code != AUDRLT_SETGAIN_CMPLT)
    {
      printf("ERROR: Command (0x%x) fails. Result code(0x%x) Module id(0x%x) Error code(0x%x)\n",
                command.header.command_code, result.header.result_code, result.error_response_param.module_id, result.error_response_param.error_code);
      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_init_simple_fifo(CMN_SimpleFifoHandle* handle, uint32_t* fifo_buf, int32_t fifo_size, AsPlayerInputDeviceHdlrForRAM* dev_handle)
{
  if (CMN_SimpleFifoInitialize(handle, fifo_buf, fifo_size, NULL) != 0)
    {
      printf("Fail to initialize simple FIFO.");
      return 1;
    }
  CMN_SimpleFifoClear(handle);

  dev_handle->simple_fifo_handler = (void*)(handle);
  dev_handle->callback_function = app_input_device_callback;
  return 0;
}

/*--------------------------------------------------------------------------*/
static int push_simple_fifo(int fd)
{
  int ret;

  ret = read(fd, &m_ram_buf, WRITE_SIMPLE_FIFO_SIZE);
  if (ret < 0)
    {
      printf("Fail to read file. errno:%d\n",get_errno());
      return 1;
    }

  if (CMN_SimpleFifoOffer(&m_simple_fifo_handle, (const void*)(m_ram_buf), ret) == 0)
    {
      printf("Simple FIFO is full!\n");
      return 1;
    }
  m_main_file_size = (m_main_file_size - ret);
  if (m_main_file_size == 0)
    {
	  app_stop_player(fd, AS_STOPPLAYER_ESEND);

      close(fd);
      p_player->state = ReadyState;

      return 2;
    }
  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_first_push_simple_fifo(int fd)
{
  int i;
  int ret = 0;

  for(i = 0; i < SIMPLE_FIFO_FRAME_NUM-1; i++)
    {
      if ((ret = push_simple_fifo(fd)) != 0)
        {
          break;
        }
    }

  return ret;
}

/*--------------------------------------------------------------------------*/
static int app_push_simple_fifo(int fd)
{
  int32_t ret = 0;
  size_t vacant_size;

  vacant_size = CMN_SimpleFifoGetVacantSize(&m_simple_fifo_handle);

  if ((vacant_size != 0) && (vacant_size > WRITE_SIMPLE_FIFO_SIZE))
    {
    	int cnt=1;
    	if(vacant_size > WRITE_SIMPLE_FIFO_SIZE*3) {
    		cnt=3;
    	}
    	else if (vacant_size > WRITE_SIMPLE_FIFO_SIZE*2) {
    		cnt=2;
    	}
    	for (int i=0; i < cnt; i++)
          {
            if ((ret = push_simple_fifo(fd)) != 0)
              {
                break;
              }
    	  }
    }

  return ret;
}

#ifdef CONFIG_AUDIOUTILS_PLAYLIST
/*--------------------------------------------------------------------------*/
static int app_open_playlist(void)
{
  p_playlist_ins = new Playlist("TRACK_DB.CSV");

  p_playlist_ins->init("/mnt/sd0/PLAYLIST");
  p_playlist_ins->setPlayMode(Playlist::PlayModeNormal);
  p_playlist_ins->setRepeatMode(Playlist::RepeatModeOff);

  p_playlist_ins->select(Playlist::ListTypeAllTrack, NULL);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_close_playlist(void)
{
  delete p_playlist_ins;

  return 0;
}

/*--------------------------------------------------------------------------*/
static int app_get_next_track(Track* track)
{
  bool ret;
  ret = p_playlist_ins->getNextTrack(track);

  return (ret == true) ? 0 : 1;
}
#else
static int app_parse_track_info(Track* track, char* track_info)
{
  /* track name */

  strncpy(track->title, strtok(track_info, ","), sizeof(track->title));

  /* ch num */

  int ch_num = atoi(strtok(NULL, ","));
  if (ch_num == 1)
    {
      track->channel_number = AS_CHANNEL_MONO;
    }
  else if (ch_num == 2)
    {
      track->channel_number = AS_CHANNEL_STEREO;
    }
  else
    {
      return 1;
    }

  /* bit length */

  int length = atoi(strtok(NULL, ","));
  if (length == 16)
    {
      track->bit_length = AS_BITLENGTH_16;
    }
  else if (length == 24)
    {
      track->bit_length = AS_BITLENGTH_24;
    }
  else
    {
      return 1;
    }

  /* sampling rate */

  int rate = atoi(strtok(NULL, ","));
  if (rate == 8000)
    {
      track->sampling_rate = AS_SAMPLINGRATE_8000;
    }
  else if (rate == 16000)
    {
      track->sampling_rate = AS_SAMPLINGRATE_16000;
    }
  else if (rate == 24000)
    {
      track->sampling_rate = AS_SAMPLINGRATE_24000;
    }
  else if (rate == 32000)
    {
      track->sampling_rate = AS_SAMPLINGRATE_32000;
    }
  else if (rate == 44100)
    {
      track->sampling_rate = AS_SAMPLINGRATE_44100;
    }
  else if (rate == 48000)
    {
      track->sampling_rate = AS_SAMPLINGRATE_48000;
    }
  else if (rate == 64000)
    {
      track->sampling_rate = AS_SAMPLINGRATE_64000;
    }
  else if (rate == 88200)
    {
      track->sampling_rate = AS_SAMPLINGRATE_88200;
    }
  else if (rate == 96000)
    {
      track->sampling_rate = AS_SAMPLINGRATE_96000;
    }
  else if (rate == 192000)
    {
      track->sampling_rate = AS_SAMPLINGRATE_192000;
    }
  else
    {
      return 1;
    }

  /* code type */

  char codec[16];
  strncpy(codec, strtok(NULL, ","), sizeof(codec));
  if ((strncmp(codec, "wav", sizeof(codec)) == 0) ||
      (strncmp(codec, "WAV", sizeof(codec)) == 0))
    {
      track->codec_type = AS_CODECTYPE_WAV;
    }
  else if ((strncmp(codec, "mp3", sizeof(codec)) == 0) ||
           (strncmp(codec, "MP3", sizeof(codec)) == 0))
    {
      track->codec_type = AS_CODECTYPE_MP3;
    }
  else if ((strncmp(codec, "aac", sizeof(codec)) == 0) ||
           (strncmp(codec, "AAC", sizeof(codec)) == 0))
    {
      track->codec_type = AS_CODECTYPE_AAC;
    }
  else if ((strncmp(codec, "opus", sizeof(codec)) == 0) ||
           (strncmp(codec, "OPUS", sizeof(codec)) == 0))
    {
      track->codec_type = AS_CODECTYPE_OPUS;
    }
  else
    {
      return 1;
    }

  return 0;
}
#endif /* CONFIG_AUDIOUTILS_PLAYLIST */

/****************************************************************************
 * Public Type Declarations
 ****************************************************************************/
typedef enum
{
  InitEvent = 0,
  StartEvent,
  StopEvent,
  PauseEvent,
  ReleaseEvent,
  RenderDoneEvent,
  SubStartEvent,
  SubStopEvent,
  IncVolEvent,
  DecVolEvent,
  ClkRcvEvent,
  SetGainEvent,
  PowerOffEvent,
  NumPlayerEvent
} PlayerEvent;


typedef int (*player_func)(char* pargs, mqd_t mqd, mqd_t mqd_sub);

typedef struct
{
  const char   *cmd;       /* The command text */
  const char   *arghelp;   /* Text describing the args */
  player_func  pFunc;     /* Pointer to command handler */
  const char   *help;      /* The help text */
} PlayerCmd;

typedef union
{
  uint32_t data;
  uint32_t step;  
  uint32_t output_device;
  int32_t  recovery_val;

  struct __gain_st
  {
    uint16_t l_gain;
    uint16_t r_gain;
  } gain;

} PlayerEventParam;

typedef struct 
{
  PlayerEvent event;
  PlayerEventParam param;
} PlayerEventCmd;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
static int audioPlayer_createTask(void);
static int audioPlayer_deleteTask(void);
static int audioPlayer_cmdHelp(char* parg, mqd_t mqd, mqd_t mqd_sub);

static int audioPlayer_outDevice(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_play(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_stop(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_pause(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_release(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_subplay(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_substop(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_incVol(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_decVol(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_clkrcv(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_setgain(char* parg, mqd_t mqd, mqd_t mqd_sub);
static int audioPlayer_powerOff(char* parg, mqd_t mqd, mqd_t mqd_sub);

/****************************************************************************
 * Shell Commands
 ****************************************************************************/

static PlayerCmd player_cmds[] =
{
    { "h",                  "",                 audioPlayer_cmdHelp,        "Display help for commands. ex)player> h" },
    { "help",               "",                 audioPlayer_cmdHelp,        "Display help for commands. ex)player> help" },
    { "outDevice",          "device",           audioPlayer_outDevice,      "Set output device.         ex)player> outDevice SPHP\n                               prarameter: device[SPHP or I2S]\n                                caution) First run required after starting player application" },
#ifdef CONFIG_AUDIOUTILS_PLAYLIST
    { "play",               "",                 audioPlayer_play,           "Start player.              ex)player> play" },
#else
    { "play",               "track info",       audioPlayer_play,           "Start player.              ex)player> play music.mp3,2,16,48000,mp3\n                               prarameter: track info[filename,ch num,bit length,sampling rate,codec]" },
#endif
    { "stop",               "",                 audioPlayer_stop,           "Stop player.               ex)player> stop" },
    { "pause",              "",                 audioPlayer_pause,          "Pause player.              ex)player> pause\n                                caution) Request 'release' or 'stop' to release the pause. However, 'stop' will end playing" },
    { "release",            "",                 audioPlayer_release,        "Release pause.             ex)player> release" },
    { "subplay",            "",                 audioPlayer_subplay,        "Start subplayer.           ex)player> subplay" },
    { "substop",            "",                 audioPlayer_substop,        "Stop subplayer.            ex)player> substop" },
    { "incVol",             "stepValue",        audioPlayer_incVol,         "Increase digital volume.   ex)player> incVol 10\n                               prarameter: stepValue[1 step: +0.5dB]\n                                caution) If the output device is I2S the request is invalid" },
    { "decVol",             "stepValue",        audioPlayer_decVol,         "Decrease digital volume.   ex)player> decVol 10\n                               prarameter: stepValue[1 step: -0.5dB]\n                                caution) If the output device is I2S the request is invalid" },
    { "clkrcv",             "",                 audioPlayer_clkrcv,         "Excute clock recovery.     ex)player> clkrcv 10" },
    { "setgain",            "",                 audioPlayer_setgain,        "Set L/R gain.              ex)player> clkrcv 120 80" },
    { "q",                  "",                 audioPlayer_powerOff,       "Exit player application.   ex)player> q" },
    { "quit",               "",                 audioPlayer_powerOff,       "Exit player application.   ex)player> quit" }
};
static const int player_cmd_count = sizeof(player_cmds) / sizeof(PlayerCmd);

/****************************************************************************
 * Shell Commands Prossess
 ****************************************************************************/
static int audioPlayer_cmdHelp(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  int   x, len, maxlen = 0;
  int   c;

  for (x = 0; x < player_cmd_count; x++)
    {
      len = strlen(player_cmds[x].cmd) + strlen(player_cmds[x].arghelp);
      if (len > maxlen)
        {
          maxlen = len;
        }
    }

  printf("AudioPlayer commands\n===Commands=+=Prarameters===+=Description=============\n");
  for (x = 0; x < player_cmd_count; x++)
    {
      printf("  %s %s", player_cmds[x].cmd, player_cmds[x].arghelp);
      len = maxlen - (strlen(player_cmds[x].cmd) + strlen(player_cmds[x].arghelp));
      for (c = 0; c < len; c++)
        {
          printf(" ");
        }
      printf("          : %s\n", player_cmds[x].help);
    }
  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_outDevice(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  char *adr;

  PlayerEventCmd cmd;

  cmd.event = InitEvent;

  adr = strtok(parg, ",");

  if ((strncmp("SPHP", adr, 4) == 0) || (strncmp("sphp", adr, 4) == 0))
    {
      cmd.param.output_device = AS_SETPLAYER_OUTPUTDEVICE_SPHP;
    }
  else if ((strncmp("I2S", adr, 3) == 0) || (strncmp("i2s", adr, 3) == 0))
    {
      cmd.param.output_device = AS_SETPLAYER_OUTPUTDEVICE_I2SOUTPUT;
    }
  else
    {
      printf("Invalid output device(%s)\n", adr);
      return 1;
    }

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_play(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = StartEvent;
  cmd.param.data = 0;
#ifndef CONFIG_AUDIOUTILS_PLAYLIST
  snprintf(s_track_info, sizeof(s_track_info), "%s", parg);
#endif /* CONFIG_AUDIOUTILS_PLAYLIST */

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_stop(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = StopEvent;
  cmd.param.data = 0;

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_pause(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = PauseEvent;
  cmd.param.data = 0;

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_release(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = ReleaseEvent;
  cmd.param.data = 0;

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_incVol(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = IncVolEvent;
  cmd.param.step = atoi(parg);

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_decVol(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = DecVolEvent;
  cmd.param.step = atoi(parg);

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_clkrcv(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = ClkRcvEvent;
  cmd.param.step = atoi(parg);

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_setgain(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = SetGainEvent;
  cmd.param.gain.l_gain = atoi(strtok_r(parg, " ", &parg));
  cmd.param.gain.r_gain = atoi(strtok_r(parg, " ", &parg));

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_powerOff(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = PowerOffEvent;
  cmd.param.data = 0;

  mq_send(mqd, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/****************************************************************************
 * Command Event Table
 ****************************************************************************/

static bool player_parse(PlayerEvent event, PlayerEventParam* param);
static bool player_ignore(PlayerEventParam* param);
static bool player_ignoreRender(PlayerEventParam* param);
static bool player_init(PlayerEventParam* param);
static bool player_start(PlayerEventParam* param);
static bool player_stop(PlayerEventParam* param);
static bool player_pause(PlayerEventParam* param);
static bool player_release(PlayerEventParam* param);
static bool player_substart(PlayerEventParam* param);
static bool player_substop(PlayerEventParam* param);
static bool player_push_es(PlayerEventParam* param);
static bool player_next(PlayerEventParam* param);
static bool player_incvol(PlayerEventParam* param);
static bool player_decvol(PlayerEventParam* param);
static bool player_clkrcv(PlayerEventParam* param);
static bool player_setgain(PlayerEventParam* param);
static bool player_poweroff(PlayerEventParam* param);

typedef bool (*MsgProc)(PlayerEventParam*);
static MsgProc MsgProcTbl[NumPlayerEvent][NumPlayerState] = {

/*                     BootedState,          ReadyState            PlayState          EofState,         StoppedState */
/* InitEvent       */ {&player_init,         &player_ignore,       &player_ignore,    &player_ignore,   &player_ignore   },
/* StartEvent      */ {&player_ignore,       &player_start,        &player_ignore,    &player_ignore,   &player_ignore   },
/* StopEvent       */ {&player_ignore,       &player_ignore,       &player_stop,      &player_stop,     &player_ignore   },
/* PauseEvent      */ {&player_ignore,       &player_ignore,       &player_pause,     &player_pause,    &player_ignore   },
/* ReleaseEvent    */ {&player_ignore,       &player_ignore,       &player_release,   &player_release,  &player_ignore   },
/* RenderDoneEvent */ {&player_ignoreRender, &player_ignoreRender, &player_push_es,   &player_next,     &player_ignoreRender   },
/* SubStartEvent   */ {&player_ignore,       &player_substart,     &player_substart,  &player_substart, &player_ignore   },
/* SubStopEvent    */ {&player_ignore,       &player_substop,      &player_substop,   &player_substop,  &player_ignore   },
/* IncVolEvent     */ {&player_ignore,       &player_incvol,       &player_incvol,    &player_incvol,   &player_incvol   },
/* DecVolEvent     */ {&player_ignore,       &player_decvol,       &player_decvol,    &player_decvol,   &player_decvol   },
/* ClkRcvEvent     */ {&player_ignore,       &player_clkrcv,       &player_clkrcv,    &player_clkrcv,   &player_clkrcv   },
/* SetGainEvent    */ {&player_ignore,       &player_ignore,       &player_setgain,    &player_ignore,   &player_ignore   },
/* PowerOffEvent   */ {&player_poweroff,     &player_poweroff,     &player_poweroff,  &player_poweroff, &player_poweroff }
};


/****************************************************************************
 * Each Event Prossess
 ****************************************************************************/
static bool player_parse(PlayerEvent event, PlayerEventParam* param)
{
  return MsgProcTbl[event][p_player->state](param);
}

/*--------------------------------------------------------------------------*/
static bool player_ignore(PlayerEventParam* param)
{
  switch(p_player->state)
    {
    case BootedState:
      printf("command ignore [No 'outDevice' command has been input or an error occurred]\n");
      break;
    case ReadyState:
      printf("command ignore [No 'play' command has been input or an error occurred]\n");
      break;
    case PlayState:
    case EofState:
      printf("command ignore [Already playing]\n");
      break;
    }
  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_ignoreRender(PlayerEventParam* param)
{
  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_init(PlayerEventParam* param)
{
  p_player->output_device = param->output_device;
  /* Init Output Select */
  if (app_init_output_select(param->output_device))
    {
      return false;
    }

  if (param->output_device == AS_SETPLAYER_OUTPUTDEVICE_I2SOUTPUT)
    {
      /* Init I2S Param */
      if (app_init_i2s_param())
        {
          return false;
        }
    }

  /* Init Simple Fifo */
  if (app_init_simple_fifo(&m_simple_fifo_handle, m_simple_fifo_buf, SIMPLE_FIFO_BUF_SIZE, &m_cur_input_device_handler))
    {
      return false;
    }

  /* Set Player Status */
  if (app_set_player_status())
    {
      return false;
    }

  int volume = (param->output_device == AS_SETPLAYER_OUTPUTDEVICE_SPHP) ? AS_VOLUME_DAC : AS_VOLUME_HOLD;
  /* Set Volume */
  if (app_set_volume(&p_player->audio_codec_dsp_vol, volume))
    {
      return false;
    }

  int ret = 0;
  ret = board_external_amp_mute_control(false);
  if (ret)
    {
      printf("Amp mute off error. %d\n", ret);
      return false;
    }

  p_player->state = ReadyState;
  p_player->subplayer_state = ReadyState;

  return true;
}


/*--------------------------------------------------------------------------*/
static void init_freq_lock(void)
{
#ifndef CONFIG_PM_DISABLE_FREQLOCK_COUNT
  g_player_lock.count = 0;
#endif
  g_player_lock.info = PM_CPUFREQLOCK_TAG('A', 'P', 0);
  g_player_lock.flag = PM_CPUFREQLOCK_FLAG_HV;

#ifndef CONFIG_PM_DISABLE_FREQLOCK_COUNT
  g_file_open_lock.count = 0;
#endif
  g_file_open_lock.info = PM_CPUFREQLOCK_TAG('F', 'O', 0);
  g_file_open_lock.flag = PM_CPUFREQLOCK_FLAG_HV;
}

/*--------------------------------------------------------------------------*/
//static void player_freq_lock(uint32_t mode, bool is_lock)
static void player_freq_lock(uint32_t mode)
{
  g_player_lock.flag = mode;
  up_pm_acquire_freqlock(&g_player_lock);
}
/*--------------------------------------------------------------------------*/
static void player_freq_release(void)
{
  up_pm_release_freqlock(&g_player_lock);
}

/*--------------------------------------------------------------------------*/
static int play_file_open(char* file_path, int32_t* file_size)
{
  up_pm_acquire_freqlock(&g_file_open_lock);
  int fd = open(file_path, O_RDONLY);

  *file_size = 0;
  if (fd >= 0)
    {
      struct stat stat_buf;
      if (stat(file_path, &stat_buf) == OK)
        {
          *file_size = stat_buf.st_size;
        }
    }
  up_pm_release_freqlock(&g_file_open_lock);

  return fd;
}

/*--------------------------------------------------------------------------*/
static bool player_start(PlayerEventParam* param)
{
  Track track;
  int fd;

  /* Frec Lock
   * MP3 : HV
   * AAC : LV
   * SBC : HV
   */

   player_freq_lock(PM_CPUFREQLOCK_FLAG_HV);  /* TODO: Currently HV only */

  /* Get next track */
#ifdef CONFIG_AUDIOUTILS_PLAYLIST
  if (app_get_next_track(&track))
    {
      printf("No more tracks to play.\n");
      player_freq_release();
      return false;
    }
#else
  if (app_parse_track_info(&track, s_track_info))
    {
      player_freq_release();
      return false;
    }
#endif /* CONFIG_AUDIOUTILS_PLAYLIST */

  char full_path[128];
  snprintf(full_path, sizeof(full_path), "%s/%s", CONFIG_TEST_AUDIO_PLAYER_FILE_MOUNTPT, track.title);

  fd = play_file_open(full_path, &m_main_file_size);
  if (fd < 0)
    {
      printf("%s open error. check paths and files!\n", full_path);
      player_freq_release();
      return false;
    }
  if (m_main_file_size == 0)
    {
      close(fd);
      player_freq_release();
      printf("%s file size is abnormal. check files!\n",full_path);
      return false;
    }

  printf("Play file: %s\n",track.title);

  p_player->fd = fd;

  p_player->state = PlayState;

  /* Push data to simple fifo */

  if (app_first_push_simple_fifo(p_player->fd) == 1)
    {
      CMN_SimpleFifoClear(&m_simple_fifo_handle);
      close(fd);
      player_freq_release();
      p_player->state = ReadyState;
      return false;
    }

  /* Init Player */

  if (app_init_player(track.codec_type, track.sampling_rate, track.channel_number, track.bit_length))
    {
      CMN_SimpleFifoClear(&m_simple_fifo_handle);
      close(fd);
      player_freq_release();
      p_player->state = ReadyState;
      return false;
    }

  /* Play Player */

  if (app_play_player())
    {
      CMN_SimpleFifoClear(&m_simple_fifo_handle);
      close(fd);
      player_freq_release();
      p_player->state = ReadyState;
      return false;
    }

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_stop(PlayerEventParam* param)

{
  /* Stop Player */

  if (app_stop_player(p_player->fd, AS_STOPPLAYER_NORMAL))
    {
      return false;
    }

  p_player->state = ReadyState;

  /* Frec Release */
  player_freq_release();

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_pause(PlayerEventParam* param)
{
  /* Pause Player */

  if (app_pause_player())
    {
      return false;
    }

  /* Frec Release */
  player_freq_release();

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_release(PlayerEventParam* param)
{

  /* Frec Lock 
	MP3 : HV
	AAC : LV
	SBC : HV
   */
  player_freq_lock(PM_CPUFREQLOCK_FLAG_HV);  /* TODO: Currently HV only */

  /* Pause release */

  if (app_play_player())
    {
      return false;
    }

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_substart(PlayerEventParam* param)
{
  player_freq_lock(PM_CPUFREQLOCK_FLAG_HV);  /* TODO: Currently HV only */

  if (app_init_subplayer())
    {
      player_freq_release();
      return false;
    }
  if (app_play_subplayer())
    {
      player_freq_release();
      return false;
    }

  p_player->subplayer_state = PlayState;

  return true;
}

static bool player_substop(PlayerEventParam* param)
{
  if (app_stop_subplayer())
    {
      return false;
    }

  p_player->subplayer_state = ReadyState;

  /* Frec Release */
  player_freq_release();

  return true;
}

/*--------------------------------------------------------------------------*/

static bool player_incvol(PlayerEventParam* param)
{
  /* Increment volume */

  if (p_player->output_device == AS_SETPLAYER_OUTPUTDEVICE_SPHP)
    {
      if (app_set_incVol(param->step, &p_player->audio_codec_dsp_vol))
        {
          return false;
        }
    }
  else
    {
      printf("Do not support request for I2S output device\n");
    }

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_decvol(PlayerEventParam* param)
{
  if (p_player->output_device == AS_SETPLAYER_OUTPUTDEVICE_SPHP)
    {
      if (app_set_decVol(param->step, &p_player->audio_codec_dsp_vol))
        {
          return false;
        }
    }
  else
    {
      printf("Do not support request for I2S output device\n");
    }

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_clkrcv(PlayerEventParam* param)
{
  if (param->recovery_val == 0)
    {
      return false;
    }

  int dir = (param->recovery_val > 0) ? 1 : -1;
  uint32_t times = (param->recovery_val > 0) ? param->recovery_val : param->recovery_val * -1;

  app_adjust_clkrecovery(dir, times);

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_setgain(PlayerEventParam* param)
{
  if ((param->gain.l_gain < 0 || 200 < param->gain.l_gain) ||
      (param->gain.r_gain < 0 || 200 < param->gain.r_gain))
    {
      return false;
    }

  app_set_lr_gain(param->gain.l_gain, param->gain.r_gain);

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_push_es(PlayerEventParam* param)
{
  /* Push ES data to Simple fifo */

  int ret = app_push_simple_fifo(p_player->fd);

  if (ret == 1)
    {
      return false;
    }
  else if (ret == 2)
    {
      if (!MsgProcTbl[StartEvent][p_player->state](param))
        {
          return false;
        }
    }

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_next(PlayerEventParam* param)
{
  if (CMN_SimpleFifoGetOccupiedSize(&m_simple_fifo_handle) == 0)
    {
      if (!MsgProcTbl[StopEvent][p_player->state](param))
        {
          return false;
        }
      if (!MsgProcTbl[StartEvent][p_player->state](param))
        {
          return false;
        }
    }

  return true;
}

/*--------------------------------------------------------------------------*/
static bool player_poweroff(PlayerEventParam* param)
{
	if ((p_player->state == PlayState)||(p_player->state == EofState))
    {
      app_stop_player(p_player->fd, AS_STOPPLAYER_NORMAL);
      p_player->state = ReadyState;
    }

   if (p_player->subplayer_state == PlayState)
    {
      app_stop_subplayer();
      p_player->subplayer_state = ReadyState;
    }
   while (up_pm_get_freqlock_count(&g_player_lock) > 0)
    {
      player_freq_release();
    }

  /* Set Ready Status */

  if (p_player->state != BootedState)
    {
      int ret = 0;
      ret = board_external_amp_mute_control(true);
      if (ret)
        {
          printf("Amp mute on error. %d\n", ret);
          return 1;
        }

      if (app_set_ready_status())
        {
          return 1;
        }
    }

  /* Power Off */

  if (app_power_off())
    {
      return 1;
    }

  closedir(p_player->dirp);

  return true;
}


/****************************************************************************
 * player_daemon
 ****************************************************************************/
static int player_loop(int argc,  const char* argv[])
{
  PlayerEventCmd cmd;

  if (app_create_context(&p_player))
    {
      return 1;
    }

  if (app_create_dir())
    {
      return 1;
    }

  if (app_power_on())
    {
      return 1;
    }

#ifdef CONFIG_AUDIOUTILS_PLAYLIST
  if (app_open_playlist())
    {
      return 1;
    }
#endif /* CONFIG_AUDIOUTILS_PLAYLIST */

  /* open message queue of receiving internal command */

  struct mq_attr que_attr;
  
  que_attr.mq_maxmsg  = 3;
  que_attr.mq_msgsize = sizeof(PlayerEventCmd);
  que_attr.mq_flags   = 0;

  mqd_t mq_internal_cmd = mq_open("player_internal_cmd_que", O_RDWR | O_CREAT | O_NONBLOCK, 0666, &que_attr);
  
  while (true)
    {
      usleep(10 * 1000);

      /* receive command from CUI */

      if (mq_receive(s_mq_cui_cmd, (char*)&cmd, sizeof(cmd), NULL) != -1)
        {
          if (player_parse(cmd.event, &cmd.param))
        	{
              if (cmd.event == PowerOffEvent)
                {
                  sub_task_suspend = true;
                  break;
                }
        	}
          else
        	{
        	  printf("Command execution error : event[%d]\n",cmd.event);
        	}
        }

      /* receive command from internal function */

      if (mq_receive(mq_internal_cmd, (char*)&cmd, sizeof(cmd), NULL) != -1)
        {
          player_parse(cmd.event, &cmd.param);
        }

      /* check if simplefifo is available */
      
      if (CMN_SimpleFifoGetVacantSize(&m_simple_fifo_handle) > WRITE_SIMPLE_FIFO_SIZE)
        {
          cmd.event = RenderDoneEvent;
          cmd.param.data = 0;

          if (mq_send(mq_internal_cmd, (const char*)&cmd, sizeof(cmd), 0) == -1)
            {
              printf("Failed to post the message. errno = %d\n", errno);
            }
        }
    }

  mq_close(mq_internal_cmd);
  mq_unlink("player_internal_cmd_que");

#ifdef CONFIG_AUDIOUTILS_PLAYLIST
  app_close_playlist();
#endif /* CONFIG_AUDIOUTILS_PLAYLIST */

  app_delete_context(p_player);

  printf("Exit task(player_loop).\n");

  exit(1);

  return 0;
}

static int sub_loop(int argc, char* argv[])
{
  char sub_full_path[128];
  snprintf(sub_full_path, sizeof(sub_full_path), "%s/subsound.mp3", CONFIG_TEST_AUDIO_PLAYER_FILE_MOUNTPT);

  int fd = play_file_open(sub_full_path, &m_sub_file_size);

  if (fd < 0)
    {
      printf("%s open error. check paths and files!\n", sub_full_path);
      return false;
    }
  if (m_sub_file_size == 0)
    {
      close(fd);
      printf("%s file size is abnormal. check files!\n",sub_full_path);
      return false;
    }

  app_init_simple_fifo(&m_simple_fifo_handle_sub, m_simple_fifo_buf_sub, SIMPLE_FIFO_BUF_SUB_SIZE, &m_cur_input_device_handler_sub);

  PlayerEventCmd cmd;
  int32_t ret;
  size_t vacant_size;

  while (true)
    {
      usleep(10 * 1000);
      if (mq_receive(s_mq_cui_cmd_sub, (char*)&cmd, sizeof(cmd), NULL) != -1)
        {
          if (!player_parse(cmd.event, &cmd.param))
          {
            printf("Command execution error : event[%d]\n",cmd.event);
          }
        }

    vacant_size = CMN_SimpleFifoGetVacantSize(&m_simple_fifo_handle_sub);

    if (vacant_size > WRITE_SIMPLE_FIFO_SIZE)
      {
        int cnt=1;
        if(vacant_size > WRITE_SIMPLE_FIFO_SIZE*3) {
          cnt=3;
        }
        else if (vacant_size > WRITE_SIMPLE_FIFO_SIZE*2) {
          cnt=2;
        }
        for (int i=0; i < cnt; i++)
          {
            ret = read(fd, &m_ram_buf_sub, WRITE_SIMPLE_FIFO_SIZE);
            if (ret < 0)
              {
                close(fd);
                printf("Fail to read file\n");
                return 1;
              }

            if (CMN_SimpleFifoOffer(&m_simple_fifo_handle_sub, (const void*)(m_ram_buf_sub), ret) == 0)
              {
                printf("Simple FIFO is full!\n");
                break;
              }
            m_sub_file_size = (m_sub_file_size - ret);
            if (m_sub_file_size == 0)
              {
                close(fd);
                fd = play_file_open(sub_full_path, &m_sub_file_size);
                break;
              }
          }
      }

      if (sub_task_suspend)
        {
          sub_task_suspend = false;
          break;
        }
    }

  CMN_SimpleFifoClear(&m_simple_fifo_handle_sub);
  close(fd);

  printf("Exit task(sub_loop).\n");

  exit(1);

  return 0;
}

static void init_libraries(void)
{
  int ret;
  uint32_t addr = AUD_SRAM_ADDR;

  ret = mpshm_init(&s_shm, 1, 1024 * 128 * 2);
  if (ret < 0)
    {
  	  printf("mpshm_init() failure. %d\n", ret);
  	  return;
    }
  ret = mpshm_remap(&s_shm, (void *)addr);
  if (ret < 0)
    {
      printf("mpshm_remap() failure. %d\n", ret);
      return;
    }
  
  /* Initalize MessageLib */
  MsgLib::initFirst(NUM_MSGQ_POOLS,MSGQ_TOP_DRM);
  MsgLib::initPerCpu();

  void* mml_data_area = MemMgrLite::translatePoolAddrToVa(MEMMGR_DATA_AREA_ADDR);
  MemMgrLite::Manager::initFirst(mml_data_area, MEMMGR_DATA_AREA_SIZE);
  MemMgrLite::Manager::initPerCpu(mml_data_area, NUM_MEM_POOLS);

  /* Create static memory pool of Layout 0 */
  const MemMgrLite::NumLayout layout_no = MEM_LAYOUT_PLAYER;
  void* work_va = MemMgrLite::translatePoolAddrToVa(MEMMGR_WORK_AREA_ADDR);
  D_ASSERT(layout_no < NUM_MEM_LAYOUTS);
  MemMgrLite::Manager::createStaticPools(layout_no, work_va, MEMMGR_MAX_WORK_SIZE, MemMgrLite::MemoryPoolLayouts[layout_no]);

}

static int finalize_libraries(void)
{
  /* Finalize MessageLib */
  MsgLib::finalize();
  
  /* Destroy static pools. */
  MemMgrLite::Manager::destroyStaticPools();

  /* Finalize memory manager */
  MemMgrLite::Manager::finalize();

  /* Destroy MP shared memory. */
  int ret;
  ret = mpshm_detach(&s_shm);
  if (ret < 0)
    {
      printf("mpshm_detach() failure. %d\n", ret);
    }

  ret = mpshm_destroy(&s_shm);
  if (ret < 0)
    {
      printf("mpshm_destroy() failure. %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
static int audioPlayer_createTask(void)
{
  bool create_rst;

  init_libraries();

  init_freq_lock();

  AudioSubSystemIDs ids;

  ids.app = MSGQ_AUD_APP;
  ids.mng = MSGQ_AUD_MGR;
  ids.player_main = MSGQ_AUD_PLY;
  ids.player_sub = MSGQ_AUD_SUB_PLY;
  ids.mixer = MSGQ_AUD_OUTPUT_MIX;
  ids.recorder = 0xFF;
  ids.effector = 0xFF;
  ids.recognizer = 0xFF;

  AS_CreateAudioManager(ids, app_attention_callback);

  AsCreatePlayerParam_t player_create_param;
  player_create_param.msgq_id.player = MSGQ_AUD_PLY;
  player_create_param.msgq_id.mng    = MSGQ_AUD_MGR;
  player_create_param.msgq_id.mixer  = MSGQ_AUD_OUTPUT_MIX;
  player_create_param.msgq_id.dsp    = MSGQ_AUD_DSP;
  player_create_param.pool_id.es     = DEC_ES_MAIN_BUF_POOL;
  player_create_param.pool_id.pcm    = REND_PCM_BUF_POOL;
  player_create_param.pool_id.dsp    = DEC_APU_CMD_POOL;
  player_create_param.pool_id.src_work = SRC_WORK_BUF_POOL;

  create_rst = AS_CreatePlayerMulti(AS_PLAYER_ID_0, &player_create_param, NULL);
  if (!create_rst)
    {
      printf("AS_CreatePlayer failed. system memory insufficient!\n");
      return 1;
    }

  AsCreatePlayerParam_t sub_player_create_param;
  sub_player_create_param.msgq_id.player = MSGQ_AUD_SUB_PLY;
  sub_player_create_param.msgq_id.mng    = MSGQ_AUD_MGR;
  sub_player_create_param.msgq_id.mixer  = MSGQ_AUD_OUTPUT_MIX;
  sub_player_create_param.msgq_id.dsp    = MSGQ_AUD_DSP;
  sub_player_create_param.pool_id.es     = DEC_ES_SUB_BUF_POOL;
  sub_player_create_param.pool_id.pcm    = REND_PCM_SUB_BUF_POOL;
  sub_player_create_param.pool_id.dsp    = DEC_APU_CMD_POOL;
  sub_player_create_param.pool_id.src_work = SRC_WORK_SUB_BUF_POOL;

  create_rst = AS_CreatePlayerMulti(AS_PLAYER_ID_1, &sub_player_create_param, NULL);
  if (!create_rst)
    {
      printf("AS_CreatePlayer failed. system memory insufficient!\n");
      return 1;
    }

  AsCreateOutputMixParam_t output_mix_create_param;
  output_mix_create_param.msgq_id.mixer = MSGQ_AUD_OUTPUT_MIX;
  output_mix_create_param.msgq_id.render_path0_filter_dsp = MSGQ_AUD_PFDSP0;
  output_mix_create_param.msgq_id.render_path1_filter_dsp = MSGQ_AUD_PFDSP1;
  output_mix_create_param.pool_id.render_path0_filter_pcm = PF0_PCM_BUF_POOL;
  output_mix_create_param.pool_id.render_path1_filter_pcm = PF1_PCM_BUF_POOL;
  output_mix_create_param.pool_id.render_path0_filter_dsp = PF0_APU_CMD_POOL;
  output_mix_create_param.pool_id.render_path1_filter_dsp = PF1_APU_CMD_POOL;

  create_rst = AS_CreateOutputMixer(&output_mix_create_param, NULL);
  if (!create_rst)
    {
      printf("AS_CreateOutputMixer failed. system memory insufficient!\n");
      return 1;
    }

  AsCreateRendererParam_t renderer_create_param;
  renderer_create_param.msgq_id.dev0_req  = MSGQ_AUD_RND_PLY;
  renderer_create_param.msgq_id.dev0_sync = MSGQ_AUD_RND_PLY_SYNC;

  renderer_create_param.msgq_id.dev1_req  = MSGQ_AUD_RND_SUB;
  renderer_create_param.msgq_id.dev1_sync = MSGQ_AUD_RND_SUB_SYNC;

  create_rst = AS_CreateRenderer(&renderer_create_param);
  if (!create_rst)
    {
      printf("AS_CreateRenderer failed. system memory insufficient!\n");
      return 1;
    }

  s_player_daemon_pid = task_create("PLAYER_DMON",  AUDIO_TASK_PLAYER_DAEMON_PRIORITY,  AUDIO_TASK_PLAYER_DAEMON_STACK_SIZE, (main_t)player_loop, NULL);
  if (s_player_daemon_pid < 0)
    {
      printf("task_create(PLAYER_DMON) failed. system memory insufficient!\n");
      return 1;
    }

  s_sub_daemon_pid = task_create("SUB_DMON",  AUDIO_TASK_PLAYER_DAEMON_PRIORITY,  AUDIO_TASK_PLAYER_DAEMON_STACK_SIZE, (main_t)sub_loop, NULL);
  if (s_sub_daemon_pid < 0)
    {
      printf("task_create(SUB_DMON) failed. system memory insufficient!\n");
      return 1;
    }

  return 0;
}

/*--------------------------------------------------------------------------*/
static int audioPlayer_deleteTask(void)
{

  /* deactivate Audio Utilities */

  AS_DeletePlayer(AS_PLAYER_ID_0);
  AS_DeletePlayer(AS_PLAYER_ID_1);
  AS_DeleteOutputMix();
  AS_DeleteRenderer();
  AS_DeleteAudioManager();

  /* PLAYER_DMON task will automatically exit task.
   * So, here is nothing to do.
   */

  return 0;
}

static int audioPlayer_subplay(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = SubStartEvent;
  cmd.param.data = 0;

  mq_send(mqd_sub, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

static int audioPlayer_substop(char* parg, mqd_t mqd, mqd_t mqd_sub)
{
  PlayerEventCmd cmd;

  cmd.event = SubStopEvent;
  cmd.param.data = 0;

  mq_send(mqd_sub, (const char*)&cmd, sizeof(cmd), 0);

  return 0;
}

/****************************************************************************
 * application_main
 ****************************************************************************/
#ifdef CONFIG_BUILD_KERNEL
extern "C" int main(int argc, FAR char *argv[])
#else
extern "C" int player_main(int argc, char *argv[])
#endif
{
  int   len, x, running;
  char  buffer[256];
  char  *cmd, *arg;
  struct mq_attr que_attr;
  int  status;

  /* open message queue of receiving CUI command */

  que_attr.mq_maxmsg  = 10;
  que_attr.mq_msgsize = sizeof(PlayerEventCmd);
  que_attr.mq_flags   = 0;
  s_mq_cui_cmd = mq_open("player_cui_cmd_que", O_RDWR | O_CREAT | O_NONBLOCK, 0666, &que_attr);
  if (s_mq_cui_cmd == 0)
    {
      printf("mq_open() failure.\n");
      return 1;
    }
  s_mq_cui_cmd_sub = mq_open("player_cui_cmd_sub_que", O_RDWR | O_CREAT | O_NONBLOCK, 0666, &que_attr);
  if (s_mq_cui_cmd_sub == 0)
    {
      printf("mq_open() failure.\n");
      return 1;
    }

  if (audioPlayer_createTask())
    {
      return 1;
    }

  printf("\nh for commands, q to exit\n");

  running = TRUE;
  while(running)
    {
      printf("player> ");
      fflush(stdout);

      len = readline(buffer, sizeof(buffer), stdin, stdout);
      buffer[len] = '\0';
      if (len > 0)
        {
          if (buffer[len-1] == '\n')
            {
              buffer[len-1] = '\0';
            }
          cmd = strtok_r(buffer, " \n", &arg);
          if (cmd == NULL)
            {
              continue;
            }
          while (*arg == ' ')
            {
              arg++;
            }
          for (x = 0; x < player_cmd_count; x++)
            {
              if (strcmp(cmd, player_cmds[x].cmd) == 0)
                {
                  if (player_cmds[x].pFunc != NULL)
                    {
                      player_cmds[x].pFunc(arg, s_mq_cui_cmd, s_mq_cui_cmd_sub);
                    }
                  if (player_cmds[x].pFunc == audioPlayer_powerOff)
                    {
                      running = FALSE;
                    }
                  break;
                }
            }

          if (x == player_cmd_count)
            {
              printf("%s:  unknown player command\n", buffer);
            }
        }
    }

  waitpid(s_player_daemon_pid, &status, 0);
  if (!WIFEXITED(status))
    {
      printf("PLAYER_DMON abnormal termination.\n");
    }
  waitpid(s_sub_daemon_pid, &status, 0);
  if (!WIFEXITED(status))
    {
      printf("SUB_DMON abnormal termination.\n");
    }

  mq_close(s_mq_cui_cmd);
  mq_close(s_mq_cui_cmd_sub);
  mq_unlink("player_cui_cmd_que");

  audioPlayer_deleteTask();

  finalize_libraries();

  printf("player exit!\n");
  return 0;
}
