/*////////////////////////////////////////////////////////////////////////
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2005-2006 Intel Corporation. All Rights Reserved.
//
//   Intel(R)  Integrated Performance Primitives
//
//     USC speech codec sample
//
// By downloading and installing this sample, you hereby agree that the
// accompanying Materials are being provided to you under the terms and
// conditions of the End User License Agreement for the Intel(R) Integrated
// Performance Primitives product previously accepted by you. Please refer
// to the file ippEULA.rtf or ippEULA.txt located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// Purpose: Wave file reading/writing  functions header file.
//
////////////////////////////////////////////////////////////////////////*/
#ifndef __WAVFILE_H__
#define __WAVFILE_H__

#include "vm_types.h"

enum {
   FILE_READ                = 0x0002,
   FILE_WRITE               = 0x0004
};

typedef struct _WaveFormat {
   Ipp16s nFormatTag;
   Ipp16s nChannels;
   Ipp32s nSamplesPerSec;
   Ipp32s nAvgBytesPerSec;
   Ipp16s nBlockAlign;
   Ipp16s nBitPerSample;
   Ipp32s cbSize;
}WaveFormat;

typedef struct _WaveFileParams {
   Ipp32s isReadMode;
   Ipp32s isFirstTimeAccess;
   void *FileHandle;
   WaveFormat    waveFmt;
   Ipp32u  DataSize;
   Ipp32u  RIFFSize;
   Ipp32u  bitrate;
} WaveFileParams;

Ipp32s OpenWavFile(WaveFileParams *wfParams, Ipp8s* FileName, Ipp32u mode);
void InitWavFile(WaveFileParams *wfParams);
Ipp32s WavFileRead(WaveFileParams *wfParams, void* Data, Ipp32s size);
Ipp32s WavFileWrite(WaveFileParams *wfParams, void* Data, Ipp32s size);
Ipp32s WavFileClose(WaveFileParams *wfParams);
Ipp32s WavFileReadHeader(WaveFileParams *wfParams);
Ipp32s WavFileWriteHeader(WaveFileParams *wfParams);

#endif /*__WAVFILE_H__*/
