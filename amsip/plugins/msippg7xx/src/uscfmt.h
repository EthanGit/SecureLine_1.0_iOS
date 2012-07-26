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
// Purpose: Format library API definition header files.
//
////////////////////////////////////////////////////////////////////////*/
#ifndef __USCFMT_H__
#define __USCFMT_H__

#include "vm_types.h"

#define USCFMTINFO  extern

#define USC_MAKE_FORMAT_DET_VERSION(major,minor) (((major&0xFF)<<8)|\
                                                         (minor&0xFF))

#define USC_MAKE_FORMAT_DET(vad,hpf,pf,truncate,law) (((law&0x3)<<5)|\
                                                         ((truncate&0x1)<<4)|\
                                                         ((pf&0x1)<<3)|\
                                                         ((hpf&0x1)<<2)|\
                                                         (vad&0x3))
#define GET_VAD_FROM_FORMAT_DET(fmtDet) (fmtDet&0x3)
#define GET_HPF_FROM_FORMAT_DET(fmtDet) ((fmtDet>>2)&0x1)
#define GET_PF_FROM_FORMAT_DET(fmtDet) ((fmtDet>>3)&0x1)
#define GET_TRUNC_FROM_FORMAT_DET(fmtDet) ((fmtDet>>4)&0x1)
#define GET_LAW_FROM_FORMAT_DET(fmtDet) ((fmtDet>>4)&0x3)

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        GetFormatsInfoVersion
//  Purpose:     getting version of the format library.
//  Returns:     number of of avialable formats.
//  Parameters:
//
*/
USCFMTINFO Ipp32s GetFormatsInfoVersion();

typedef struct {
   /* /////////////////////////////////////////////////////////////////////////////
   //  Name:        GetNumFormats
   //  Purpose:     getting number of of avialable formats.
   //  Returns:     number of of avialable formats.
   //  Parameters:
   //
   */
   Ipp32s (*GetNumFormats)(void);

   /* /////////////////////////////////////////////////////////////////////////////
   //  Name:        GetNameByFormatTag
   //  Purpose:     getting codec name by format tag.
   //  Returns:     1 if success, 0 if fails.
   //  Parameters:
   //    lFormatTag        input format tag.
   //    pDstCodecName     pointer to the output codec name.
   */
   Ipp32s (*GetNameByFormatTag)(Ipp32s lFormatTag, Ipp8s *pDstCodecName);

   /* /////////////////////////////////////////////////////////////////////////////
   //  Name:        GetFormatDetailsByFormatTag
   //  Purpose:     getting format details in USC_MAKE_FORMAT_DET form by format tag.
   //  Returns:     1 if success, 0 if fails.
   //  Parameters:
   //    lFormatTag        input format tag.
   //    pFormatDetails    pointer to the output format details in USC_MAKE_FORMAT_DET form.
   */
   Ipp32s (*GetFormatDetailsByFormatTag)(Ipp32s lFormatTag, Ipp32s *pFormatDetails);

   /* /////////////////////////////////////////////////////////////////////////////
   //  Name:        GetFormatTagByNameAndByDetails
   //  Purpose:     getting format tag by codec name and format details in USC_MAKE_FORMAT_DET.
   //  Returns:     1 if success, 0 if fails.
   //  Parameters:
   //    pSrcCodecName     pointer to the input codec name.
   //    fmtDetails        format details in USC_MAKE_FORMAT_DET form.
   //    lFormatTag        pointer to the output format tag.
   */
   Ipp32s (*GetFormatTagByNameAndByDetails)(const Ipp8s *pSrcCodecName, Ipp32s fmtDetails, Ipp32s *lFormatTag);
} usc_fmt_info_Fxns;

#endif /*__USCFMT_H__*/
