//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 - 2011 Intel Corporation. All Rights Reserved.
//

#include "umc_defs.h"
#if defined(UMC_ENABLE_H264_VIDEO_ENCODER)

#ifndef __UMC_H264_CONFIG__
#define __UMC_H264_CONFIG__

#define BITDEPTH_9_12
//#undef BITDEPTH_9_12

#define ALPHA_BLENDING_H264
//#undef ALPHA_BLENDING_H264

//#define FRAME_QP_FROM_FILE "fqp.txt"
#undef FRAME_QP_FROM_FILE

#define TABLE_FUNC
//#undef TABLE_FUNC

#define FRAME_INTERPOLATION
//#undef FRAME_INTERPOLATION

// Control amount of slices per frame by user-defined slice size limit
// Keep in mind what slice size will be expanded in order to fit at least one macroblock per slice
#define SLICE_CHECK_LIMIT
//#undef SLICE_CHECK_LIMIT

//REMOVE slice check limit if num_slices=0
#define AMD_FIX_SLICE_CHECK_LIMIT

//#define ALT_RC
#undef ALT_RC

//#define FRAME_TYPE_DETECT_DS  //Frame type detector with downsampling
#undef FRAME_TYPE_DETECT_DS

#define ALT_BITSTREAM_ALLOC
//#undef ALT_BITSTREAM_ALLOC



#if defined (_OPENMP)
#include "omp.h"

#define FRAMETYPE_DETECT_ST  //Threaded frame type detector for slice threading
//#undef FRAMETYPE_DETECT_ST
#define DEBLOCK_THREADING     //Threaded deblocking for slice threading
//#undef DEBLOCK_THREADING
#define EXPAND_PLANE_THREAD   //Parallel expand plane for chroma
//#undef EXPAND_PLANE_THREAD
#define INTERPOLATE_FRAME_THREAD //Parallel frame pre interpolation
//#undef INTERPOLATE_FRAME_THREAD
#define SLICE_THREADING_LOAD_BALANCING // Redistribute macroblocks between slices based on previously encoded frame timings
//#undef SLICE_THREADING_LOAD_BALANCING
#endif // _OPENMP

#undef INTRINSIC_OPT
#if !defined(WIN64) && (defined (WIN32) || defined (_WIN32) || defined (WIN32E) || defined (_WIN32E) || defined(__i386__) || defined(__x86_64__))
    #if defined(__INTEL_COMPILER) || (_MSC_VER >= 1300) || (defined(__GNUC__) && defined(__SSE2__) && (__GNUC__ > 3))
        #define INTRINSIC_OPT
        #include "emmintrin.h"
    #endif
#endif

//#define INSERT_SEI
#undef INSERT_SEI

#endif /* __UMC_H264_CONFIG__ */

#endif //UMC_ENABLE_H264_VIDEO_ENCODER
