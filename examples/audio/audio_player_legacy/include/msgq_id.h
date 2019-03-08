/****************************************************************************
 * msgq_id.h
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
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
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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

#ifndef MSGQ_ID_H_INCLUDED
#define MSGQ_ID_H_INCLUDED

/* Message area size: 4336 bytes */
#define MSGQ_TOP_DRM	0xfc000
#define MSGQ_END_DRM	0xfd0f0

/* Message area fill value after message poped */
#define MSG_FILL_VALUE_AFTER_POP	0x0

/* Message parameter type match check */
#define MSG_PARAM_TYPE_MATCH_CHECK	false

/* Message queue pool IDs */
#define MSGQ_NULL	0
#define MSGQ_AUD_MGR	1
#define MSGQ_AUD_APP	2
#define MSGQ_AUD_DSP	3
#define MSGQ_AUD_PFDSP0	4
#define MSGQ_AUD_PFDSP1	5
#define MSGQ_AUD_PLY	6
#define MSGQ_AUD_SUB_PLY	7
#define MSGQ_AUD_OUTPUT_MIX	8
#define MSGQ_AUD_RND_PLY	9
#define MSGQ_AUD_RND_PLY_SYNC	10
#define MSGQ_AUD_RND_SUB	11
#define MSGQ_AUD_RND_SUB_SYNC	12
#define NUM_MSGQ_POOLS	13

/* User defined constants */

/************************************************************************/
#define MSGQ_AUD_MGR_QUE_BLOCK_DRM	0xfc044
#define MSGQ_AUD_MGR_N_QUE_DRM	0xfc374
#define MSGQ_AUD_MGR_N_SIZE	88
#define MSGQ_AUD_MGR_N_NUM	10
#define MSGQ_AUD_MGR_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_MGR_H_SIZE	0
#define MSGQ_AUD_MGR_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_APP_QUE_BLOCK_DRM	0xfc088
#define MSGQ_AUD_APP_N_QUE_DRM	0xfc6e4
#define MSGQ_AUD_APP_N_SIZE	64
#define MSGQ_AUD_APP_N_NUM	2
#define MSGQ_AUD_APP_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_APP_H_SIZE	0
#define MSGQ_AUD_APP_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_DSP_QUE_BLOCK_DRM	0xfc0cc
#define MSGQ_AUD_DSP_N_QUE_DRM	0xfc764
#define MSGQ_AUD_DSP_N_SIZE	20
#define MSGQ_AUD_DSP_N_NUM	5
#define MSGQ_AUD_DSP_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_DSP_H_SIZE	0
#define MSGQ_AUD_DSP_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_PFDSP0_QUE_BLOCK_DRM	0xfc110
#define MSGQ_AUD_PFDSP0_N_QUE_DRM	0xfc7c8
#define MSGQ_AUD_PFDSP0_N_SIZE	20
#define MSGQ_AUD_PFDSP0_N_NUM	5
#define MSGQ_AUD_PFDSP0_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_PFDSP0_H_SIZE	0
#define MSGQ_AUD_PFDSP0_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_PFDSP1_QUE_BLOCK_DRM	0xfc154
#define MSGQ_AUD_PFDSP1_N_QUE_DRM	0xfc82c
#define MSGQ_AUD_PFDSP1_N_SIZE	20
#define MSGQ_AUD_PFDSP1_N_NUM	5
#define MSGQ_AUD_PFDSP1_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_PFDSP1_H_SIZE	0
#define MSGQ_AUD_PFDSP1_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_PLY_QUE_BLOCK_DRM	0xfc198
#define MSGQ_AUD_PLY_N_QUE_DRM	0xfc890
#define MSGQ_AUD_PLY_N_SIZE	48
#define MSGQ_AUD_PLY_N_NUM	5
#define MSGQ_AUD_PLY_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_PLY_H_SIZE	0
#define MSGQ_AUD_PLY_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_SUB_PLY_QUE_BLOCK_DRM	0xfc1dc
#define MSGQ_AUD_SUB_PLY_N_QUE_DRM	0xfc980
#define MSGQ_AUD_SUB_PLY_N_SIZE	48
#define MSGQ_AUD_SUB_PLY_N_NUM	5
#define MSGQ_AUD_SUB_PLY_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_SUB_PLY_H_SIZE	0
#define MSGQ_AUD_SUB_PLY_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_OUTPUT_MIX_QUE_BLOCK_DRM	0xfc220
#define MSGQ_AUD_OUTPUT_MIX_N_QUE_DRM	0xfca70
#define MSGQ_AUD_OUTPUT_MIX_N_SIZE	48
#define MSGQ_AUD_OUTPUT_MIX_N_NUM	8
#define MSGQ_AUD_OUTPUT_MIX_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_OUTPUT_MIX_H_SIZE	0
#define MSGQ_AUD_OUTPUT_MIX_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_RND_PLY_QUE_BLOCK_DRM	0xfc264
#define MSGQ_AUD_RND_PLY_N_QUE_DRM	0xfcbf0
#define MSGQ_AUD_RND_PLY_N_SIZE	32
#define MSGQ_AUD_RND_PLY_N_NUM	16
#define MSGQ_AUD_RND_PLY_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_RND_PLY_H_SIZE	0
#define MSGQ_AUD_RND_PLY_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_RND_PLY_SYNC_QUE_BLOCK_DRM	0xfc2a8
#define MSGQ_AUD_RND_PLY_SYNC_N_QUE_DRM	0xfcdf0
#define MSGQ_AUD_RND_PLY_SYNC_N_SIZE	16
#define MSGQ_AUD_RND_PLY_SYNC_N_NUM	8
#define MSGQ_AUD_RND_PLY_SYNC_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_RND_PLY_SYNC_H_SIZE	0
#define MSGQ_AUD_RND_PLY_SYNC_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_RND_SUB_QUE_BLOCK_DRM	0xfc2ec
#define MSGQ_AUD_RND_SUB_N_QUE_DRM	0xfce70
#define MSGQ_AUD_RND_SUB_N_SIZE	32
#define MSGQ_AUD_RND_SUB_N_NUM	16
#define MSGQ_AUD_RND_SUB_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_RND_SUB_H_SIZE	0
#define MSGQ_AUD_RND_SUB_H_NUM	0
/************************************************************************/
#define MSGQ_AUD_RND_SUB_SYNC_QUE_BLOCK_DRM	0xfc330
#define MSGQ_AUD_RND_SUB_SYNC_N_QUE_DRM	0xfd070
#define MSGQ_AUD_RND_SUB_SYNC_N_SIZE	16
#define MSGQ_AUD_RND_SUB_SYNC_N_NUM	8
#define MSGQ_AUD_RND_SUB_SYNC_H_QUE_DRM	0xffffffff
#define MSGQ_AUD_RND_SUB_SYNC_H_SIZE	0
#define MSGQ_AUD_RND_SUB_SYNC_H_NUM	0
#endif /* MSGQ_ID_H_INCLUDED */
