/****************************************************************************
 *  This file is part of PPMs project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 2004, 2006   *
 *  Contents: interface to encoding/decoding routines                       *
 *  Comments: this file can be used as an interface to PPMs module          *
 *  (consisting of Model.cpp) from external program                         *
 ****************************************************************************/
#if !defined(_PPMD_H_)
#define _PPMD_H_

#include "PPMdType.h"

#ifdef  __cplusplus
extern "C" {
#endif

void _STDCALL EncodeFile(_PPMD_FILE* EncodedFile,_PPMD_FILE* DecodedFile,int MaxOrder);
void _STDCALL DecodeFile(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile,int MaxOrder);

/*  imported function                                                       */
void _STDCALL  PrintInfo(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile);

#ifdef  __cplusplus
}
#endif

#endif /* !defined(_PPMD_H_) */
