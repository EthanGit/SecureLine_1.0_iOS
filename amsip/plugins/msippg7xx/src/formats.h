/*////////////////////////////////////////////////////////////////////////
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2006 Intel Corporation. All Rights Reserved.
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
// Purpose: USC formats library API header file.
//
////////////////////////////////////////////////////////////////////////*/
#ifndef __FORMATS_H__
#define __FORMATS_H__

#define USC_MAX_NAME_LEN 64

typedef struct _fmtInfo{
   const Ipp8s codecName[USC_MAX_NAME_LEN];
   Ipp32s lFormatTag;
   Ipp32s fmtDetails;
} fmtInfo;

typedef struct _uscFormats
{
   Ipp32s nEntries;
   fmtInfo *formatsInfo;
}uscFormats;

#endif /*__FORMATS_H__*/
