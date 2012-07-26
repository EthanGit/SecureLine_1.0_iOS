/*////////////////////////////////////////////////////////////////////////
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2005-2007 Intel Corporation. All Rights Reserved.
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
// Purpose: Speech sample. Main program file.
//
////////////////////////////////////////////////////////////////////////*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ippcore.h"
#include "ipps.h"
#include "ippsc.h"

#include "usc.h"

#include "util.h"
#include "loadcodec.h"
#include "wavfile.h"
#include "usccodec.h"
//AMD
//#include "vm_time.h"
#include "uscreg.h"

#define LOAD_CODEC_FAIL     1
#define FOPEN_FAIL          2
#define MEMORY_FAIL         3
#define UNKNOWN_FORMAT      4
#define USC_CALL_FAIL       5


#define COPYRIGHT_STRING "Copyright (c) 2005-2007 Intel Corporation. All Rights Reserved."

Ipp32s ProcessOneFrameOneChannel(USCParams *uscPrms, Ipp8s *inputBuffer,
                              Ipp8s *outputBuffer, Ipp32s *pSrcLenUsed, Ipp32s *pDstLenUsed, FILE *f_log)
{
   Ipp32s frmlen, infrmLen, FrmDataLen;
   USC_PCMStream PCMStream;
   USC_Bitstream Bitstream;
   if(uscPrms->pInfo->params.direction==USC_ENCODE) {
      /*Do the pre-procession of the frame*/
      infrmLen = USCEncoderPreProcessFrame(uscPrms, inputBuffer,
                                             outputBuffer,&PCMStream,&Bitstream);
      /*Encode one frame*/
      FrmDataLen = USCCodecEncode(uscPrms, &PCMStream,&Bitstream,f_log);
      if(FrmDataLen < 0) return USC_CALL_FAIL;
      infrmLen += FrmDataLen;
      /*Do the post-procession of the frame*/
      frmlen = USCEncoderPostProcessFrame(uscPrms, inputBuffer,
                                             outputBuffer,&PCMStream,&Bitstream);
      *pSrcLenUsed = infrmLen;
      *pDstLenUsed = frmlen;
   } else {
      /*Do the pre-procession of the frame*/
      infrmLen = USCDecoderPreProcessFrame(uscPrms, inputBuffer,
                                             outputBuffer,&Bitstream,&PCMStream);
      /*Decode one frame*/
      FrmDataLen = USCCodecDecode(uscPrms, &Bitstream,&PCMStream,f_log);
      if(FrmDataLen < 0) return USC_CALL_FAIL;
      infrmLen += FrmDataLen;
      /*Do the post-procession of the frame*/
      frmlen = USCDecoderPostProcessFrame(uscPrms, inputBuffer,
                                             outputBuffer,&Bitstream,&PCMStream);
      *pSrcLenUsed = infrmLen;
      *pDstLenUsed = frmlen;
   }
   return 0;
}
#define MAX_LEN(a,b) ((a)>(b))? (a):(b)
Ipp32s ProcessOneChannel(LoadedCodec *codec, WaveFileParams *wfOutputParams, Ipp8s *inputBuffer,
                              Ipp32s inputLen, Ipp8s *outputBuffer, Ipp32s *pDuration, Ipp32s *dstLen, FILE *f_log)
{
   Ipp32s duration=0, outLen = 0, currLen;
   Ipp8s *pInputBuffPtr = inputBuffer;
   Ipp32s frmlen, infrmLen, cvtLen;
   Ipp32s lLowBound;
   currLen = inputLen;

   USCCodecGetTerminationCondition(&codec->uscParams, &lLowBound);

   while(currLen > lLowBound) {
      ProcessOneFrameOneChannel(&codec->uscParams, pInputBuffPtr,
                              outputBuffer, &infrmLen, &frmlen,f_log);
      /* Write encoded data to the output file*/
      cvtLen = frmlen;
      if((wfOutputParams->waveFmt.nFormatTag==ALAW_PCM)||(wfOutputParams->waveFmt.nFormatTag==MULAW_PCM)) {
         USC_CvtToLaw(&codec->uscParams, wfOutputParams->waveFmt.nFormatTag, outputBuffer, &cvtLen);
      }
      WavFileWrite(wfOutputParams, outputBuffer, cvtLen);
      /* Move pointer to the next position*/
      currLen -= infrmLen;
      pInputBuffPtr += infrmLen;
      duration += MAX_LEN(infrmLen,frmlen);
      outLen += frmlen;
   }

   *pDuration = duration;
   *dstLen = outLen;
   return 0;
}
