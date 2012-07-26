/*////////////////////////////////////////////////////////////////////////
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2006-2007 Intel Corporation. All Rights Reserved.
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
// Purpose: USC formats library API functions.
//
////////////////////////////////////////////////////////////////////////*/


#include "uscfmt.h"
#include "formats.h"
#include "uscreg.h"
#include "string.h"

#define FORMAT_INFO_VERSION USC_MAKE_FORMAT_DET_VERSION(1,0)

static Ipp32s GetNumFormats(void);
static Ipp32s GetNameByFormatTag(Ipp32s lFormatTag, Ipp8s *pDstCodecName);
static Ipp32s GetFormatDetailsByFormatTag(Ipp32s lFormatTag, Ipp32s *pFormatDetails);
static Ipp32s GetFormatTagByNameAndByDetails(const Ipp8s *pSrcCodecName, Ipp32s fmtDetails, Ipp32s *lFormatTag);

USCFMTINFO usc_fmt_info_Fxns USC_FORMAT_INFO_FXNS={
   GetNumFormats,
   GetNameByFormatTag,
   GetFormatDetailsByFormatTag,
   GetFormatTagByNameAndByDetails
};

fmtInfo uscFormatsInfo[] = {
    "IPP_AMRWB",      WAVE_FORMAT_IPP_AMRWB, USC_MAKE_FORMAT_DET(0,0,0,1,0),
    "IPP_AMRWB",   WAVE_FORMAT_IPP_AMRWBVAD, USC_MAKE_FORMAT_DET(1,0,0,1,0),
   "IPP_G723.1",       WAVE_FORMAT_IPP_G723, USC_MAKE_FORMAT_DET(0,1,1,0,0),
   "IPP_G723.1",    WAVE_FORMAT_IPP_G723VAD, USC_MAKE_FORMAT_DET(1,1,1,0,0),
     "IPP_G726",       WAVE_FORMAT_IPP_G726, USC_MAKE_FORMAT_DET(0,0,0,0,0),
     "IPP_G728",       WAVE_FORMAT_IPP_G728, USC_MAKE_FORMAT_DET(0,0,1,0,0),
    "IPP_G729I",      WAVE_FORMAT_IPP_G729I, USC_MAKE_FORMAT_DET(0,0,0,0,0),
    "IPP_G729I",   WAVE_FORMAT_IPP_G729IVAD, USC_MAKE_FORMAT_DET(1,0,0,0,0),
    "IPP_G729A",      WAVE_FORMAT_IPP_G729A, USC_MAKE_FORMAT_DET(0,0,0,0,0),
    "IPP_G729A",   WAVE_FORMAT_IPP_G729AVAD, USC_MAKE_FORMAT_DET(1,0,0,0,0),
   "IPP_GSMAMR",        WAVE_FORMAT_IPP_AMR, USC_MAKE_FORMAT_DET(0,0,0,1,0),
   "IPP_GSMAMR",     WAVE_FORMAT_IPP_AMRVAD, USC_MAKE_FORMAT_DET(1,0,0,1,0),
    "IPP_GSMFR",      WAVE_FORMAT_IPP_GSMFR, USC_MAKE_FORMAT_DET(0,0,0,0,0),
    "IPP_GSMFR",   WAVE_FORMAT_IPP_GSMFRVAD, USC_MAKE_FORMAT_DET(1,0,0,0,0),
   "IPP_G722.1",      WAVE_FORMAT_IPP_G7221, USC_MAKE_FORMAT_DET(0,0,0,0,0),
 "IPP_G729I_FP",    WAVE_FORMAT_IPP_G729IFP, USC_MAKE_FORMAT_DET(0,0,0,0,0),
 "IPP_G729I_FP", WAVE_FORMAT_IPP_G729IFPVAD, USC_MAKE_FORMAT_DET(1,0,0,0,0),
 "IPP_G729A_FP",    WAVE_FORMAT_IPP_G729AFP, USC_MAKE_FORMAT_DET(0,0,0,0,0),
 "IPP_G729A_FP", WAVE_FORMAT_IPP_G729AFPVAD, USC_MAKE_FORMAT_DET(1,0,0,0,0),
     "IPP_G722",     WAVE_FORMAT_IPP_G722SB, USC_MAKE_FORMAT_DET(0,1,1,0,0),
    "IPP_G711A",      WAVE_FORMAT_IPP_G711A, USC_MAKE_FORMAT_DET(0,0,1,0,0),
    "IPP_G711A",   WAVE_FORMAT_IPP_G711AVAD, USC_MAKE_FORMAT_DET(1,0,1,0,0),
    "IPP_G711U",      WAVE_FORMAT_IPP_G711U, USC_MAKE_FORMAT_DET(0,0,1,0,0),
    "IPP_G711U",   WAVE_FORMAT_IPP_G711UVAD, USC_MAKE_FORMAT_DET(1,0,1,0,0),
   "IPP_AMRWBE",     WAVE_FORMAT_IPP_AMRWBE, USC_MAKE_FORMAT_DET(0,0,1,0,0),
   "IPP_AMRWBE",  WAVE_FORMAT_IPP_AMRWBEVAD, USC_MAKE_FORMAT_DET(1,0,1,0,0),
   "IPP_G729.1",      WAVE_FORMAT_IPP_G7291, USC_MAKE_FORMAT_DET(0,0,0,0,0),
     "IPP_iLBC",       WAVE_FORMAT_IPP_ILBC, USC_MAKE_FORMAT_DET(0,0,1,0,0),
    "IPP_MSRTA",   WAVE_FORMAT_IPP_MSRTAVAD, USC_MAKE_FORMAT_DET(1,1,0,0,0),
    "IPP_MSRTA",      WAVE_FORMAT_IPP_MSRTA, USC_MAKE_FORMAT_DET(0,1,0,0,0),
 "IPP_MSRTA_FP", WAVE_FORMAT_IPP_MSRTAFPVAD, USC_MAKE_FORMAT_DET(1,1,0,0,0),
 "IPP_MSRTA_FP",    WAVE_FORMAT_IPP_MSRTAFP, USC_MAKE_FORMAT_DET(0,1,0,0,0)
};

