/* Copyright (C) 2008 STMicroelectronics, MCD */
/*

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef __IAR_SYSTEMS_ICC__     /* IAR Compiler */
#define inline inline
#endif

#ifdef  __CC_ARM               /* ARM Compiler */
#define inline __inline
#endif

// select which speex mode to use.
// options: SPEEX_MODEID_NB, SPEEX_MODEID_WB, SPEEX_MODEID_UWB
#define SIFT_SPEEX_MODE SPEEX_MODEID_WB

//#if (SIFT_SPEEX_MODE == SPEEX_MODEID_NB)
//#define DISABLE_WIDEBAND
//#endif

#define EXPORT 
#define FIXED_POINT
#define DISABLE_FLOAT_API
#define DISABLE_VBR
#define DISABLE_NOTIFICATIONS
#define DISABLE_WARNINGS
#define RELEASE
#define OVERRIDE_SPEEX_PUTC
#define OVERRIDE_SPEEX_FATAL

#define MAX_CHARS_PER_FRAME (20/BYTES_PER_CHAR)

#define SPEEXENC_PERSIST_STACK_SIZE 0
#define SPEEXENC_SCRATCH_STACK_SIZE 0
#define SPEEXDEC_PERSIST_STACK_SIZE 2500
#define SPEEXDEC_SCRATCH_STACK_SIZE 1000

#define SPEEX_PERSIST_STACK_SIZE (SPEEXENC_PERSIST_STACK_SIZE + SPEEXDEC_PERSIST_STACK_SIZE)
#define SPEEX_SCRATCH_STACK_SIZE SPEEXENC_SCRATCH_STACK_SIZE
#define NB_ENC_STACK SPEEXENC_SCRATCH_STACK_SIZE
#define NB_DEC_STACK SPEEXDEC_SCRATCH_STACK_SIZE

#ifdef __cplusplus
extern "C" {
#endif

extern void _speex_fatal(const char *str, const char *file, int line);
extern void _speex_putc(int ch, void *file);

#ifdef __cplusplus
}
#endif

