/*////////////////////////////////////////////////////////////////////////
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2005-2008 Intel Corporation. All Rights Reserved.
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
// Purpose: Auxiliary functions file.
//
////////////////////////////////////////////////////////////////////////*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "usc.h"
#include "util.h"
#include "vm_sys_info.h"

#include "ippcore.h"
#include "ipps.h"
#include "ippsc.h"

#define MAX_LINE_LEN 1024

void USC_OutputString(FILE *flog, Ipp8s *format,...)
{
   Ipp8s line[MAX_LINE_LEN];
   va_list  args;

   va_start(args, format);
   vsprintf(line, format, args);
   va_end(args);

   if(flog) {
      fprintf(flog,"%s", line);
   } else {
      printf("%s", line);
   }

   return;
}