uscFormats USC_FORMATS = {
   32,
   uscFormatsInfo
};

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        GetFormatsInfoVersion
//  Purpose:     getting version of the format library.
//  Returns:     number of available formats.
//  Parameters:
//
*/
Ipp32s GetFormatsInfoVersion()
{
   return FORMAT_INFO_VERSION;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        GetNumFormats
//  Purpose:     getting number of available formats.
//  Returns:     number of available formats.
//  Parameters:
//
*/
static Ipp32s GetNumFormats()
{
   return USC_FORMATS.nEntries;
}

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        GetNameByFormatTag
//  Purpose:     getting codec name by format tag.
//  Returns:     1 if success, 0 if fails.
//  Parameters:
//    lFormatTag        input format tag.
//    pDstCodecName     pointer to the output codec name.
*/
static Ipp32s GetNameByFormatTag(Ipp32s lFormatTag, Ipp8s *pDstCodecName)
{
   Ipp32s i;
   for(i=0;i<USC_FORMATS.nEntries;i++) {
      if(USC_FORMATS.formatsInfo[i].lFormatTag==lFormatTag){
         strcpy(pDstCodecName,USC_FORMATS.formatsInfo[i].codecName);
         return 1;
      }
   }
   return 0;
}

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        GetFormatDetailsByFormatTag
//  Purpose:     getting format details in USC_MAKE_FORMAT_DET form by format tag.
//  Returns:     1 if success, 0 if fails.
//  Parameters:
//    lFormatTag        input format tag.
//    pFormatDetails    pointer to the output format details in USC_MAKE_FORMAT_DET form.
*/
static Ipp32s GetFormatDetailsByFormatTag(Ipp32s lFormatTag, Ipp32s *pFormatDetails)
{
   Ipp32s i;
   for(i=0;i<USC_FORMATS.nEntries;i++) {
      if(USC_FORMATS.formatsInfo[i].lFormatTag==lFormatTag){
         *pFormatDetails = USC_FORMATS.formatsInfo[i].fmtDetails;
         return 1;
      }
   }
   return 0;
}

static Ipp32s CompareDetails(Ipp32s SrcDetails, Ipp32s RefDetails)
{
   if((GET_VAD_FROM_FORMAT_DET(SrcDetails)==GET_VAD_FROM_FORMAT_DET(RefDetails)) &&
      (GET_HPF_FROM_FORMAT_DET(SrcDetails)==GET_HPF_FROM_FORMAT_DET(RefDetails)) &&
      (GET_PF_FROM_FORMAT_DET(SrcDetails)==GET_PF_FROM_FORMAT_DET(RefDetails)) &&
      (GET_TRUNC_FROM_FORMAT_DET(SrcDetails)==GET_TRUNC_FROM_FORMAT_DET(RefDetails)) &&
      (GET_LAW_FROM_FORMAT_DET(SrcDetails)==GET_LAW_FROM_FORMAT_DET(RefDetails)))
      return 1;
   return 0;
}
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        GetFormatTagByNameAndByDetails
//  Purpose:     getting format tag by codec name and format details in USC_MAKE_FORMAT_DET.
//  Returns:     1 if success, 0 if fails.
//  Parameters:
//    pSrcCodecName     pointer to the input codec name.
//    fmtDetails        format details in USC_MAKE_FORMAT_DET form.
//    lFormatTag        pointer to the output format tag.
*/
static Ipp32s GetFormatTagByNameAndByDetails(const Ipp8s *pSrcCodecName, Ipp32s fmtDetails, Ipp32s *lFormatTag)
{
   Ipp32s i;
   for(i=0;i<USC_FORMATS.nEntries;i++) {
      if((strcmp(USC_FORMATS.formatsInfo[i].codecName,pSrcCodecName)==0)&&
         CompareDetails(fmtDetails, USC_FORMATS.formatsInfo[i].fmtDetails)){
         *lFormatTag = USC_FORMATS.formatsInfo[i].lFormatTag;
         return 1;
      }
   }
   return 0;
}
