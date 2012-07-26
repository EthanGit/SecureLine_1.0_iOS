/*////////////////////////////////////////////////////////////////////////
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2006-2008 Intel Corporation. All Rights Reserved.
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
// Purpose: Auxiliary functions header file.
//
////////////////////////////////////////////////////////////////////////*/
#ifndef __UTIL_H__
#define __UTIL_H__

#include <ippdefs.h>
#include "vm_file.h"

#define LOAD_CODEC_FAIL     1
#define FOPEN_FAIL          2
#define MEMORY_FAIL         3
#define UNKNOWN_FORMAT      4
#define USC_CALL_FAIL       5


#define LINEAR_PCM        1
#define ALAW_PCM          6
#define MULAW_PCM         7
typedef struct _USCCodec {
   USC_MemBank*        pBanks;
   USC_Handle          hUSCCodec;
}USCCodec;

#define MAX_FILENAME_LEN      256
typedef struct _USCParams {
   USC_CodecInfo       *pInfo;
   USC_Fxns            *USC_Fns;
   Ipp32s              nbanks;
   USCCodec            uCodec;
}USCParams;

typedef struct _CommandLineParams {
   Ipp8s inputFileName[MAX_FILENAME_LEN];
   Ipp8s outputFileName[MAX_FILENAME_LEN];
   Ipp8s logFileName[MAX_FILENAME_LEN];
   Ipp8s csvFileName[MAX_FILENAME_LEN];
   Ipp8s codecName[MAX_FILENAME_LEN];
   Ipp8s TimingCodecName[MAX_FILENAME_LEN];
   Ipp32s  bitrate;
   Ipp32s  Vad;
   Ipp32s  enumerate;
   Ipp32s  optionReport;
   Ipp32s  puttocsv;
   Ipp32s  puttologfile;
   Ipp32s  timing;
   Ipp32s  nRepeat;
   Ipp32s  nOutputPCMType;
}CommandLineParams;

void ReadCommandLine(CommandLineParams *params, Ipp32s argc, Ipp8s *argv[]);
void SetCommandLineByDefault(CommandLineParams *params);
void PrintUsage(Ipp8s *pAppName);
vm_file *OpenCSVFile(Ipp8s *csvFileName);
void CloseCSVFile(vm_file *f_csv);
Ipp32s OutputString2CSVFile(CommandLineParams *params, USC_CodecInfo *pInfo, Ipp32f spSeconds,Ipp32f speed_in_mhz);
Ipp32s OutputDuplexString2CSVFile(CommandLineParams *params, USC_CodecInfo *pInfo, Ipp32f spSeconds,
                               Ipp32f ENCspeed_in_mhz, Ipp32f DECspeed_in_mhz);
Ipp32s OutputSysInfo2CSVFile(vm_file *f_csv);
void AddCodecTableHeader2CSVFile(vm_file *f_csv);
Ipp32s Timing();
void USC_OutputString(FILE *flog, Ipp8s *format,...);
#endif/* __UTIL_H__*/
