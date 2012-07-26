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
// Purpose: USC wrapper functions header file.
//
////////////////////////////////////////////////////////////////////////*/
#ifndef __USCCODEC_H__
#define __USCCODEC_H__

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecAllocInfo
//  Purpose:     Allocates memory for the USC_CodecInfo structure.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parametres.
*/
Ipp32s USCCodecAllocInfo(USCParams *uscPrms, FILE *flog);

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecGetInfo
//  Purpose:     Retrive info from USC codec.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parametres.
*/
Ipp32s USCCodecGetInfo(USCParams *uscPrms, FILE *flog);

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        SetUSCEncoderPCMType
//  Purpose:     Iterate trough supported pcmTypes and set it.
//  Returns:     None.
//  Parameters:
//    uscPrms                pointer to the input\output USC codec parametres.
//    cmdParams              pointer to the input parametres from command line.
//    fileComressionType     type of input stream, PCM or law.
//    desPCMType             pointer to the stream PCM type.
//    f_log                  pointer to the log file, can be NULL.
*/
Ipp32s SetUSCEncoderPCMType(USCParams *uscPrms, Ipp32s fileComressionType, USC_PCMType *desPCMType, FILE *f_log);
int SetUSCDecoderPCMType(USCParams *uscPrms, int fileComressionType, USC_PCMType *desPCMType, FILE *f_log);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        SetUSCEncoderParams
//  Purpose:     Set encode params.
//  Returns:     None.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parametres.
//    cmdParams     pointer to the input parametres from command line.
*/
void SetUSCEncoderParams(USCParams *uscPrms, CommandLineParams *cmdParams);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        SetUSCDecoderParams
//  Purpose:     Set decode params.
//  Returns:     None.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parametres.
//    cmdParams     pointer to the input parametres from command line.
*/
void SetUSCDecoderParams(USCParams *uscPrms, CommandLineParams *cmdParams);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecAlloc
//  Purpose:     Alloc memory for USC codec.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parametres.
//    cmdParams     pointer to the input parametres from command line.
*/
Ipp32s USCCodecAlloc(USCParams *uscPrms, CommandLineParams *cmdParams);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCEncoderInit
//  Purpose:     Initialize USC encoder object.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parametres.
//    cmdParams     pointer to the input parametres from command line.
*/
Ipp32s USCEncoderInit(USCParams *uscPrms, CommandLineParams *cmdParams, FILE *flog);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCEncoderInit
//  Purpose:     Initialize USC decoder object.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms       pointer to the input\output USC codec parametres.
//    cmdParams     pointer to the input parametres from command line.
*/
Ipp32s USCDecoderInit(USCParams *uscPrms, CommandLineParams *cmdParams, FILE *flog);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCEncoderPreProcessFrame
//  Purpose:     Pre-process frame to encode.
//  Returns:     lenght of written data to the output stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parametres.
//    pSrc      pointer to the input PCM data.
//    pDst      pointer to the output bitstream data.
//    in        pointer to the output USC_PCMStream structure for the frame.
//    out       pointer to the output USC_Bitstream structure for the frame.
*/
Ipp32s USCEncoderPreProcessFrame(USCParams *uscPrms, Ipp8s *pSrc, Ipp8s *pDst,USC_PCMStream *in,USC_Bitstream *out);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCEncoderPostProcessFrame
//  Purpose:     Post-process of encoded frame.
//  Returns:     lenght of written data to the output stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parametres.
//    pSrc      pointer to the input PCM data.
//    pDst      pointer to the output bitstream data.
//    in        pointer to the output USC_PCMStream structure for the frame.
//    out       pointer to the output USC_Bitstream structure for the frame.
*/
Ipp32s USCEncoderPostProcessFrame(USCParams *uscPrms, Ipp8s *pSrc, Ipp8s *pDst,USC_PCMStream *in,USC_Bitstream *out);

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCDecoderPreProcessFrame
//  Purpose:     Pre-process frame to decode.
//  Returns:     lenght of processed data from the input stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parametres.
//    pSrc      pointer to the input bitstream data.
//    pDst      pointer to the output PCM data.
//    in        pointer to the output USC_Bitstream structure for the frame.
//    out       pointer to the output USC_PCMStream structure for the frame.
*/
Ipp32s USCDecoderPreProcessFrame(USCParams *uscPrms, Ipp8s *pSrc, Ipp8s *pDst,USC_Bitstream *in,USC_PCMStream *out);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCDecoderPostProcessFrame
//  Purpose:     Post-process of decoded frame.
//  Returns:     lenght of processed data from the input stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parametres.
//    pSrc      pointer to the input bitstream data.
//    pDst      pointer to the output PCM data.
//    in        pointer to the output USC_Bitstream structure for the frame.
//    out       pointer to the output USC_PCMStream structure for the frame.
*/
Ipp32s USCDecoderPostProcessFrame(USCParams *uscPrms, Ipp8s *pSrc, Ipp8s *pDst,USC_Bitstream *in,USC_PCMStream *out);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecEncode
//  Purpose:     Encode pre-processed frame.
//  Returns:     lenght of processed data from the input stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parametres.
//    in        pointer to the input USC_Bitstream structure for the frame.
//    out       pointer to the output USC_PCMStream structure for the frame.
*/
Ipp32s USCCodecEncode(USCParams *uscPrms, USC_PCMStream *in,USC_Bitstream *out, FILE *flog);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecDecode
//  Purpose:     Decode pre-processed frame.
//  Returns:     lenght of processed data from the input stream.
//  Parameters:
//    uscPrms   pointer to the input USC codec parametres.
//    in        pointer to the input USC_PCMStream structure for the frame.
//    out       pointer to the output USC_Bitstream structure for the frame.
*/
Ipp32s USCCodecDecode(USCParams *uscPrms, USC_Bitstream *in,USC_PCMStream *out, FILE *flog);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecGetSize
//  Purpose:     Get output stream size.
//  Returns:     0 if success, -1 if fails.
//  Parameters:
//    uscPrms   pointer to the input USC codec parametres.
//    lenSrc    length of the input stream.
//    lenDst    pointer to the length of the output stream.
*/
Ipp32s USCCodecGetSize(USCParams *uscPrms, Ipp32s lenSrc, Ipp32s *lenDst, FILE *flog);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCClear
//  Purpose:     Pre-init resources of the USC codec.
//  Returns:     0 if success.
//  Parameters:
//    uscPrms   pointer to the input USC codec parametres.
*/
Ipp32s USCClear(USCParams *uscPrms);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCFree
//  Purpose:     Release resources of the USC codec.
//  Returns:     0 if success.
//  Parameters:
//    uscPrms   pointer to the input USC codec parametres.
*/
Ipp32s USCFree(USCParams *uscPrms);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCGetFrameHeaderSize
//  Purpose:     Retrieve frame header size.
//  Returns:     frame header size.
//  Parameters:
*/
Ipp32s USCGetFrameHeaderSize();
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecCorrectSize
//  Purpose:     Correct output stream size to include frame headers.
//  Returns:     0 if success.
//  Parameters:
//    uscPrms          pointer to the input USC codec parametres.
//    lenSrc           length of the input stream.
//    pSrcDstOutLen    pointer to the corrected length of the output stream.
*/
Ipp32s USCCodecCorrectSize(USCParams *uscPrms, Ipp32s lenSrc, Ipp32s *pSrcDstOutLen);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        USCCodecCorrectSize
//  Purpose:     Get termination condition to organize encode\decode loop.
//  Returns:     0 if success.
//  Parameters:
//    uscPrms          pointer to the input USC codec parametres.
//    pLowBound        pointer to the low bound of the data to process.
//    pSrcDstOutLen    pointer to the corrected length of the output stream.
*/
Ipp32s USCCodecGetTerminationCondition(USCParams *uscPrms, Ipp32s *pLowBound);

Ipp32s USC_CvtToLinear(int framesize, Ipp32s PCMType, Ipp8s **inputBuffer, Ipp32s *pLen);
Ipp32s USC_CvtToLaw(USCParams *uscPrms, Ipp32s PCMType, Ipp8s *inputBuffer, Ipp32s *pLen);

#endif /* __USCCODEC_H__*/
