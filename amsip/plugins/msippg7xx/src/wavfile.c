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
// Purpose: Wave file reading/writing  functions file.
//
////////////////////////////////////////////////////////////////////////*/
#include <stdio.h>
#include <memory.h>
#include "wavfile.h"
#include "usc.h"
#include "util.h"

#if defined (_BIG_ENDIAN)
#define RiffChunkID (Ipp32u)0x52494646
#define WaveChunkID (Ipp32u)0x57415645
#define FmtChunkID  (Ipp32u)0x666D7420
#define FactChunkID (Ipp32u)0x66616374
#define DataChunkID (Ipp32u)0x64617461

#define SWAP_32S(x) (Ipp32u)(((x) << 24) | (((x)&0xff00) << 8) | (((x) >>8)&0xff00) | ((x&0xff000000) >> 24))
#define SWAP_16S(x) (Ipp16u)(((Ipp16u)(x) << 8) | ((Ipp16u)(x) >>8 ))

#else
#define RiffChunkID (Ipp32u)0x46464952
#define WaveChunkID (Ipp32u)0x45564157
#define FmtChunkID  (Ipp32u)0x20746D66
#define FactChunkID (Ipp32u)0x74636166
#define DataChunkID (Ipp32u)0x61746164

#define SWAP_32S(x) x
#define SWAP_16S(x) x

#endif

static void SwapBytes_16u(Ipp16u *pSrc,Ipp32s len)
{
   Ipp32s i;
   for(i=0;i<len;i++) {
      pSrc[i] = (((pSrc[i]) << 8) | ((pSrc[i]) >> 8 ));
   }
   return;
}
typedef struct _WaveChunk
{
   Ipp32u lChunkId;
   Ipp32u lChunkSize;
}WaveChunk;

typedef struct _waveHeader
{
   WaveChunk      riffChunk;
   Ipp32u   waveWord;
   WaveChunk      fmtChunk;
   WaveFormat     fmt;
   WaveChunk      dataChunk;
}waveHeader;

void InitWavFile(WaveFileParams *wfParams)
{
   wfParams->isFirstTimeAccess = 1;
   wfParams->FileHandle = NULL;
   wfParams->DataSize = 0;
   wfParams->RIFFSize = 0;
   wfParams->bitrate = 0;
}

Ipp32s OpenWavFile(WaveFileParams *wfParams, Ipp8s* FileName, Ipp32u mode)
{
   if (mode == FILE_WRITE ) {
      wfParams->FileHandle = (void*)fopen(FileName,"wb");
      wfParams->isReadMode = 0;
   } else {
      wfParams->FileHandle = (void*)fopen(FileName,"rb");
      wfParams->isReadMode = 1;
   }
   if (wfParams->FileHandle == NULL) {
      return -1;
   }
   return 0;
}

Ipp32s WavFileRead(WaveFileParams *wfParams, void *Data, Ipp32s size)
{
   Ipp32s n;
   if (wfParams->FileHandle == NULL) {
      return -1;
   }

   n = (Ipp32s)fread(Data,(size_t)1,(size_t)size,(FILE*)wfParams->FileHandle);
#if defined (_BIG_ENDIAN)
   if((wfParams->waveFmt.nFormatTag==1)&&(wfParams->waveFmt.nBitPerSample==16))
      SwapBytes_16u((Ipp16u *)Data,size>>1);
#endif

   return (Ipp32s)n;
}

Ipp32s WavFileWrite(WaveFileParams *wfParams, void * Data, Ipp32s size)
{
   Ipp32s     n;
   Ipp32s     header_size;

   if (wfParams->isFirstTimeAccess) {
      wfParams->isFirstTimeAccess = 0;

      header_size = sizeof(waveHeader);
      fseek((FILE*)wfParams->FileHandle,header_size,SEEK_SET);

      wfParams->RIFFSize = header_size - 4;
      wfParams->DataSize = 0;
   }

   if (wfParams->FileHandle == NULL) {
      return -1;
   }
#if defined (_BIG_ENDIAN)
   if((wfParams->waveFmt.nFormatTag==1)&&(wfParams->waveFmt.nBitPerSample==16))
      SwapBytes_16u((Ipp16u *)Data,size>>1);
#endif
   n = (Ipp32s)fwrite(Data,1,size,(FILE*)wfParams->FileHandle);

   wfParams->RIFFSize += size;
   wfParams->DataSize += size;

   return n;
}

