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
// Purpose: USC wrapper functions file.
//
////////////////////////////////////////////////////////////////////////*/
#include <stdio.h>
#include <stdlib.h>
#include "usc.h"
#include "util.h"
#include "usccodec.h"
#include "ipps.h"

#define FRAME_HEADER_SIZE 6
#define MAX_STRING_NAME 1024

static void PrintErrorUSCStatus(FILE *flog, USC_Status status, char *USCfuncName)
{
   if(USCfuncName) {
      USC_OutputString(flog, "%s: ",USCfuncName);
   }
   if(status==USC_BadArgument) {
      USC_OutputString(flog, "Bad argument passed.\n");
   } else if(status==USC_NotInitialized) {
      USC_OutputString(flog, "USC object doesn't initialized.\n");
   } else if(status==USC_InvalidHandler) {
      USC_OutputString(flog, "Invalid USC handler passed.\n");
   } else if(status==USC_UnsupportedPCMType) {
      USC_OutputString(flog, "Unsupported PCM type.\n");
   } else if(status==USC_UnsupportedBitRate) {
      USC_OutputString(flog, "Unsupported bitrate.\n");
   } else if(status==USC_UnsupportedFrameType) {
      USC_OutputString(flog, "Unsupported frametype.\n");
   } else if(status==USC_UnsupportedVADType) {
      USC_OutputString(flog, "Unsupported VAD type.\n");
   } else if(status==USC_BadDataPointer) {
      USC_OutputString(flog, "NULL pointer passed.\n");
   } else if(status==USC_NoOperation) {
      USC_OutputString(flog, "No any operations was done.\n");
   } else {
      USC_OutputString(flog, "Unknown status code.\n");
   }
}

#define USC_CHECK_ERROR_AND_RET(status,logFile,funcName) \
   if(status!=USC_NoError) {\
      PrintErrorUSCStatus(logFile, status, funcName);\
      return -1;\
   }

static Ipp32s checkBitrate(USCParams *uscPrms, Ipp32s bitrate)
{
   Ipp32s i;
   for(i=0;i<uscPrms->pInfo->nRates;i++) {
      if(uscPrms->pInfo->pRateTbl[i].bitrate==bitrate) return 1;
   }
   return 0;
}

#define ABS_DIFF(x,y,res)\
{\
   Ipp32s ABS_DIFF_z = x - y;\
   if(ABS_DIFF_z<0) {\
      res = -ABS_DIFF_z;\
   } else {\
      res = ABS_DIFF_z;\
   }\
}