Ipp32s WavFileClose(WaveFileParams *wfParams)
{
   fclose((FILE*)wfParams->FileHandle);
   wfParams->FileHandle = NULL;
   return 0;
}
Ipp32s WavFileWriteHeader(WaveFileParams *wfParams)
{
   waveHeader wHeader;
   Ipp32s n;

   if(!wfParams->isReadMode) {
      if (wfParams->FileHandle == NULL)  {
         return -1;
      }
      wHeader.riffChunk.lChunkId = RiffChunkID;
      wHeader.riffChunk.lChunkSize = SWAP_32S(wfParams->RIFFSize);

      wHeader.waveWord = WaveChunkID;

      wHeader.fmtChunk.lChunkId = FmtChunkID;
      wHeader.fmtChunk.lChunkSize = SWAP_32S(sizeof(WaveFormat));

      if ((wfParams->waveFmt.nFormatTag == LINEAR_PCM)||
         (wfParams->waveFmt.nFormatTag == ALAW_PCM)||
         (wfParams->waveFmt.nFormatTag == MULAW_PCM)) {
         wHeader.fmt.nAvgBytesPerSec = wfParams->waveFmt.nSamplesPerSec * wfParams->waveFmt.nChannels *
                                                        (wfParams->waveFmt.nBitPerSample >> 3);
      } else {
         wHeader.fmt.nAvgBytesPerSec = ((wfParams->bitrate + 7 ) / 8);
      }
      wHeader.fmt.nFormatTag = SWAP_16S(wfParams->waveFmt.nFormatTag);
      wHeader.fmt.nChannels = SWAP_16S(wfParams->waveFmt.nChannels);
      wHeader.fmt.nSamplesPerSec = SWAP_32S(wfParams->waveFmt.nSamplesPerSec);

      wHeader.fmt.nAvgBytesPerSec=SWAP_32S(wHeader.fmt.nAvgBytesPerSec);
      wHeader.fmt.cbSize = 0;
      wHeader.fmt.nBlockAlign = SWAP_16S(wfParams->waveFmt.nBlockAlign);
      wHeader.fmt.nBitPerSample = SWAP_16S(wfParams->waveFmt.nBitPerSample);

      wHeader.dataChunk.lChunkId = DataChunkID;
      wHeader.dataChunk.lChunkSize = SWAP_32S(wfParams->DataSize);

      fseek((FILE*)wfParams->FileHandle,0,SEEK_SET);
      n = (Ipp32s)fwrite(&wHeader,1,sizeof(wHeader),(FILE*)wfParams->FileHandle);
   }
   return 0;
}
#define WAVE_MIN(a,b) ((a)<(b))? (a) : (b)
Ipp32s WavFileReadHeader(WaveFileParams *wfParams)
{
   waveHeader header = {0,0,0,0,0,0,0};
   WaveChunk xChunk;

   Ipp32u lTmp = 0,lOffset = 0, lDataChunkOffset = 0;
   Ipp32s lFmtChunkOk = 0, lDataChunkOk = 0;

   if (wfParams->FileHandle == NULL)  {
      return -1;
   }

   if(wfParams->isReadMode) {
      if (fread(&xChunk,sizeof(xChunk),1,(FILE*)wfParams->FileHandle) != 1) {
         return -1;
      }
      if ( xChunk.lChunkId != RiffChunkID) {
         /* It isn't RIFF file*/
         return -2;
      }
      if (fread(&lTmp,sizeof(lTmp),1,(FILE*)wfParams->FileHandle) != 1) {
         return -1;
      }
      if (lTmp != WaveChunkID) {
         /* It isn't WAVE file*/
         return -3;
      }

      while(!(lFmtChunkOk && lDataChunkOk)) {
         lOffset = ftell((FILE*)wfParams->FileHandle);

         if (fread(&xChunk,sizeof(xChunk),1,(FILE*)wfParams->FileHandle) != 1) {
            return -1;
         }
         xChunk.lChunkSize=SWAP_32S(xChunk.lChunkSize);
         switch (xChunk.lChunkId) {
            case FmtChunkID:
               if (fread(&header.fmt.nFormatTag,WAVE_MIN(xChunk.lChunkSize,sizeof(WaveFormat)-sizeof(Ipp16s)),
                                          1, (FILE*)wfParams->FileHandle) != 1) {
                  return -1;
               }
               if (xChunk.lChunkSize > (sizeof(WaveFormat)-sizeof(Ipp16s))) {
                  fseek((FILE*)wfParams->FileHandle,xChunk.lChunkSize - (sizeof(WaveFormat)-sizeof(Ipp16s)),
                                                         SEEK_CUR);
               }
               lFmtChunkOk = 1;
               break;
            case DataChunkID:
               lDataChunkOffset = lOffset;
               wfParams->DataSize = xChunk.lChunkSize;
               fseek((FILE*)wfParams->FileHandle,xChunk.lChunkSize,SEEK_CUR);
               lDataChunkOk = 1;
               break;
            default:
               fseek((FILE*)wfParams->FileHandle,xChunk.lChunkSize,SEEK_CUR);
               break;
         }
      }
      fseek((FILE*)wfParams->FileHandle,lDataChunkOffset + 8,SEEK_SET);
      wfParams->waveFmt.nFormatTag  = SWAP_16S(header.fmt.nFormatTag);
      wfParams->waveFmt.nSamplesPerSec = SWAP_32S(header.fmt.nSamplesPerSec);
      wfParams->waveFmt.nBitPerSample  = SWAP_16S(header.fmt.nBitPerSample);
      wfParams->waveFmt.nChannels = SWAP_16S(header.fmt.nChannels);
      lTmp = SWAP_32S(header.fmt.nAvgBytesPerSec);
      wfParams->bitrate = ((lTmp*8)/10)*10;
   }

   return 0;
}