static Ipp32s GetClosestBitrate(USCParams *uscPrms, Ipp32s bitrate)
{
   Ipp32s i;
   Ipp32s minDelta = IPP_MAX_32S, idx = 0;
   for(i=0;i<uscPrms->pInfo->nRates;i++) {
      Ipp32s delta;
      ABS_DIFF(uscPrms->pInfo->pRateTbl[i].bitrate,bitrate,delta);
      if(delta < minDelta) {
         idx = i;
         minDelta = delta;
      }
   }
   return uscPrms->pInfo->pRateTbl[idx].bitrate;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecAllocInfo
//  Purpose:     Allocates memory for the USC_CodecInfo structure.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parameters.
*/
Ipp32s USCCodecAllocInfo(USCParams *uscPrms, FILE *flog)
{
   USC_Status status;
   Ipp32s size;
   uscPrms->pInfo = NULL;
   status = uscPrms->USC_Fns->std.GetInfoSize(&size);
   USC_CHECK_ERROR_AND_RET(status,flog,"GetInfoSize")

   uscPrms->pInfo = (USC_CodecInfo*)ippsMalloc_8u(size);

   if(uscPrms->pInfo==NULL) return -1;
   return 0;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecGetInfo
//  Purpose:     Retrive info from USC codec.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input USC codec parameters.
*/
Ipp32s USCCodecGetInfo(USCParams *uscPrms, FILE *flog)
{
   USC_Status status;
   status = uscPrms->USC_Fns->std.GetInfo((USC_Handle)NULL, uscPrms->pInfo);
   USC_CHECK_ERROR_AND_RET(status,flog,"GetInfo")
   return 0;
}

Ipp32s SetUSCEncoderPCMType(USCParams *uscPrms, Ipp32s fileComressionType, USC_PCMType *desPCMType, FILE *f_log)
{
   Ipp32s i, isfound=0;
   if(fileComressionType!=LINEAR_PCM) {
      if(desPCMType->bitPerSample != 8) {
         USC_OutputString(f_log, "Bits per sample must be 8 for the A-Law/Mu-Law input file\n");
         return -1;
      }
      for(i=0;i<uscPrms->pInfo->nPcmTypes;i++) {
         if((uscPrms->pInfo->pPcmTypesTbl[i].bitPerSample==16) &&
            (uscPrms->pInfo->pPcmTypesTbl[i].nChannels==desPCMType->nChannels) &&
            (uscPrms->pInfo->pPcmTypesTbl[i].sample_frequency==desPCMType->sample_frequency)) {
               isfound=1;
               uscPrms->pInfo->params.pcmType.bitPerSample = uscPrms->pInfo->pPcmTypesTbl[i].bitPerSample;
               uscPrms->pInfo->params.pcmType.nChannels = uscPrms->pInfo->pPcmTypesTbl[i].nChannels;
               uscPrms->pInfo->params.pcmType.sample_frequency = uscPrms->pInfo->pPcmTypesTbl[i].sample_frequency;
            }
      }
   } else {
      for(i=0;i<uscPrms->pInfo->nPcmTypes;i++) {
         if((uscPrms->pInfo->pPcmTypesTbl[i].bitPerSample==desPCMType->bitPerSample) &&
            (uscPrms->pInfo->pPcmTypesTbl[i].nChannels==desPCMType->nChannels) &&
            (uscPrms->pInfo->pPcmTypesTbl[i].sample_frequency==desPCMType->sample_frequency)) {
               isfound=1;
               uscPrms->pInfo->params.pcmType.bitPerSample = uscPrms->pInfo->pPcmTypesTbl[i].bitPerSample;
               uscPrms->pInfo->params.pcmType.nChannels = uscPrms->pInfo->pPcmTypesTbl[i].nChannels;
               uscPrms->pInfo->params.pcmType.sample_frequency = uscPrms->pInfo->pPcmTypesTbl[i].sample_frequency;
            }
      }
   }
   if(1==isfound) {
      if(uscPrms->pInfo->params.pcmType.nChannels==2) {
         uscPrms->pInfo->params.modes.outMode = USC_OUT_STEREO;
      }
      return 0;
   }
   return -1;
}

int SetUSCDecoderPCMType(USCParams *uscPrms, int fileComressionType, USC_PCMType *desPCMType, FILE *f_log)
{
   uscPrms->pInfo->params.pcmType.bitPerSample = 16;
   uscPrms->pInfo->params.pcmType.nChannels = desPCMType->nChannels;
   uscPrms->pInfo->params.pcmType.sample_frequency = desPCMType->sample_frequency;

   if(uscPrms->pInfo->params.pcmType.nChannels==2) {
      uscPrms->pInfo->params.modes.outMode = USC_OUT_STEREO;
   }

   fileComressionType = fileComressionType;
   f_log = f_log;

   return 0;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        SetUSCEncoderParams
//  Purpose:     Set encode params.
//  Returns:     None.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parameters.
//    cmdParams     pointer to the input parameters from command line.
*/
void SetUSCEncoderParams(USCParams *uscPrms, CommandLineParams *cmdParams)
{
   uscPrms->pInfo->params.direction = USC_ENCODE;
   if(cmdParams->Vad > uscPrms->pInfo->params.modes.vad) {
      printf("WARNING: Unsupported VAD type. Ignored\n");
      uscPrms->pInfo->params.modes.vad = 0;
   } else
      uscPrms->pInfo->params.modes.vad = cmdParams->Vad;

   uscPrms->pInfo->params.law = 0;

   if(checkBitrate(uscPrms, cmdParams->bitrate))
      uscPrms->pInfo->params.modes.bitrate = cmdParams->bitrate;
   else
      printf("WARNING: Unsupported rate. Used default: %d\n",uscPrms->pInfo->params.modes.bitrate);

   return;
}

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        SetUSCDecoderParams
//  Purpose:     Set decode params.
//  Returns:     None.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parameters.
//    cmdParams     pointer to the input parameters from command line.
*/
void SetUSCDecoderParams(USCParams *uscPrms, CommandLineParams *cmdParams)
{
   uscPrms->pInfo->params.direction = USC_DECODE;
   uscPrms->pInfo->params.law = 0;
   if(checkBitrate(uscPrms, cmdParams->bitrate)) {
      uscPrms->pInfo->params.modes.bitrate = cmdParams->bitrate;
   } else {
      uscPrms->pInfo->params.modes.bitrate = GetClosestBitrate(uscPrms, cmdParams->bitrate);
   }
   return;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecAlloc
//  Purpose:     Alloc memory for USC codec.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parameters.
//    cmdParams     pointer to the input parameters from command line.
*/
USC_Status USCCodecAlloc(USCParams *uscPrms, CommandLineParams *cmdParams)
{
   Ipp32s i;
   USC_Status status;
   status = uscPrms->USC_Fns->std.NumAlloc(&uscPrms->pInfo->params, &uscPrms->nbanks);
   if(status!=USC_NoError) return status;

   uscPrms->uCodec.pBanks = (USC_MemBank*)ippsMalloc_8u(sizeof(USC_MemBank)*uscPrms->nbanks);
   if (!uscPrms->uCodec.pBanks)
      return USC_BadDataPointer;
   status = uscPrms->USC_Fns->std.MemAlloc(&uscPrms->pInfo->params, uscPrms->uCodec.pBanks);
   if(status!=USC_NoError) return status;

   for(i=0; i<uscPrms->nbanks;i++){
      uscPrms->uCodec.pBanks[i].pMem = (Ipp8s*)ippsMalloc_8u(uscPrms->uCodec.pBanks[i].nbytes);
   }
   for(i=0; i<uscPrms->nbanks;i++){
      if (!uscPrms->uCodec.pBanks[i].pMem)
         return USC_BadDataPointer;
   }

   cmdParams = cmdParams;
   return USC_NoError;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCEncoderInit
//  Purpose:     Initialize USC encoder object.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parameters.
//    cmdParams     pointer to the input parameters from command line.
*/
Ipp32s USCEncoderInit(USCParams *uscPrms, CommandLineParams *cmdParams, FILE *flog)
{
   USC_Status status;

   status = uscPrms->USC_Fns->std.Init(&uscPrms->pInfo->params, uscPrms->uCodec.pBanks,
                                       &uscPrms->uCodec.hUSCCodec);
   USC_CHECK_ERROR_AND_RET(status,flog,"Init")

   status = uscPrms->USC_Fns->std.GetInfo(uscPrms->uCodec.hUSCCodec, uscPrms->pInfo);
   USC_CHECK_ERROR_AND_RET(status,flog,"GetInfo")

   cmdParams = cmdParams;
   return 0;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCEncoderInit
//  Purpose:     Initialize USC decoder object.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parameters.
//    cmdParams     pointer to the input parameters from command line.
*/
Ipp32s USCDecoderInit(USCParams *uscPrms, CommandLineParams *cmdParams, FILE *flog)
{
   USC_Status status;

   status = uscPrms->USC_Fns->std.Init(&uscPrms->pInfo->params, uscPrms->uCodec.pBanks,
                                             &uscPrms->uCodec.hUSCCodec);
   USC_CHECK_ERROR_AND_RET(status,flog,"Init")

   status = uscPrms->USC_Fns->std.GetInfo(uscPrms->uCodec.hUSCCodec, uscPrms->pInfo);
   USC_CHECK_ERROR_AND_RET(status,flog,"GetInfo")

   cmdParams = cmdParams;
   return 0;
}

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCEncoderPreProcessFrame
//  Purpose:     Pre-process frame to encode.
//  Returns:     lenght of written data to the output stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parameters.
//    pSrc      pointer to the input PCM data.
//    pDst      pointer to the output bitstream data.
//    in        pointer to the output USC_PCMStream structure for the frame.
//    out       pointer to the output USC_Bitstream structure for the frame.
*/
Ipp32s USCEncoderPreProcessFrame(USCParams *uscPrms, Ipp8s *pSrc, Ipp8s *pDst,USC_PCMStream *in,USC_Bitstream *out)
{
   in->bitrate = uscPrms->pInfo->params.modes.bitrate;
   in->nbytes = uscPrms->pInfo->params.framesize;
   in->pBuffer = pSrc;
   in->pcmType.bitPerSample = uscPrms->pInfo->params.pcmType.bitPerSample;
   in->pcmType.nChannels = uscPrms->pInfo->params.pcmType.nChannels;
   in->pcmType.sample_frequency = uscPrms->pInfo->params.pcmType.sample_frequency;
   out->pBuffer = pDst+FRAME_HEADER_SIZE;
   return 0;
}

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCEncoderPostProcessFrame
//  Purpose:     Post-process of encoded frame.
//  Returns:     lenght of written data to the output stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parameters.
//    pSrc      pointer to the input PCM data.
//    pDst      pointer to the output bitstream data.
//    in        pointer to the output USC_PCMStream structure for the frame.
//    out       pointer to the output USC_Bitstream structure for the frame.
*/
Ipp32s USCEncoderPostProcessFrame(USCParams *uscPrms, Ipp8s *pSrc, Ipp8s *pDst,USC_PCMStream *in,USC_Bitstream *out)
{
   Ipp8u tmp;
   tmp = (Ipp8u)(((Ipp32u)out->bitrate)>>16);
   pDst[0] = (Ipp8s)(tmp&0xFF);
   tmp = (Ipp8u)(((Ipp32u)out->bitrate)>>8);
   pDst[1] = (Ipp8s)(tmp&0xFF);
   pDst[2] = (Ipp8s)(out->bitrate&0xFF);
   pDst[3] = (Ipp8s)out->frametype;
   tmp = (Ipp8u)(((Ipp16u)out->nbytes)>>8);
   pDst[4] = (Ipp8s)(tmp&0xFF);
   pDst[5] = (Ipp8s)(out->nbytes&0xFF);

   in = in;
   pSrc = pSrc;
   uscPrms = uscPrms;

   return out->nbytes+FRAME_HEADER_SIZE;
}

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCDecoderPreProcessFrame
//  Purpose:     Pre-process frame to decode.
//  Returns:     lenght of processed data from the input stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parameters.
//    pSrc      pointer to the input bitstream data.
//    pDst      pointer to the output PCM data.
//    in        pointer to the output USC_Bitstream structure for the frame.
//    out       pointer to the output USC_PCMStream structure for the frame.
*/
Ipp32s USCDecoderPreProcessFrame(USCParams *uscPrms, Ipp8s *pSrc, Ipp8s *pDst,USC_Bitstream *in,USC_PCMStream *out)
{
   in->bitrate = (Ipp32s)(((Ipp8u)(pSrc[0])<<16)|((Ipp8u)(pSrc[1])<<8)|(Ipp8u)pSrc[2]);
   in->frametype = (Ipp32s)((Ipp8s)pSrc[3]);
   in->nbytes = (Ipp32s)(((Ipp8u)(pSrc[4])<<8)|(Ipp8u)pSrc[5]);
   in->pBuffer = pSrc+FRAME_HEADER_SIZE;
   out->pBuffer = pDst;

   uscPrms = uscPrms;

   return FRAME_HEADER_SIZE;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCDecoderPostProcessFrame
//  Purpose:     Post-process of decoded frame.
//  Returns:     lenght of processed data from the input stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parameters.
//    pSrc      pointer to the input bitstream data.
//    pDst      pointer to the output PCM data.
//    in        pointer to the output USC_Bitstream structure for the frame.
//    out       pointer to the output USC_PCMStream structure for the frame.
*/
Ipp32s USCDecoderPostProcessFrame(USCParams *uscPrms, Ipp8s *pSrc, Ipp8s *pDst,USC_Bitstream *in,USC_PCMStream *out)
{
   uscPrms = uscPrms;
   pSrc = pSrc;
   pDst = pDst;
   in = in;
   return out->nbytes;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecEncode
//  Purpose:     Encode pre-processed frame.
//  Returns:     lenght of processed data from the input stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parameters.
//    in        pointer to the input USC_Bitstream structure for the frame.
//    out       pointer to the output USC_PCMStream structure for the frame.
*/
Ipp32s USCCodecEncode(USCParams *uscPrms, USC_PCMStream *in,USC_Bitstream *out, FILE *flog)
{
   USC_Status status;

   status = uscPrms->USC_Fns->Encode (uscPrms->uCodec.hUSCCodec, in, out);
   USC_CHECK_ERROR_AND_RET(status,flog,"Encode")

   return in->nbytes;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecDecode
//  Purpose:     Decode pre-processed frame.
//  Returns:     lenght of processed data from the input stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parameters.
//    in        pointer to the input USC_PCMStream structure for the frame.
//    out       pointer to the output USC_Bitstream structure for the frame.
*/
Ipp32s USCCodecDecode(USCParams *uscPrms, USC_Bitstream *in,USC_PCMStream *out, FILE *flog)
{
   USC_Status status;

   status = uscPrms->USC_Fns->Decode (uscPrms->uCodec.hUSCCodec, in, out);
   USC_CHECK_ERROR_AND_RET(status,flog,"Decode")

   return in->nbytes;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecGetSize
//  Purpose:     Get output stream size.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms   pointer to the input USC codec parameters.
//    lenSrc    length of the input stream.
//    lenDst    pointer to the length of the output stream.
*/
Ipp32s USCCodecGetSize(USCParams *uscPrms, Ipp32s lenSrc, Ipp32s *lenDst, FILE *flog)
{
   USC_Status status;
   *lenDst = 0;
   status = uscPrms->USC_Fns->GetOutStreamSize(&uscPrms->pInfo->params, uscPrms->pInfo->params.modes.bitrate, lenSrc, lenDst);
   USC_CHECK_ERROR_AND_RET(status,flog,"GetOutStreamSize")
   return 0;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecCorrectSize
//  Purpose:     Correct output stream size to include frame headers.
//  Returns:     0 if success.
//  Parameters:
//    uscPrms          pointer to the input USC codec parameters.
//    lenSrc           length of the input stream.
//    pSrcDstOutLen    pointer to the corrected length of the output stream.
*/
Ipp32s USCCodecCorrectSize(USCParams *uscPrms, Ipp32s lenSrc, Ipp32s *pSrcDstOutLen)
{
   Ipp32s nFrames;
   if(uscPrms->pInfo->params.direction==USC_ENCODE) {
      nFrames = lenSrc/uscPrms->pInfo->params.framesize;
      /* Add block to hold the last compressed frame*/
      if (lenSrc % uscPrms->pInfo->params.framesize != 0) {
         nFrames++;
      }
      *pSrcDstOutLen += nFrames*FRAME_HEADER_SIZE;
   }

   return 0;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecCorrectSize
//  Purpose:     Get termination condition to organize encode\decode loop.
//  Returns:     0 if success.
//  Parameters:
//    uscPrms          pointer to the input USC codec parameters.
//    pLowBound        pointer to the low bound of the data to process.
//    pSrcDstOutLen    pointer to the corrected length of the output stream.
*/
Ipp32s USCCodecGetTerminationCondition(USCParams *uscPrms, Ipp32s *pLowBound)
{
   if(uscPrms->pInfo->params.direction==USC_ENCODE) {
      /* In encode mode than we only operate under the whole frame*/
      *pLowBound = uscPrms->pInfo->params.framesize-1;
   } else {
      /* In decode mode we estimate the whole encoded frame presents in bitstream*/
      *pLowBound = 0;
   }

   return 0;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCClear
//  Purpose:     Pre-init resources of the USC codec.
//  Returns:     0 if success.
//  Parameters:
//    uscPrms   pointer to the input USC codec parameters.
*/
Ipp32s USCClear(USCParams *uscPrms)
{
   uscPrms->nbanks = 0;
   uscPrms->uCodec.pBanks = NULL;
   uscPrms->pInfo = NULL;

   return 0;
}

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCFree
//  Purpose:     Release resources of the USC codec.
//  Returns:     0 if success.
//  Parameters:
//    uscPrms   pointer to the input USC codec parameters.
*/
Ipp32s USCFree(USCParams *uscPrms)
{
   Ipp32s i;

   for(i=0; i<uscPrms->nbanks;i++){
      ippsFree(uscPrms->uCodec.pBanks[i].pMem);
      uscPrms->uCodec.pBanks[i].pMem = NULL;
   }
   ippsFree(uscPrms->uCodec.pBanks);
   uscPrms->uCodec.pBanks = NULL;
   uscPrms->nbanks = 0;

   if(uscPrms->pInfo) {
      ippsFree(uscPrms->pInfo);
      uscPrms->pInfo = NULL;
   }

   return 0;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCGetFrameHeaderSize
//  Purpose:     Retrieve frame header size.
//  Returns:     frame header size.
//  Parameters:
*/
Ipp32s USCGetFrameHeaderSize()
{
   return FRAME_HEADER_SIZE;
}

Ipp32s USC_CvtToLinear(int framesize, Ipp32s PCMType, Ipp8s **inputBuffer, Ipp32s *pLen)
{
   if (PCMType > LINEAR_PCM) {
      Ipp8u *cvt_buff = NULL;
      Ipp32s lCvtLen = *pLen << 1;

      cvt_buff=ippsMalloc_8u(lCvtLen+framesize);
      if(cvt_buff==NULL){
         return -1;
      }
      /* unpack to linear PCM if compound input */
      if (PCMType==ALAW_PCM) { /* A-Law input */
         ippsALawToLin_8u16s((Ipp8u*)*inputBuffer, (Ipp16s*)cvt_buff, *pLen);
      } else if (PCMType==MULAW_PCM){    /* Mu-Law input */
         ippsMuLawToLin_8u16s((Ipp8u*)*inputBuffer, (Ipp16s*)cvt_buff, *pLen);
      } else {
         return -1;
      }
      ippsFree(*inputBuffer);
      *inputBuffer = (Ipp8s*)cvt_buff;
      *pLen = lCvtLen;
   }

   return 0;
}

Ipp32s USC_CvtToLaw(USCParams *uscPrms, Ipp32s PCMType, Ipp8s *inputBuffer, Ipp32s *pLen)
{
   if (PCMType > LINEAR_PCM) {
      Ipp32s lCvtLen = *pLen >> 1;
      if (PCMType==ALAW_PCM) { /* A-Law output */
         ippsLinToALaw_16s8u((const Ipp16s*) inputBuffer, (Ipp8u*) inputBuffer, lCvtLen);
      } else if (PCMType==MULAW_PCM) {   /* Mu-Law output */
         Ipp16s *tmpBuffer=NULL;
         tmpBuffer = ippsMalloc_16s(lCvtLen);
         if(tmpBuffer==NULL) return -1;
         ippsCopy_16s((Ipp16s*)inputBuffer,tmpBuffer,lCvtLen);
         ippsLinToMuLaw_16s8u(tmpBuffer, (Ipp8u*) inputBuffer, lCvtLen);
         ippsFree(tmpBuffer);
      } else {
         return -1;
      }
      *pLen = lCvtLen;
   }
   uscPrms = uscPrms;
   return 0;
}
