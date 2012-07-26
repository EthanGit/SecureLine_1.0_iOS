//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 - 2011 Intel Corporation. All Rights Reserved.
//

#include <vm_thread.h>

#if PIXBITS == 8

#define PIXTYPE Ipp8u
#define COEFFSTYPE Ipp16s
#define H264ENC_MAKE_NAME(NAME) NAME##_8u16s

#elif PIXBITS == 16

#define PIXTYPE Ipp16u
#define COEFFSTYPE Ipp32s
#define H264ENC_MAKE_NAME(NAME) NAME##_16u32s

#else //PIXBITS

void H264EncoderFakeFunction() { UNSUPPORTED_PIXBITS; }

#endif //PIXBITS

#define H264CoreEncoderType H264ENC_MAKE_NAME(H264CoreEncoder)
#define H264SliceType H264ENC_MAKE_NAME(H264Slice)
#define H264BsFakeType H264ENC_MAKE_NAME(H264BsFake)
#define H264BsRealType H264ENC_MAKE_NAME(H264BsReal)
#define EncoderRefPicListType H264ENC_MAKE_NAME(EncoderRefPicList)
#define H264EncoderFrameType H264ENC_MAKE_NAME(H264EncoderFrame)

//
// Constructor for the EncoderH264 class.
//
Status H264ENC_MAKE_NAME(H264Slice_Create)(
    void* state)
{
    H264SliceType* slice = (H264SliceType *)state;
    memset(slice, 0, sizeof(H264SliceType));
    return UMC_OK;
}


Status H264ENC_MAKE_NAME(H264Slice_Init)(
    void* state,
    H264EncoderParams &info)
{
    H264SliceType* slice_enc = (H264SliceType *)state;
    Ipp32s status;
    Ipp32s allocSize = 256*sizeof(PIXTYPE) // pred for direct
                     + 256*sizeof(PIXTYPE) // temp working for direct
                     + 256*sizeof(PIXTYPE) // pred for BiPred
                     + 256*sizeof(PIXTYPE) // temp buf for BiPred
                     + 256*sizeof(PIXTYPE) // temp buf for ChromaPred
                     + 256*sizeof(PIXTYPE) // MC
                     + 256*sizeof(PIXTYPE) // ME
                     + 6*512*sizeof(PIXTYPE) // MB prediction & reconstruct
                     + 6*256*sizeof(COEFFSTYPE) //MB transform result
                     + 64*sizeof(Ipp16s)     // Diff
                     + 64*sizeof(COEFFSTYPE) // TransformResults
                     + 64*sizeof(COEFFSTYPE) // QuantResult
                     + 64*sizeof(COEFFSTYPE) // DequantResult
                     + 16*sizeof(COEFFSTYPE) // luma dc
                     + 256*sizeof(Ipp16s)    // MassDiff
                     + 512 + ALIGN_VALUE
                     + 3 * (16 * 16 * 3 + 100);  //Bitstreams
    slice_enc->m_pAllocatedMBEncodeBuffer = (Ipp8u*)H264_Malloc(allocSize);
    if (!slice_enc->m_pAllocatedMBEncodeBuffer)
        return(UMC::UMC_ERR_ALLOC);

    // 16-byte align buffer start
    slice_enc->m_pPred4DirectB = (PIXTYPE*)align_pointer<Ipp8u*>(slice_enc->m_pAllocatedMBEncodeBuffer, ALIGN_VALUE);
    slice_enc->m_pTempBuff4DirectB = slice_enc->m_pPred4DirectB + 256;
    slice_enc->m_pPred4BiPred = slice_enc->m_pTempBuff4DirectB + 256;
    slice_enc->m_pTempBuff4BiPred = slice_enc->m_pPred4BiPred + 256;
    slice_enc->m_pTempChromaPred = slice_enc->m_pTempBuff4BiPred + 256;

    slice_enc->m_cur_mb.mb4x4.prediction = slice_enc->m_pTempChromaPred + 256; // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mb4x4.reconstruct = slice_enc->m_cur_mb.mb4x4.prediction + 256; // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mb4x4.transform = (COEFFSTYPE*)(slice_enc->m_cur_mb.mb4x4.reconstruct + 256); // 256 for pred_intra and 256 for reconstructed blocks

    slice_enc->m_cur_mb.mb8x8.prediction = (PIXTYPE*)(slice_enc->m_cur_mb.mb4x4.transform + 256); // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mb8x8.reconstruct = slice_enc->m_cur_mb.mb8x8.prediction + 256; // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mb8x8.transform = (COEFFSTYPE*)(slice_enc->m_cur_mb.mb8x8.reconstruct + 256); // 256 for pred_intra and 256 for reconstructed blocks

    slice_enc->m_cur_mb.mb16x16.prediction = (PIXTYPE*)(slice_enc->m_cur_mb.mb8x8.transform + 256); // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mb16x16.reconstruct = slice_enc->m_cur_mb.mb16x16.prediction + 256; // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mb16x16.transform = (COEFFSTYPE*)(slice_enc->m_cur_mb.mb16x16.reconstruct + 256); // 256 for pred_intra and 256 for reconstructed blocks

    slice_enc->m_cur_mb.mbInter.prediction = (PIXTYPE*)(slice_enc->m_cur_mb.mb16x16.transform + 256); // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mbInter.reconstruct = slice_enc->m_cur_mb.mbInter.prediction + 256; // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mbInter.transform = (COEFFSTYPE*)(slice_enc->m_cur_mb.mbInter.reconstruct + 256); // 256 for pred_intra and 256 for reconstructed blocks

    slice_enc->m_cur_mb.mbChromaIntra.prediction = (PIXTYPE*)(slice_enc->m_cur_mb.mbInter.transform + 256); // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mbChromaIntra.reconstruct = slice_enc->m_cur_mb.mbChromaIntra.prediction + 256; // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mbChromaIntra.transform = (COEFFSTYPE*)(slice_enc->m_cur_mb.mbChromaIntra.reconstruct + 256); // 256 for pred_intra and 256 for reconstructed blocks

    slice_enc->m_cur_mb.mbChromaInter.prediction = (PIXTYPE*)(slice_enc->m_cur_mb.mbChromaIntra.transform + 256); // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mbChromaInter.reconstruct = slice_enc->m_cur_mb.mbChromaInter.prediction + 256; // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_cur_mb.mbChromaInter.transform = (COEFFSTYPE*)(slice_enc->m_cur_mb.mbChromaInter.reconstruct + 256); // 256 for pred_intra and 256 for reconstructed blocks
    slice_enc->m_pMBEncodeBuffer = (PIXTYPE*)(slice_enc->m_cur_mb.mbChromaInter.transform + 256);

    //Init bitstreams
    if (slice_enc->fakeBitstream == NULL)
    {
        slice_enc->fakeBitstream = (H264BsFakeType *)H264_Malloc(sizeof(H264BsFakeType));
        if (!slice_enc->fakeBitstream)
            return UMC_ERR_ALLOC;
    }

    H264ENC_MAKE_NAME(H264BsFake_Create)(slice_enc->fakeBitstream, 0, 0, info.chroma_format_idc, status);

    Ipp32s i;
    for (i = 0; i < 9; i++)
    {
        if (slice_enc->fBitstreams[i] == NULL)
        {
            slice_enc->fBitstreams[i] = (H264BsFakeType *)H264_Malloc(sizeof(H264BsFakeType));
            if (!slice_enc->fBitstreams[i])
                return UMC_ERR_ALLOC;
        }

        H264ENC_MAKE_NAME(H264BsFake_Create)(slice_enc->fBitstreams[i], 0, 0, info.chroma_format_idc, status);
    }

    return(UMC_OK);
}

void H264ENC_MAKE_NAME(H264Slice_Destroy)(
    void* state)
{
    H264SliceType* slice_enc = (H264SliceType *)state;
    if(slice_enc->m_pAllocatedMBEncodeBuffer != NULL) {
        H264_Free(slice_enc->m_pAllocatedMBEncodeBuffer);
        slice_enc->m_pAllocatedMBEncodeBuffer = NULL;
    }
    slice_enc->m_pPred4DirectB = NULL;
    slice_enc->m_pPred4BiPred = NULL;
    slice_enc->m_pTempBuff4DirectB = NULL;
    slice_enc->m_pTempBuff4BiPred = NULL;
    slice_enc->m_pMBEncodeBuffer = NULL;
    slice_enc->m_cur_mb.mb4x4.prediction = NULL;
    slice_enc->m_cur_mb.mb4x4.reconstruct = NULL;
    slice_enc->m_cur_mb.mb4x4.transform = NULL;
    slice_enc->m_cur_mb.mb8x8.prediction = NULL;
    slice_enc->m_cur_mb.mb8x8.reconstruct = NULL;
    slice_enc->m_cur_mb.mb8x8.transform = NULL;

    if (slice_enc->fakeBitstream != NULL){
        H264_Free(slice_enc->fakeBitstream);
        slice_enc->fakeBitstream = NULL;
    }

    Ipp32s i;
    for (i = 0; i < 9; i++)
    {
        if(slice_enc->fBitstreams[i] != NULL)
        {
            H264_Free(slice_enc->fBitstreams[i]);
            slice_enc->fBitstreams[i] = NULL;
        }
    }
}

Status H264ENC_MAKE_NAME(H264CoreEncoder_Create)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    H264EncoderParams_Create(&core_enc->m_info);

    core_enc->m_pBitStream = NULL;
    core_enc->memAlloc = NULL;
    core_enc->profile_frequency = 1;
    core_enc->m_iProfileIndex = 0;
    core_enc->m_is_mb_data_initialized = false;
    core_enc->m_pAllocEncoderInst = NULL;
    core_enc->m_pbitstreams = NULL;

    core_enc->m_bs1 = NULL;
    H264ENC_MAKE_NAME(H264EncoderFrameList_Create)(&core_enc->m_dpb, NULL);
    H264ENC_MAKE_NAME(H264EncoderFrameList_Create)(&core_enc->m_cpb, NULL);
    core_enc->m_uIntraFrameInterval = 0;
    core_enc->m_uIDRFrameInterval = 0;
    core_enc->m_PicOrderCnt = 0;
    core_enc->m_PicOrderCnt_Accu = 0;
    core_enc->m_pParsedDataNew = 0;
    core_enc->m_pReconstructFrame = NULL;
    core_enc->m_l1_cnt_to_start_B = 0;
    core_enc->m_pMBOffsets = NULL;
    core_enc->m_EmptyThreshold = NULL;
    core_enc->m_DirectBSkipMEThres = NULL;
    core_enc->m_PSkipMEThres = NULL;
    core_enc->m_BestOf5EarlyExitThres = NULL;
    core_enc->use_implicit_weighted_bipred = false;
    core_enc->m_is_cur_pic_afrm = false;
    core_enc->m_Slices = NULL;
    core_enc->m_total_bits_encoded = 0;
#ifdef SLICE_CHECK_LIMIT
    core_enc->m_MaxSliceSize = 0;
#endif

    H264_AVBR_Create(&core_enc->avbr);
    core_enc->m_PaddedSize.width = 0;
    core_enc->m_PaddedSize.height = 0;

    // Initialize the BRCState local variables based on the default
    // settings in core_enc->m_info.

    // If these assertions fail, then uTargetFrmSize needs to be set to
    // something other than 0.
    // Initialize the sequence parameter set structure.

    memset(&core_enc->m_SeqParamSet, 0, sizeof(H264SeqParamSet));
    core_enc->m_SeqParamSet.profile_idc = H264_PROFILE_MAIN;
    core_enc->m_SeqParamSet.chroma_format_idc = 1;
    core_enc->m_SeqParamSet.bit_depth_luma = 8;
    core_enc->m_SeqParamSet.bit_depth_chroma = 8;
    core_enc->m_SeqParamSet.bit_depth_aux = 8;
    core_enc->m_SeqParamSet.alpha_opaque_value = 8;

    // Initialize the picture parameter set structure.
    memset(&core_enc->m_PicParamSet, 0, sizeof(H264PicParamSet));

    // Initialize the slice header structure.
    memset(&core_enc->m_SliceHeader, 0, sizeof(H264SliceHeader));
    core_enc->m_SliceHeader.direct_spatial_mv_pred_flag = 1;

    core_enc->m_DirectTypeStat[0] = 0;
    core_enc->m_DirectTypeStat[1] = 0;

    core_enc->eFrameSeq = (H264EncoderFrameType **)H264_Malloc(1 * sizeof(H264EncoderFrameType*));
    if (!core_enc->eFrameSeq)
    {
        H264ENC_MAKE_NAME(H264CoreEncoder_Destroy)(state);
        return UMC_ERR_ALLOC;
    }

    core_enc->eFrameType = (EnumPicCodType *)H264_Malloc(1 * sizeof(EnumPicCodType));;
    if (!core_enc->eFrameType)
    {
        H264ENC_MAKE_NAME(H264CoreEncoder_Destroy)(state);
        return UMC_ERR_ALLOC;
    }

    core_enc->eFrameType[0] = PREDPIC;
    return UMC_OK;
}


//
// Encode - drives the compression of a "Temporal Reference"
//
Status H264ENC_MAKE_NAME(H264CoreEncoder_Encode)(
    void* state,
    VideoData* src,
    MediaData* dst,
    const H264_Encoder_Compression_Flags flags,
    H264_Encoder_Compression_Notes& notes)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Status ps = UMC_OK;
    core_enc->m_pCurrentFrame = 0;
    core_enc->m_field_index = 0;
    Ipp32s isRef = 0;
    Ipp32s frame_index = 0;

    if (!(flags & H264_ECF_LAST_FRAME)){
        Ipp32u nShortTerm, nLongTerm;
        EnumPicCodType  ePictureType;
        FrameType ft = src->GetFrameType();
        switch(ft){
            case I_PICTURE:
                ePictureType = INTRAPIC;
                break;
            case P_PICTURE:
                //Check for available frames in dpb
                H264ENC_MAKE_NAME(H264EncoderFrameList_countActiveRefs)(
                    &core_enc->m_dpb,
                    nShortTerm,
                    nLongTerm);
                if ((nShortTerm + nLongTerm) == 0)
                    return UMC_ERR_NOT_ENOUGH_DATA; //May be it should be  UMC_ERR_INVALID_PARAMS
                ePictureType = PREDPIC;
                break;
            case B_PICTURE:
                //Check for available frames in dpb
                H264ENC_MAKE_NAME(H264EncoderFrameList_countActiveRefs)(
                    &core_enc->m_dpb,
                    nShortTerm,
                    nLongTerm);
                if( (nShortTerm + nLongTerm) == 0 ) return UMC_ERR_NOT_ENOUGH_DATA; //May be it should be  UMC_ERR_INVALID_PARAMS
                ePictureType = BPREDPIC;
                break;
            case NONE_PICTURE:
            default:
#ifdef FRAME_QP_FROM_FILE
                char ft = frame_type.front();
                frame_type.pop_front();
                isRef = 0;
                switch( ft ){
                    case 'i':
                    case 'I':
                        ePictureType = INTRAPIC;
                        isRef = 1;
                        core_enc->m_bMakeNextFrameIDR = true;
                        break;
                    case 'p':
                    case 'P':
                        ePictureType = PREDPIC;
                        isRef = 1;
                        break;
                    case 'b':
                    case 'B':
                        ePictureType = BPREDPIC;
                        break;
                }
                //fprintf(stderr,"frame: %d %c ",core_enc->m_uFrames_Num,ft);
#else
                frame_index = core_enc->m_iProfileIndex;
                ePictureType = H264ENC_MAKE_NAME(H264CoreEncoder_DetermineFrameType)(state, core_enc->m_iProfileIndex);
                if (ePictureType == BPREDPIC) {
                    switch( core_enc->m_info.treat_B_as_reference ){
                        case 0:
                            isRef = 0; //No B references
                            break;
                        case 1:
                            if (core_enc->m_iProfileIndex - 1 == ((core_enc->m_info.B_frame_rate + 1) >> 1)) //Only middle B is reference, better to use round to low in case of even number of B frames
                                isRef = 1;
                            else
                                isRef = 0;
                            break;
                        case 2: //All B frames are refereces
                            isRef = 1;
                            break;
                    }
                }
#endif
                break;
        }

        core_enc->m_pLastFrame = core_enc->m_pCurrentFrame = H264ENC_MAKE_NAME(H264EncoderFrameList_InsertFrame)(
			core_enc->m_MaxSliceSize,
            &core_enc->m_cpb,
            src,
            ePictureType,
            isRef,
            core_enc->m_info.num_slices * ((core_enc->m_info.coding_type == 1) + 1),
            core_enc->m_PaddedSize
#if defined (ALPHA_BLENDING_H264)
            , core_enc->m_SeqParamSet.aux_format_idc
#endif
            );
            // pad frame to MB boundaries
        {
            Ipp32s padW = core_enc->m_PaddedSize.width - core_enc->m_pCurrentFrame->uWidth;
            if (padW > 0) {
                Ipp32s  i, j;
                PIXTYPE *py = core_enc->m_pCurrentFrame->m_pYPlane + core_enc->m_pCurrentFrame->uWidth;
                for (i = 0; i < (Ipp32s)core_enc->m_pCurrentFrame->uHeight; i ++) {
                    for (j = 0; j < padW; j ++)
                        py[j] = py[-1];
                        // py[j] = 0;
                    py += core_enc->m_pCurrentFrame->m_pitchPixels;
                }
                if (core_enc->m_info.chroma_format_idc != 0) {
                    Ipp32s h = core_enc->m_pCurrentFrame->uHeight;
                    if (core_enc->m_info.chroma_format_idc == 1)
                        h >>= 1;
                    padW >>= 1;
                    PIXTYPE *pu = core_enc->m_pCurrentFrame->m_pUPlane + (core_enc->m_pCurrentFrame->uWidth >> 1);
                    PIXTYPE *pv = core_enc->m_pCurrentFrame->m_pVPlane + (core_enc->m_pCurrentFrame->uWidth >> 1);
                    for (i = 0; i < h; i ++) {
                        for (j = 0; j < padW; j ++) {
                            pu[j] = pu[-1];
                            pv[j] = pv[-1];
                            // pu[j] = 0;
                            // pv[j] = 0;
                        }
                        pu += core_enc->m_pCurrentFrame->m_pitchPixels;
                        pv += core_enc->m_pCurrentFrame->m_pitchPixels;
                    }
                }
            }
            Ipp32s padH = core_enc->m_PaddedSize.height - core_enc->m_pCurrentFrame->uHeight;
            if (padH > 0) {
                Ipp32s  i;
                PIXTYPE *pyD = core_enc->m_pCurrentFrame->m_pYPlane + core_enc->m_pCurrentFrame->uHeight * core_enc->m_pCurrentFrame->m_pitchPixels;
                PIXTYPE *pyS = pyD - core_enc->m_pCurrentFrame->m_pitchPixels;
                for (i = 0; i < padH; i ++) {
                    memcpy(pyD, pyS, core_enc->m_PaddedSize.width * sizeof(PIXTYPE));
                    //memset(pyD, 0, core_enc->m_PaddedSize.width * sizeof(PIXTYPE));
                    pyD += core_enc->m_pCurrentFrame->m_pitchPixels;
                }
                if (core_enc->m_info.chroma_format_idc != 0) {
                    Ipp32s h = core_enc->m_pCurrentFrame->uHeight;
                    if (core_enc->m_info.chroma_format_idc == 1) {
                        h >>= 1;
                        padH >>= 1;
                    }
                    PIXTYPE *puD = core_enc->m_pCurrentFrame->m_pUPlane + h * core_enc->m_pCurrentFrame->m_pitchPixels;
                    PIXTYPE *pvD = core_enc->m_pCurrentFrame->m_pVPlane + h * core_enc->m_pCurrentFrame->m_pitchPixels;
                    PIXTYPE *puS = puD - core_enc->m_pCurrentFrame->m_pitchPixels;
                    PIXTYPE *pvS = pvD - core_enc->m_pCurrentFrame->m_pitchPixels;
                    for (i = 0; i < padH; i ++) {
                        memcpy(puD, puS, (core_enc->m_PaddedSize.width >> 1) * sizeof(PIXTYPE));
                        memcpy(pvD, pvS, (core_enc->m_PaddedSize.width >> 1) * sizeof(PIXTYPE));
                        //memset(puD, 0, (core_enc->m_PaddedSize.width >> 1) * sizeof(PIXTYPE));
                        //memset(pvD, 0, (core_enc->m_PaddedSize.width >> 1) * sizeof(PIXTYPE));
                        puD += core_enc->m_pCurrentFrame->m_pitchPixels;
                        pvD += core_enc->m_pCurrentFrame->m_pitchPixels;
                    }
                }
            }
        }
        if(core_enc->m_pCurrentFrame){
#ifdef FRAME_QP_FROM_FILE
            int qp = frame_qp.front();
            frame_qp.pop_front();
            core_enc->m_pCurrentFrame->frame_qp = qp;
            //fprintf(stderr,"qp: %d\n",qp);
#endif
            core_enc->m_pCurrentFrame->m_bIsIDRPic = false;
            if (core_enc->m_bMakeNextFrameIDR && ePictureType == INTRAPIC){
                core_enc->m_pCurrentFrame->m_bIsIDRPic = true;
                core_enc->m_PicOrderCnt_Accu += core_enc->m_PicOrderCnt;
                core_enc->m_PicOrderCnt = 0;
                core_enc->m_bMakeNextFrameIDR = false;
            }
            switch (core_enc->m_info.coding_type) {
            case 0:
                core_enc->m_pCurrentFrame->m_PictureStructureForDec = core_enc->m_pCurrentFrame->m_PictureStructureForRef = FRM_STRUCTURE;
                core_enc->m_pCurrentFrame->m_PicOrderCnt[0] = core_enc->m_PicOrderCnt * 2;
                core_enc->m_pCurrentFrame->m_PicOrderCnt[1] = core_enc->m_PicOrderCnt * 2 + 1;
                core_enc->m_pCurrentFrame->m_PicOrderCounterAccumulated = 2*(core_enc->m_PicOrderCnt + core_enc->m_PicOrderCnt_Accu);
                core_enc->m_pCurrentFrame->m_bottom_field_flag[0] = 0;
                core_enc->m_pCurrentFrame->m_bottom_field_flag[1] = 0;
                break;
            case 1:
                core_enc->m_pCurrentFrame->m_PictureStructureForDec = core_enc->m_pCurrentFrame->m_PictureStructureForRef = FLD_STRUCTURE;
                core_enc->m_pCurrentFrame->m_PicOrderCnt[0] = core_enc->m_PicOrderCnt * 2;
                core_enc->m_pCurrentFrame->m_PicOrderCnt[1] = core_enc->m_PicOrderCnt * 2 + 1;
                core_enc->m_pCurrentFrame->m_PicOrderCounterAccumulated = 2*(core_enc->m_PicOrderCnt + core_enc->m_PicOrderCnt_Accu);
                core_enc->m_pCurrentFrame->m_bottom_field_flag[0] = 0;
                core_enc->m_pCurrentFrame->m_bottom_field_flag[1] = 1;
                break;
            case 2:
                core_enc->m_pCurrentFrame->m_PictureStructureForDec = core_enc->m_pCurrentFrame->m_PictureStructureForRef = AFRM_STRUCTURE;
                core_enc->m_pCurrentFrame->m_PicOrderCnt[0] = core_enc->m_PicOrderCnt * 2;
                core_enc->m_pCurrentFrame->m_PicOrderCnt[1] = core_enc->m_PicOrderCnt * 2 + 1;
                core_enc->m_pCurrentFrame->m_PicOrderCounterAccumulated = 2*(core_enc->m_PicOrderCnt + core_enc->m_PicOrderCnt_Accu);
                core_enc->m_pCurrentFrame->m_bottom_field_flag[0] = 0;
                core_enc->m_pCurrentFrame->m_bottom_field_flag[1] = 0;
                break;
            default:
                return UMC_ERR_UNSUPPORTED;
            }
            H264ENC_MAKE_NAME(H264EncoderFrame_InitRefPicListResetCount)(core_enc->m_pCurrentFrame, 0);
            H264ENC_MAKE_NAME(H264EncoderFrame_InitRefPicListResetCount)(core_enc->m_pCurrentFrame, 1);
            core_enc->m_PicOrderCnt++;
            if (core_enc->m_pCurrentFrame->m_bIsIDRPic)
                H264ENC_MAKE_NAME(H264EncoderFrameList_IncreaseRefPicListResetCount)(&core_enc->m_cpb, core_enc->m_pCurrentFrame);
            if (core_enc->m_Analyse & ANALYSE_FRAME_TYPE)
                H264ENC_MAKE_NAME(H264CoreEncoder_FrameTypeDetect)(state);
        } else {
            return UMC_ERR_INVALID_STREAM;
        }
    }else{
        if (core_enc->m_pLastFrame && core_enc->m_pLastFrame->m_PicCodType == BPREDPIC) {
            core_enc->m_pLastFrame->m_PicCodType = PREDPIC;
            core_enc->m_l1_cnt_to_start_B = 1;
        }
    }

    if (UMC_OK == ps) {
        core_enc->m_pCurrentFrame = H264ENC_MAKE_NAME(H264EncoderFrameList_findOldestToEncode)(
            &core_enc->m_cpb,
            &core_enc->m_dpb,
            core_enc->m_l1_cnt_to_start_B,
            core_enc->m_info.treat_B_as_reference);

        if (core_enc->m_pCurrentFrame){

            if (core_enc->m_pCurrentFrame->m_bIsIDRPic)
                core_enc->m_uFrameCounter = 0;
            core_enc->m_pCurrentFrame->m_FrameNum = core_enc->m_uFrameCounter;

//            if( core_enc->m_pCurrentFrame->m_PicCodType != BPREDPIC || core_enc->m_info.treat_B_as_reference )
            if( core_enc->m_pCurrentFrame->m_PicCodType != BPREDPIC || core_enc->m_pCurrentFrame->m_RefPic )
                core_enc->m_uFrameCounter++;

            H264ENC_MAKE_NAME(H264CoreEncoder_MoveFromCPBToDPB)(state);

            notes &= ~H264_ECN_NO_FRAME;
        } else {
            core_enc->cnotes |= H264_ECN_NO_FRAME;
            return UMC_ERR_NOT_ENOUGH_DATA;
        }

        EnumPicCodType ePictureType = core_enc->m_pCurrentFrame->m_PicCodType;
        EnumPicClass    ePic_Class;
        switch (ePictureType) {
            case INTRAPIC:
                if (core_enc->m_pCurrentFrame->m_bIsIDRPic || core_enc->m_uFrames_Num == 0) {
                    // Right now, only the first INTRAPIC in a sequence is an IDR Frame
                    ePic_Class = IDR_PIC;
                    core_enc->m_l1_cnt_to_start_B = core_enc->m_info.num_ref_to_start_code_B_slice;
                } else
                    ePic_Class = REFERENCE_PIC;
                break;

            case PREDPIC:
                ePic_Class = REFERENCE_PIC;
                break;

            case BPREDPIC:
                ePic_Class = core_enc->m_info.treat_B_as_reference && core_enc->m_pCurrentFrame->m_RefPic ? REFERENCE_PIC : DISPOSABLE_PIC;
                break;

            default:
                VM_ASSERT(false);
                ePic_Class = IDR_PIC;
                break;
        }

        // ePictureType is a reference variable.  It is updated by CompressFrame if the frame is internally forced to a key frame.
        size_t data_size = dst->GetDataSize();
        ps = H264ENC_MAKE_NAME(H264CoreEncoder_CompressFrame)(state, ePictureType, ePic_Class, dst);
        dst->SetTime( core_enc->m_pCurrentFrame->pts_start, core_enc->m_pCurrentFrame->pts_end);
        data_size = dst->GetDataSize() - data_size;
        if (data_size > 0) {
            if (ePictureType == INTRAPIC)       // Tell the environment we just generated a key frame
                notes |= H264_ECN_KEY_FRAME;
            else if (ePictureType == BPREDPIC) // Tell the environment we just generated a B frame
                notes |= H264_ECN_B_FRAME;
        }

        if (ps == UMC_OK && data_size == 0 && (flags & H264_ECF_LAST_FRAME))
            notes |= H264_ECN_NO_FRAME;
    }

    if (ps == UMC_OK)
        core_enc->m_pCurrentFrame->m_wasEncoded = true;
    H264ENC_MAKE_NAME(H264CoreEncoder_CleanDPB)(state);
    if (ps == UMC_OK)
        core_enc->m_info.numEncodedFrames++;
    return ps;
}


/**********************************************************************
 *  EncoderH264 destructor.
 **********************************************************************/
void H264ENC_MAKE_NAME(H264CoreEncoder_Destroy)(
    void* state)
{
    if (state) {
        H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
        // release the noise reduction prefilter
        H264ENC_MAKE_NAME(H264CoreEncoder_Close)(state);
        H264_AVBR_Destroy(&core_enc->avbr);
        H264EncoderParams_Destroy(&core_enc->m_info);
        H264ENC_MAKE_NAME(H264EncoderFrameList_Destroy)(&core_enc->m_dpb);
        H264ENC_MAKE_NAME(H264EncoderFrameList_Destroy)(&core_enc->m_cpb);
    }
}

//#include "vm_time.h"
//extern vm_tick t_slices[10000][8];
//extern int frame_count;
//
// CompressFrame
//
#define FRAME_SIZE_RATIO_THRESH_MAX 16.0
#define FRAME_SIZE_RATIO_THRESH_MIN 8.0
#define TOTAL_SIZE_RATIO_THRESH 1.5

Status H264ENC_MAKE_NAME(H264CoreEncoder_CompressFrame)(
    void* state,
    EnumPicCodType& ePictureType,
    EnumPicClass& ePic_Class,
    MediaData* dst)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Status status = UMC_OK;
    Ipp32s slice;
    bool bufferOverflowFlag = false;
    bool buffersNotFull = true;
    Ipp32s bitsPerFrame = 0;
    bool brcRecode = false;

#ifdef SLICE_CHECK_LIMIT
    Ipp32s numSliceMB;
#endif // SLICE_CHECK_LIMIT

    Ipp32s data_size = (Ipp32s)dst->GetDataSize();
    //   fprintf(stderr,"frame=%d\n", core_enc->m_uFrames_Num);
    if (core_enc->m_Analyse & ANALYSE_RECODE_FRAME /*&& core_enc->m_pReconstructFrame == NULL //could be size change*/){ //make init reconstruct buffer
        core_enc->m_pReconstructFrame = (H264EncoderFrameType *)H264_Malloc(sizeof(H264EncoderFrameType));
        if (!core_enc->m_pReconstructFrame)
            return UMC::UMC_ERR_ALLOC;
        H264ENC_MAKE_NAME(H264EncoderFrame_Create)(core_enc->m_MaxSliceSize, core_enc->m_pReconstructFrame, &core_enc->m_pCurrentFrame->m_data, core_enc->memAlloc,
#if defined (ALPHA_BLENDING_H264)
            core_enc->m_SeqParamSet.aux_format_idc,
#endif // ALPHA_BLENDING_H264
            0);
        if (H264ENC_MAKE_NAME(H264EncoderFrame_allocate)(core_enc->m_pReconstructFrame, core_enc->m_PaddedSize, core_enc->m_info.num_slices))
            return UMC::UMC_ERR_ALLOC;
        core_enc->m_pReconstructFrame->m_bottom_field_flag[0] = core_enc->m_pCurrentFrame->m_bottom_field_flag[0];
        core_enc->m_pReconstructFrame->m_bottom_field_flag[1] = core_enc->m_pCurrentFrame->m_bottom_field_flag[1];
    }
  // reencode frame loop
    for (;;) {
        brcRecode = false;
        core_enc->m_is_cur_pic_afrm = (Ipp32s)(core_enc->m_pCurrentFrame->m_PictureStructureForDec==AFRM_STRUCTURE);
        for (core_enc->m_field_index=0;core_enc->m_field_index<=(Ipp8u)(core_enc->m_pCurrentFrame->m_PictureStructureForDec<FRM_STRUCTURE);core_enc->m_field_index++) {
            core_enc->m_NeedToCheckMBSliceEdges = core_enc->m_info.num_slices > 1 || core_enc->m_field_index >0;
            EnumSliceType default_slice_type = INTRASLICE;
            bool startPicture = true;
#ifdef SLICE_CHECK_LIMIT
#ifdef AMD_FIX_SLICE_CHECK_LIMIT
			if (core_enc->m_MaxSliceSize)
					numSliceMB = 0;
#else
			numSliceMB = 0;
#endif
#endif // SLICE_CHECK_LIMIT
#if defined (ALPHA_BLENDING_H264)
            bool alpha = true; // Is changed to the opposite at the beginning.
            do { // First iteration primary picture, second -- alpha (when present)
                alpha = !alpha;
                if(!alpha) {
#endif // ALPHA_BLENDING_H264
                if (ePic_Class == IDR_PIC) {
                    if (core_enc->m_field_index)
                        ePic_Class = REFERENCE_PIC;
                    else {
                        //if (core_enc->m_uFrames_Num == 0) //temoporaly disabled
                        {
                            H264ENC_MAKE_NAME(H264CoreEncoder_SetSequenceParameters)(state);
                            H264ENC_MAKE_NAME(H264CoreEncoder_SetPictureParameters)(state);
                        }
                        // Toggle the idr_pic_id on and off so that adjacent IDRs will have different values
                        // This is done here because it is done per frame and not per slice.
//FPV                            core_enc->m_SliceHeader.idr_pic_id ^= 0x1;
                        core_enc->m_SliceHeader.idr_pic_id++;
                        core_enc->m_SliceHeader.idr_pic_id &= 0xff; //Restrict to 255 to reduce number of bits(max value 65535 in standard)
                    }
                }
                core_enc->m_PicType = ePictureType;
                default_slice_type = (core_enc->m_PicType == PREDPIC) ? PREDSLICE : (core_enc->m_PicType == INTRAPIC) ? INTRASLICE : BPREDSLICE;
#ifdef ALT_RC
                if (core_enc->m_info.rate_controls.method == H264_RCM_VBR || core_enc->m_info.rate_controls.method == H264_RCM_CBR) {
                    if (core_enc->m_PicType == INTRAPIC)
                        H264_AVBR_SetGopLen(&core_enc->avbr, core_enc->m_uIntraFrameInterval);
                    H264_AVBR_PreFrame(&core_enc->avbr, core_enc->m_PicType);
                    //core_enc->avbr.mQuantPrev = core_enc->avbr.mQuantB = core_enc->avbr.mQuantP = core_enc->avbr.mQuantI = 1;
                }
#endif // ALT_RC
                core_enc->m_PicParamSet.chroma_format_idc = core_enc->m_SeqParamSet.chroma_format_idc;
                core_enc->m_PicParamSet.bit_depth_luma = core_enc->m_SeqParamSet.bit_depth_luma;
#if defined (ALPHA_BLENDING_H264)
            } else {
                H264ENC_MAKE_NAME(H264EncoderFrameList_switchToAuxiliary)(&core_enc->m_cpb);
                H264ENC_MAKE_NAME(H264EncoderFrameList_switchToAuxiliary)(&core_enc->m_dpb);
                core_enc->m_PicParamSet.chroma_format_idc = 0;
                core_enc->m_PicParamSet.bit_depth_luma = core_enc->m_SeqParamSet.bit_depth_aux;
            }
#endif // ALPHA_BLENDING_H264
            if (!(core_enc->m_Analyse & ANALYSE_RECODE_FRAME))
                core_enc->m_pReconstructFrame = core_enc->m_pCurrentFrame;
            // reset bitstream object before begin compression
            Ipp32s i;
            for (i = 0; i < core_enc->m_info.num_slices*((core_enc->m_info.coding_type == 1) + 1); i++)   //TODO fix for PicAFF/AFRM
                H264ENC_MAKE_NAME(H264BsReal_Reset)(core_enc->m_pbitstreams[i]);
#if defined (ALPHA_BLENDING_H264)
            if (!alpha)
#endif // ALPHA_BLENDING_H264
            {
                H264ENC_MAKE_NAME(H264CoreEncoder_SetSliceHeaderCommon)(state, core_enc->m_pCurrentFrame);
                if( default_slice_type == BPREDSLICE ){
                    if( core_enc->m_Analyse & ANALYSE_ME_AUTO_DIRECT){
                        if( core_enc->m_SliceHeader.direct_spatial_mv_pred_flag )
                            core_enc->m_SliceHeader.direct_spatial_mv_pred_flag = core_enc->m_DirectTypeStat[0] > ((545*core_enc->m_DirectTypeStat[1])>>9) ? 0:1;
                        else
                            core_enc->m_SliceHeader.direct_spatial_mv_pred_flag = core_enc->m_DirectTypeStat[1] > ((545*core_enc->m_DirectTypeStat[0])>>9) ? 1:0;
                        core_enc->m_DirectTypeStat[0]=core_enc->m_DirectTypeStat[1]=0;
                    } else
                        core_enc->m_SliceHeader.direct_spatial_mv_pred_flag = core_enc->m_info.direct_pred_mode & 0x1;
                }
                status = H264ENC_MAKE_NAME(H264CoreEncoder_encodeFrameHeader)(state, core_enc->m_bs1, dst, (ePic_Class == IDR_PIC), startPicture);
                if (status != UMC_OK)
                    goto done;
            }
            status = H264ENC_MAKE_NAME(H264CoreEncoder_Start_Picture)(state, &ePic_Class, ePictureType);
            if (status != UMC_OK)
                goto done;
            Ipp32s slice_qp_delta_default = core_enc->m_Slices[0].m_slice_qp_delta;
            H264ENC_MAKE_NAME(H264CoreEncoder_UpdateRefPicListCommon)(state);
#if defined (_OPENMP)
            vm_thread_priority mainTreadPriority = vm_get_current_thread_priority();
#pragma omp parallel for private(slice)
#endif // _OPENMP
            for (slice = (Ipp32s)core_enc->m_info.num_slices*core_enc->m_field_index; slice < core_enc->m_info.num_slices*(core_enc->m_field_index+1); slice++) {
#if defined (_OPENMP)
                vm_set_current_thread_priority(mainTreadPriority);
#endif // _OPENMP
//                t_slices[frame_count][slice - core_enc->m_info.num_slices * core_enc->m_field_index] = vm_time_get_tick();
                core_enc->m_Slices[slice].m_slice_qp_delta = (Ipp8s)slice_qp_delta_default;
                core_enc->m_Slices[slice].m_slice_number = slice;
                core_enc->m_Slices[slice].m_slice_type = default_slice_type; // Pass to core encoder
#ifdef ALT_RC
                core_enc->m_Slices[slice].m_Texture_Sum = 0;
                core_enc->m_Slices[slice].m_Sad_Sum = 0;
#endif // ALT_RC
                H264ENC_MAKE_NAME(H264CoreEncoder_UpdateRefPicList)(state, core_enc->m_Slices + slice, &core_enc->m_pCurrentFrame->m_pRefPicList[slice], core_enc->m_SliceHeader, &core_enc->m_ReorderInfoL0, &core_enc->m_ReorderInfoL1);
                //if (core_enc->m_SliceHeader.MbaffFrameFlag)
                //    H264ENC_MAKE_NAME(H264CoreEncoder_UpdateRefPicList)(state, &core_enc->m_pRefPicList[slice], core_enc->m_SliceHeader, &core_enc->m_ReorderInfoL0, &core_enc->m_ReorderInfoL1);
#ifdef SLICE_CHECK_LIMIT
re_encode_slice:
#endif // SLICE_CHECK_LIMIT
                Ipp32s slice_bits = H264BsBase_GetBsOffset(core_enc->m_Slices[slice].m_pbitstream);
                EnumPicCodType pic_type = INTRAPIC;
                if (core_enc->m_info.rate_controls.method == H264_RCM_VBR_SLICE || core_enc->m_info.rate_controls.method == H264_RCM_CBR_SLICE){
                    pic_type = (core_enc->m_Slices[slice].m_slice_type == INTRASLICE) ? INTRAPIC : (core_enc->m_Slices[slice].m_slice_type == PREDSLICE) ? PREDPIC : BPREDPIC;
                    core_enc->m_Slices[slice].m_slice_qp_delta = (Ipp8s)(H264_AVBR_GetQP(&core_enc->avbr, pic_type) - core_enc->m_PicParamSet.pic_init_qp);
                    core_enc->m_Slices[slice].m_iLastXmittedQP = core_enc->m_PicParamSet.pic_init_qp + core_enc->m_Slices[slice].m_slice_qp_delta;
                }
                // Compress one slice
                if (core_enc->m_is_cur_pic_afrm)
                    core_enc->m_Slices[slice].status = H264ENC_MAKE_NAME(H264CoreEncoder_Compress_Slice_MBAFF)(state, core_enc->m_Slices + slice);
                else {
                    core_enc->m_Slices[slice].status = H264ENC_MAKE_NAME(H264CoreEncoder_Compress_Slice)(state, core_enc->m_Slices + slice, core_enc->m_Slices[slice].m_slice_number == core_enc->m_info.num_slices*core_enc->m_field_index);
#ifdef SLICE_CHECK_LIMIT
                    if( core_enc->m_MaxSliceSize){
                        Ipp32s numMBs = core_enc->m_HeightInMBs*core_enc->m_WidthInMBs;
                        numSliceMB += core_enc->m_Slices[slice].m_MB_Counter;
                        dst->SetDataSize(dst->GetDataSize() + H264ENC_MAKE_NAME(H264BsReal_EndOfNAL)(core_enc->m_Slices[slice].m_pbitstream, (Ipp8u*)dst->GetDataPointer() + dst->GetDataSize(), (ePic_Class != DISPOSABLE_PIC),
#if defined (ALPHA_BLENDING_H264)
                            (alpha) ? NAL_UT_LAYERNOPART :
#endif // ALPHA_BLENDING_H264
                            ((ePic_Class == IDR_PIC) ? NAL_UT_IDR_SLICE : NAL_UT_SLICE), startPicture));
                        if (numSliceMB != numMBs) {
                            core_enc->m_NeedToCheckMBSliceEdges = true;
                            H264ENC_MAKE_NAME(H264BsReal_Reset)(core_enc->m_Slices[slice].m_pbitstream);
                            core_enc->m_Slices[slice].m_slice_number++;
                            goto re_encode_slice;
                        } else
                            core_enc->m_Slices[slice].m_slice_number = slice;
                    }
#endif // SLICE_CHECK_LIMIT
                }
                if (core_enc->m_info.rate_controls.method == H264_RCM_VBR_SLICE || core_enc->m_info.rate_controls.method == H264_RCM_CBR_SLICE)
                    H264_AVBR_PostFrame(&core_enc->avbr, pic_type, (Ipp32s)slice_bits);
                slice_bits = H264BsBase_GetBsOffset(core_enc->m_Slices[slice].m_pbitstream) - slice_bits;

//                t_slices[frame_count][slice - core_enc->m_info.num_slices * core_enc->m_field_index] =
//                    vm_time_get_tick() - t_slices[frame_count][slice - core_enc->m_info.num_slices * core_enc->m_field_index];
            }
//            frame_count++;
#ifdef SLICE_THREADING_LOAD_BALANCING
            Ipp64s ticks_total = 0;
            for (slice = (Ipp32s)core_enc->m_info.num_slices*core_enc->m_field_index; slice < core_enc->m_info.num_slices*(core_enc->m_field_index+1); slice++)
                ticks_total += core_enc->m_Slices[slice].m_ticks_per_slice;
            if (core_enc->m_pCurrentFrame->m_PicCodType == INTRAPIC)
            {
                core_enc->m_B_ticks_data_available = 0;
                core_enc->m_P_ticks_data_available = 0;
                core_enc->m_P_ticks_per_frame = ticks_total;
                core_enc->m_B_ticks_per_frame = ticks_total;
            }
            else if (core_enc->m_pCurrentFrame->m_PicCodType == PREDPIC)
            {
                core_enc->m_P_ticks_data_available = 1;
                core_enc->m_P_ticks_per_frame = ticks_total;
            }
            else
            {
                core_enc->m_B_ticks_data_available = 1;
                core_enc->m_B_ticks_per_frame = ticks_total;
            }
#endif // SLICE_THREADING_LOAD_BALANCING

            //Write slice to the stream in order, copy Slice RBSP to the end of the output buffer after adding start codes and SC emulation prevention.
#ifdef SLICE_CHECK_LIMIT
            if(!core_enc->m_MaxSliceSize)
            {
#endif // SLICE_CHECK_LIMIT
                for (slice = (Ipp32s)core_enc->m_info.num_slices*core_enc->m_field_index; slice < core_enc->m_info.num_slices*(core_enc->m_field_index+1); slice++)
                {
                    if(dst->GetDataSize() + (Ipp32s)H264BsBase_GetBsSize(core_enc->m_Slices[slice].m_pbitstream) + 5 /* possible extra bytes */ > dst->GetBufferSize())
                        bufferOverflowFlag = true;
                    else
                    {   //Write to output bitstream
                        dst->SetDataSize(dst->GetDataSize() + H264ENC_MAKE_NAME(H264BsReal_EndOfNAL)(core_enc->m_Slices[slice].m_pbitstream, (Ipp8u*)dst->GetDataPointer() + dst->GetDataSize(), (ePic_Class != DISPOSABLE_PIC),
#if defined (ALPHA_BLENDING_H264)
                            (alpha) ? NAL_UT_LAYERNOPART :
#endif // ALPHA_BLENDING_H264
                            ((ePic_Class == IDR_PIC) ? NAL_UT_IDR_SLICE : NAL_UT_SLICE), startPicture));
                    }
                    buffersNotFull = buffersNotFull && H264BsBase_CheckBsLimit(core_enc->m_Slices[slice].m_pbitstream);
                }
                if (bufferOverflowFlag)
                {
                    if(core_enc->m_Analyse & ANALYSE_RECODE_FRAME)
                        goto recode_check; //Output buffer overflow
                    else
                    {
                        status = UMC::UMC_ERR_NOT_ENOUGH_BUFFER;
                        goto done;
                    }
                }
                // check for buffer overrun on some of slices
                if (!buffersNotFull )
                {
                    if( core_enc->m_Analyse & ANALYSE_RECODE_FRAME )
                        goto recode_check;
                    else
                    {
                        status = H264ENC_MAKE_NAME(H264CoreEncoder_EncodeDummyFrame)(state, dst);
                        core_enc->m_bMakeNextFrameKey = true;
                        if (status == UMC_OK)
                            goto end_of_frame;
                        else
                            goto done;
                    }
                }

                if (core_enc->m_info.rate_controls.method == H264_RCM_VBR || core_enc->m_info.rate_controls.method == H264_RCM_CBR)
                {
                    bitsPerFrame = (Ipp32s)((dst->GetDataSize() - data_size) << 3);
                    if (core_enc->m_Analyse & ANALYSE_RECODE_FRAME)
                    {
                        Ipp64s totEncoded, totTarget;
                        totEncoded = core_enc->avbr.mBitsEncodedTotal + bitsPerFrame;
                        totTarget = core_enc->avbr.mBitsDesiredTotal + core_enc->avbr.mBitsDesiredFrame;
                        Ipp64f thratio, thratio_tot = (Ipp64f)((totEncoded - totTarget) / totTarget);
                        thratio = FRAME_SIZE_RATIO_THRESH_MAX * (1.0 - thratio_tot);
                        h264_Clip(thratio, FRAME_SIZE_RATIO_THRESH_MIN, FRAME_SIZE_RATIO_THRESH_MAX);
                        if (bitsPerFrame > core_enc->avbr.mBitsDesiredFrame * thratio)
                        {
                            Ipp32s qp = H264_AVBR_GetQP(&core_enc->avbr, core_enc->m_PicType);
                            if (qp < 51)
                            {
                                Ipp32s qp_new = (Ipp32s)(qp * sqrt((Ipp64f)bitsPerFrame / (thratio * core_enc->avbr.mBitsDesiredFrame)));
                                if (qp_new <= qp)
                                    qp_new++;
                                h264_Clip(qp_new, 1, 51);
                                H264_AVBR_SetQP(&core_enc->avbr, core_enc->m_PicType, qp_new);
                                brcRecode = true;
                                goto recode_check;
                            }
                        }
                    }
                    H264_AVBR_PostFrame(&core_enc->avbr, core_enc->m_PicType, bitsPerFrame);
                }
#ifdef SLICE_CHECK_LIMIT
            }
            else
            {
                if (core_enc->m_info.rate_controls.method == H264_RCM_VBR || core_enc->m_info.rate_controls.method == H264_RCM_CBR)
                {
                    bitsPerFrame = (Ipp32s)((dst->GetDataSize() - data_size) << 3);
                    H264_AVBR_PostFrame(&core_enc->avbr, core_enc->m_PicType, bitsPerFrame);
                }
            }
#endif // SLICE_CHECK_LIMIT
            //Deblocking all frame/field
#ifdef DEBLOCK_THREADING
            if( ePic_Class != DISPOSABLE_PIC )
            {
                //Current we assume the same slice type for all slices
                H264CoreEncoder_DeblockingFunction pDeblocking = NULL;
                switch (default_slice_type){
                    case INTRASLICE:
                        pDeblocking = &H264ENC_MAKE_NAME(H264CoreEncoder_DeblockMacroblockISlice);
                        break;
                    case PREDSLICE:
                        pDeblocking = &H264ENC_MAKE_NAME(H264CoreEncoder_DeblockMacroblockPSlice);
                        break;
                    case BPREDSLICE:
                        pDeblocking = &H264ENC_MAKE_NAME(H264CoreEncoder_DeblockMacroblockBSlice);
                        break;
                    default:
                        pDeblocking = NULL;
                        break;
                }

                Ipp32s mbstr;
                //Reset table
#pragma omp parallel for private(mbstr)
                for( mbstr=0; mbstr<core_enc->m_HeightInMBs; mbstr++ ){
                    memset( (void*)(core_enc->mbs_deblock_ready + mbstr*core_enc->m_WidthInMBs), 0, sizeof(Ipp8u)*core_enc->m_WidthInMBs);
                }
#pragma omp parallel for private(mbstr) schedule(static,1)
                for( mbstr=0; mbstr<core_enc->m_HeightInMBs; mbstr++ ){
                    //Lock string
                    Ipp32s first_mb = mbstr*core_enc->m_WidthInMBs;
                    Ipp32s last_mb = first_mb + core_enc->m_WidthInMBs;
                    for( Ipp32s mb=first_mb; mb<last_mb; mb++ ){
                        //Check up mb is ready
                        Ipp32s upmb = mb - core_enc->m_WidthInMBs + 1;
                        if( mb == last_mb-1 ) upmb = mb - core_enc->m_WidthInMBs;
                        if( upmb >= 0 )
                            while( *(core_enc->mbs_deblock_ready + upmb) == 0 );
                         (*pDeblocking)(state, mb);
                         //Unlock ready MB
                         *(core_enc->mbs_deblock_ready + mb) = 1;
                    }
                }
            }
#else
            for (slice = (Ipp32s)core_enc->m_info.num_slices*core_enc->m_field_index; slice < core_enc->m_info.num_slices*(core_enc->m_field_index+1); slice++){
                if (core_enc->m_Slices[slice].status != UMC_OK){
                    // It is unreachable in the current implementation, so there is no problem!!!
                    core_enc->m_bMakeNextFrameKey = true;
                    VM_ASSERT(0);// goto done;
                }else if(ePic_Class != DISPOSABLE_PIC){
                    H264ENC_MAKE_NAME(H264CoreEncoder_DeblockSlice)(state, core_enc->m_Slices + slice, core_enc->m_Slices[slice].m_first_mb_in_slice + core_enc->m_WidthInMBs*core_enc->m_HeightInMBs*core_enc->m_field_index, core_enc->m_Slices[slice].m_MB_Counter);
                }
            }
#endif
end_of_frame:
            if (ePic_Class != DISPOSABLE_PIC)
                H264ENC_MAKE_NAME(H264CoreEncoder_End_Picture)(state);
            core_enc->m_HeightInMBs <<= (Ipp8u)(core_enc->m_pCurrentFrame->m_PictureStructureForDec < FRM_STRUCTURE); //Do we need it here?
#if defined (ALPHA_BLENDING_H264)
            } while(!alpha && core_enc->m_SeqParamSet.aux_format_idc );
            if(alpha){
//                core_enc->m_pCurrentFrame->usePrimary();
                H264ENC_MAKE_NAME(H264EncoderFrameList_switchToPrimary)(&core_enc->m_cpb);
                H264ENC_MAKE_NAME(H264EncoderFrameList_switchToPrimary)(&core_enc->m_dpb);
            }
#endif // ALPHA_BLENDING_H264
//            for(slice = 0; slice < core_enc->m_info.num_slices*((core_enc->m_info.coding_type == 1) + 1); slice++)  //TODO fix for PicAFF/AFRM
//                bitsPerFrame += core_enc->m_pbitstreams[slice]->GetBsOffset();
            if (ePic_Class != DISPOSABLE_PIC)
                H264ENC_MAKE_NAME(H264CoreEncoder_UpdateRefPicMarking)(state);
        }
#if 0
        fprintf(stderr, "%c\t%d\t%d\n", (ePictureType == INTRAPIC) ? 'I' : (ePictureType == PREDPIC) ? 'P' : 'B', (Ipp32s)core_enc->m_total_bits_encoded, H264_AVBR_GetQP(&core_enc->avbr, core_enc->m_PicType));
#endif
recode_check:
        if( (core_enc->m_Analyse & ANALYSE_RECODE_FRAME) && ( bufferOverflowFlag || !buffersNotFull || brcRecode) ){ //check frame size to be less than output buffer otherwise change qp and recode frame
            if (!brcRecode) {
                Ipp32s qp = H264_AVBR_GetQP(&core_enc->avbr, core_enc->m_PicType);
                if( qp == 51 ){
                    status = UMC::UMC_ERR_NOT_ENOUGH_BUFFER;
                    goto done;
                }
              H264_AVBR_SetQP(&core_enc->avbr, core_enc->m_PicType, ++qp);
            }
            bufferOverflowFlag = false;
            buffersNotFull = true;
            brcRecode = false;
            if (ePic_Class == IDR_PIC && (!core_enc->m_field_index)) {
              core_enc->m_SliceHeader.idr_pic_id--;
              core_enc->m_SliceHeader.idr_pic_id &= 0xff; //Restrict to 255 to reduce number of bits(max value 65535 in standard)
            }
            dst->SetDataSize(data_size);
        } else
            break;
    };
    bitsPerFrame = (Ipp32s)((dst->GetDataSize() - data_size) << 3);
    core_enc->m_total_bits_encoded += bitsPerFrame;

#ifdef ALT_RC
    Ipp64f MAD = 0;
    Ipp32s textureBits = 0;
    for (slice=0; slice < core_enc->m_info.num_slices; slice++)
    {
        textureBits += core_enc->m_Slices[slice].m_Texture_Sum;
        MAD += core_enc->m_Slices[slice].m_Sad_Sum;
    }
    MAD /= (core_enc->m_info.info.clip_info.width * core_enc->m_info.info.clip_info.height);
    if (core_enc->m_info.rate_controls.method == H264_RCM_VBR || core_enc->m_info.rate_controls.method == H264_RCM_CBR) {
        H264_AVBR_PostFrame_AltRC(&core_enc->avbr, core_enc->m_PicType, (Ipp32s)core_enc->m_total_bits_encoded, MAD, textureBits);
    }
#else
/*
    if (core_enc->m_info.rate_controls.method == H264_RCM_VBR || core_enc->m_info.rate_controls.method == H264_RCM_CBR)
        H264_AVBR_PostFrame(&core_enc->avbr, core_enc->m_PicType, bitsPerFrame);
*/
#endif // ALT_RC
    if( core_enc->m_Analyse & ANALYSE_RECODE_FRAME ){
        H264ENC_MAKE_NAME(H264EncoderFrame_exchangeFrameYUVPointers)(core_enc->m_pReconstructFrame, core_enc->m_pCurrentFrame);
        H264ENC_CALL_DELETE(H264EncoderFrame, core_enc->m_pReconstructFrame);
        core_enc->m_pReconstructFrame = NULL;
    }
    if (dst->GetDataSize() == 0) {
        core_enc->m_bMakeNextFrameKey = true;
        status = UMC_ERR_FAILED;
        goto done;
    }
    core_enc->m_uFrames_Num++;
done:
    return status;
}

//
// move all frames in WaitingForRef to ReadyToEncode
//
Status H264ENC_MAKE_NAME(H264CoreEncoder_MoveFromCPBToDPB)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
 //   EnumPicCodType  ePictureType;
    H264ENC_MAKE_NAME(H264EncoderFrameList_RemoveFrame)(&core_enc->m_cpb, core_enc->m_pCurrentFrame);
    H264ENC_MAKE_NAME(H264EncoderFrameList_insertAtCurrent)(&core_enc->m_dpb, core_enc->m_pCurrentFrame);
    return UMC_OK;
}

Status H264ENC_MAKE_NAME(H264CoreEncoder_CleanDPB)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    H264EncoderFrameType *pFrm = H264ENC_MAKE_NAME(H264EncoderFrameList_findNextDisposable)(&core_enc->m_dpb);
    //   EnumPicCodType  ePictureType;
    Status      ps = UMC_OK;
    while (pFrm != NULL)
    {
        H264ENC_MAKE_NAME(H264EncoderFrameList_RemoveFrame)(&core_enc->m_dpb, pFrm);
        H264ENC_MAKE_NAME(H264EncoderFrameList_insertAtCurrent)(&core_enc->m_cpb, pFrm);
        pFrm = H264ENC_MAKE_NAME(H264EncoderFrameList_findNextDisposable)(&core_enc->m_dpb);
    }
    return ps;
}

/*************************************************************
 *  Name: SetSequenceParameters
 *  Description:  Fill in the Sequence Parameter Set for this
 *  sequence.  Can only change at an IDR picture.
 *************************************************************/
void H264ENC_MAKE_NAME(H264CoreEncoder_SetSequenceParameters)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    core_enc->m_SeqParamSet.profile_idc = core_enc->m_info.profile_idc;

    // We don't meet any of these contraints yet
    core_enc->m_SeqParamSet.constraint_set0_flag = 0;
    core_enc->m_SeqParamSet.constraint_set1_flag = 1;
    core_enc->m_SeqParamSet.constraint_set2_flag = 0;
    core_enc->m_SeqParamSet.constraint_set3_flag = 0;

    core_enc->m_SeqParamSet.seq_parameter_set_id = 0;

    // Frame numbers are mod 256.
    core_enc->m_SeqParamSet.log2_max_frame_num = 8;

    // Setup pic_order_cnt_type based on use of B frames.
    // Note! pic_order_cnt_type == 1 is not implemented

    // The following are not transmitted in either case below, and are
    // just initialized here to be nice.
    core_enc->m_SeqParamSet.delta_pic_order_always_zero_flag = 0;
    core_enc->m_SeqParamSet.offset_for_non_ref_pic = 0;
    core_enc->m_SeqParamSet.poffset_for_ref_frame = NULL;
    core_enc->m_SeqParamSet.num_ref_frames_in_pic_order_cnt_cycle = 0;

    if (core_enc->m_info.B_frame_rate == 0 && core_enc->m_info.coding_type == 0)
    {
        core_enc->m_SeqParamSet.pic_order_cnt_type = 2;
        core_enc->m_SeqParamSet.log2_max_pic_order_cnt_lsb = 0;
        // Right now this only supports simple P frame patterns (e.g. H264PPPP...)
    } else {
        //Ipp32s log2_max_poc = (Ipp32u)log(((Ipp64f)core_enc->m_info.B_frame_rate +
        //    core_enc->m_info.num_ref_to_start_code_B_slice)/log((Ipp64f)2) + 1) << 1;
        Ipp32s log2_max_poc = (Ipp32s) (log((Ipp64f)((core_enc->m_info.B_frame_rate<<((core_enc->m_info.treat_B_as_reference==2)?1:0))+ core_enc->m_info.num_ref_frames))
                               / log(2.0)) + 3; // 3=1+1+1=round+multiply by 2 in counting+devide by 2 in comparison

        core_enc->m_SeqParamSet.log2_max_pic_order_cnt_lsb = IPP_MAX(log2_max_poc, 4);

        if (core_enc->m_SeqParamSet.log2_max_pic_order_cnt_lsb > 16)
        {
            VM_ASSERT(false);
            core_enc->m_SeqParamSet.log2_max_pic_order_cnt_lsb = 16;
        }

        core_enc->m_SeqParamSet.pic_order_cnt_type = 0;
        // Right now this only supports simple B frame patterns (e.g. IBBPBBP...)
    }
    core_enc->m_SeqParamSet.num_ref_frames = core_enc->m_info.num_ref_frames;

    // Note!  NO code after this point supports pic_order_cnt_type == 1
    // Always zero because we don't support field encoding
    core_enc->m_SeqParamSet.offset_for_top_to_bottom_field = 0;

    core_enc->m_SeqParamSet.frame_mbs_only_flag = (core_enc->m_info.coding_type)? 0: 1;

    core_enc->m_SeqParamSet.gaps_in_frame_num_value_allowed_flag = 0;
    core_enc->m_SeqParamSet.mb_adaptive_frame_field_flag = core_enc->m_info.coding_type>1;

    // If set to 1, 8x8 blocks in Direct Mode always use 1 MV,
    // obtained from the "outer corner" 4x4 block, regardless
    // of how the CoLocated 8x8 is split into subblocks.  If this
    // is 0, then the 8x8 in Direct Mode is subdivided exactly as
    // the Colocated 8x8, with the appropriate number of derived MVs.
    core_enc->m_SeqParamSet.direct_8x8_inference_flag =
        core_enc->m_info.use_direct_inference || !core_enc->m_SeqParamSet.frame_mbs_only_flag ? 1 : 0;

    // Picture Dimensions in MBs
    core_enc->m_SeqParamSet.frame_width_in_mbs = ((core_enc->m_info.info.clip_info.width+15)>>4);
    core_enc->m_SeqParamSet.frame_height_in_mbs = ((core_enc->m_info.info.clip_info.height+(16<<(1 - core_enc->m_SeqParamSet.frame_mbs_only_flag)) - 1)>>4) >> (1 - core_enc->m_SeqParamSet.frame_mbs_only_flag);
    Ipp32s frame_height_in_mbs = core_enc->m_SeqParamSet.frame_height_in_mbs << (1 - core_enc->m_SeqParamSet.frame_mbs_only_flag);

    // If the width & height in MBs doesn't match the image dimensions then do
    // some cropping in the decoder
    if (((core_enc->m_SeqParamSet.frame_width_in_mbs<<4) != core_enc->m_info.info.clip_info.width) ||
        ((frame_height_in_mbs << 4) != core_enc->m_info.info.clip_info.height)) {
        core_enc->m_SeqParamSet.frame_cropping_flag = 1;
        core_enc->m_SeqParamSet.frame_crop_left_offset = 0;
        core_enc->m_SeqParamSet.frame_crop_right_offset =
            ((core_enc->m_SeqParamSet.frame_width_in_mbs<<4) - core_enc->m_info.info.clip_info.width)/UMC_H264_ENCODER::SubWidthC[core_enc->m_SeqParamSet.chroma_format_idc];
        core_enc->m_SeqParamSet.frame_crop_top_offset = 0;
        core_enc->m_SeqParamSet.frame_crop_bottom_offset =
            ((frame_height_in_mbs<<4) - core_enc->m_info.info.clip_info.height)/(UMC_H264_ENCODER::SubHeightC[core_enc->m_SeqParamSet.chroma_format_idc]*(2 - core_enc->m_SeqParamSet.frame_mbs_only_flag));
    } else {
        core_enc->m_SeqParamSet.frame_cropping_flag = 0;
        core_enc->m_SeqParamSet.frame_crop_left_offset = 0;
        core_enc->m_SeqParamSet.frame_crop_right_offset = 0;
        core_enc->m_SeqParamSet.frame_crop_top_offset = 0;
        core_enc->m_SeqParamSet.frame_crop_bottom_offset = 0;
    }

    core_enc->m_SeqParamSet.vui_parameters_present_flag = 0;

    core_enc->m_SeqParamSet.level_idc = core_enc->m_info.level_idc;

    core_enc->m_SeqParamSet.profile_idc                    = core_enc->m_info.profile_idc;
    core_enc->m_SeqParamSet.chroma_format_idc              = (Ipp8s)core_enc->m_info.chroma_format_idc;
    core_enc->m_SeqParamSet.bit_depth_luma                 = core_enc->m_info.bit_depth_luma;
    core_enc->m_SeqParamSet.bit_depth_chroma               = core_enc->m_info.bit_depth_chroma;
    core_enc->m_SeqParamSet.qpprime_y_zero_transform_bypass_flag = core_enc->m_info.qpprime_y_zero_transform_bypass_flag;
    core_enc->m_SeqParamSet.seq_scaling_matrix_present_flag = false;

    core_enc->m_SeqParamSet.bit_depth_aux                  = core_enc->m_info.bit_depth_aux;
    core_enc->m_SeqParamSet.alpha_incr_flag                = core_enc->m_info.alpha_incr_flag;
    core_enc->m_SeqParamSet.alpha_opaque_value             = core_enc->m_info.alpha_opaque_value;
    core_enc->m_SeqParamSet.alpha_transparent_value        = core_enc->m_info.alpha_transparent_value;
    core_enc->m_SeqParamSet.aux_format_idc                 = core_enc->m_info.aux_format_idc;

    if(   core_enc->m_SeqParamSet.bit_depth_aux != 8
       || core_enc->m_SeqParamSet.alpha_incr_flag != 0
       || core_enc->m_SeqParamSet.alpha_opaque_value != 0
       || core_enc->m_SeqParamSet.alpha_transparent_value != 0
       || core_enc->m_SeqParamSet.aux_format_idc != 0)
    {
        core_enc->m_SeqParamSet.pack_sequence_extension = 1;
    }

    // Precalculate these values so we have them for later (repeated) use.
    core_enc->m_SeqParamSet.MaxMbAddress = (core_enc->m_SeqParamSet.frame_width_in_mbs * frame_height_in_mbs) - 1;
    H264ENC_MAKE_NAME(H264CoreEncoder_SetDPBSize)(state);

    //Scaling matrices
    if( core_enc->m_info.use_default_scaling_matrix){
            Ipp32s i;
            //setup matrices that will be used
            core_enc->m_SeqParamSet.seq_scaling_matrix_present_flag = true;
            // 4x4 matrices
            for( i=0; i<6; i++ ) core_enc->m_SeqParamSet.seq_scaling_list_present_flag[i] = true;
            //Copy default
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[0], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[1], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[2], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[3], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[4], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[5], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));

            // 8x8 matrices
            core_enc->m_SeqParamSet.seq_scaling_list_present_flag[6] = true;
            core_enc->m_SeqParamSet.seq_scaling_list_present_flag[7] = true;

            //Copy default scaling matrices
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_8x8[0], UMC_H264_ENCODER::DefaultScalingList8x8[1], 64*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_8x8[1], UMC_H264_ENCODER::DefaultScalingList8x8[1], 64*sizeof(Ipp8u));
        }else{
            core_enc->m_SeqParamSet.seq_scaling_matrix_present_flag = false;
            //Copy default
/*            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[0], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[1], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[2], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[3], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[4], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_4x4[5], UMC_H264_ENCODER::FlatScalingList4x4, 16*sizeof(Ipp8u));
*/
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_8x8[0], UMC_H264_ENCODER::FlatScalingList8x8, 64*sizeof(Ipp8u));
            memcpy(core_enc->m_SeqParamSet.seq_scaling_list_8x8[1], UMC_H264_ENCODER::FlatScalingList8x8, 64*sizeof(Ipp8u));
        }

     //Generate new scaling matrices for use in transform
      Ipp32s qp_rem,i;
/*     for( i=0; i < 6; i++ )
         for( qp_rem = 0; qp_rem<6; qp_rem++ ) {
             ippiGenScaleLevel4x4_H264_8u16s_D2( core_enc->m_SeqParamSet.seq_scaling_list_4x4[i],
                                     core_enc->m_SeqParamSet.seq_scaling_inv_matrix_4x4[i][qp_rem],
                                     core_enc->m_SeqParamSet.seq_scaling_matrix_4x4[i][qp_rem],
                                     qp_rem );

        }
*/
        for( i=0; i<2; i++)
            for( qp_rem=0; qp_rem<6; qp_rem++ )
                ippiGenScaleLevel8x8_H264_8u16s_D2(core_enc->m_SeqParamSet.seq_scaling_list_8x8[i],
                                        8,
                                        core_enc->m_SeqParamSet.seq_scaling_inv_matrix_8x8[i][qp_rem],
                                        core_enc->m_SeqParamSet.seq_scaling_matrix_8x8[i][qp_rem],
                                        qp_rem);

        //VUI parameters
        core_enc->m_SeqParamSet.vui_parameters_present_flag = 1;
        core_enc->m_SeqParamSet.vui_parameters.aspect_ratio_info_present_flag = 1;
        core_enc->m_SeqParamSet.vui_parameters.aspect_ratio_idc = 1;
        if(core_enc->m_info.info.framerate != 0)
        {
#ifdef AMD_REMOVETIMINGINFO
            core_enc->m_SeqParamSet.vui_parameters.timing_info_present_flag = 1;
            core_enc->m_SeqParamSet.vui_parameters.num_units_in_tick = 1;
            core_enc->m_SeqParamSet.vui_parameters.time_scale = 
              2*core_enc->m_SeqParamSet.vui_parameters.num_units_in_tick * (Ipp32u)core_enc->m_info.info.framerate;
            core_enc->m_SeqParamSet.vui_parameters.fixed_frame_rate_flag = 1;
#endif
        }

} // SetSequenceParameters

/*************************************************************
 *  Name: SetPictureParameters
 *  Description:  Fill in the Picture Parameter Set for this
 *  sequence.  Can only change at an IDR picture.
 *************************************************************/
void H264ENC_MAKE_NAME(H264CoreEncoder_SetPictureParameters)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    core_enc->m_PicParamSet.pic_parameter_set_id = 0;

    // Assumes that there is only one sequence param set to choose from
    core_enc->m_PicParamSet.seq_parameter_set_id = core_enc->m_SeqParamSet.seq_parameter_set_id;

    core_enc->m_PicParamSet.entropy_coding_mode = core_enc->m_info.entropy_coding_mode;

    core_enc->m_PicParamSet.pic_order_present_flag = (core_enc->m_SeqParamSet.frame_mbs_only_flag == 0);

    core_enc->m_PicParamSet.weighted_pred_flag = 0;

    // We use implicit weighted prediction (2) when B frame rate can
    // benefit from it.  When B_Frame_rate == 0 or 1, it doesn't matter,
    // so we do what is faster (0).

    core_enc->m_PicParamSet.weighted_bipred_idc = core_enc->m_info.use_implicit_weighted_bipred ? 2 : 0;

    // Default to P frame constant quality at time of an IDR
    if (core_enc->m_info.rate_controls.method == H264_RCM_VBR ||
        core_enc->m_info.rate_controls.method == H264_RCM_CBR ||
        core_enc->m_info.rate_controls.method == H264_RCM_VBR_SLICE ||
        core_enc->m_info.rate_controls.method == H264_RCM_CBR_SLICE
        ){
        //core_enc->m_PicParamSet.pic_init_qp = (Ipp8s)H264_AVBR_GetQP(&core_enc->avbr, INTRAPIC);
        core_enc->m_PicParamSet.pic_init_qp = 26;
    }else if(core_enc->m_info.rate_controls.method == H264_RCM_QUANT){
        core_enc->m_PicParamSet.pic_init_qp = core_enc->m_info.rate_controls.quantP;
    }
    core_enc->m_PicParamSet.pic_init_qs = 26;     // Not used

    core_enc->m_PicParamSet.chroma_qp_index_offset = 0;
    core_enc->m_PicParamSet.deblocking_filter_variables_present_flag = 1;

    core_enc->m_PicParamSet.constrained_intra_pred_flag = 0;

    // We don't do redundant slices...
    core_enc->m_PicParamSet.redundant_pic_cnt_present_flag = 0;
    core_enc->m_PicParamSet.pic_scaling_matrix_present_flag = 0;
    core_enc->m_PicParamSet.transform_8x8_mode_flag = false;
    // In the future, if flexible macroblock ordering is
    // desired, then a macroblock allocation map will need
    // to be coded and the value below updated accordingly.
    core_enc->m_PicParamSet.num_slice_groups = 1;     // Hard coded for now
    core_enc->m_PicParamSet.SliceGroupInfo.slice_group_map_type = 0;
    core_enc->m_PicParamSet.SliceGroupInfo.t3.pic_size_in_map_units = 0;
    core_enc->m_PicParamSet.SliceGroupInfo.t3.pSliceGroupIDMap = NULL;

    // I guess these need to be 1 or greater since they are written as "minus1".
    core_enc->m_PicParamSet.num_ref_idx_l0_active = 1;
    core_enc->m_PicParamSet.num_ref_idx_l1_active = 1;

    core_enc->m_PicParamSet.transform_8x8_mode_flag = core_enc->m_info.transform_8x8_mode_flag;

} // SetPictureParameters

/*************************************************************
 *  Name: SetSliceHeaderCommon
 *  Description:  Given the ePictureType and core_enc->m_info
 *                fill in the slice header for this slice
 ************************************************************/
void H264ENC_MAKE_NAME(H264CoreEncoder_SetSliceHeaderCommon)(
    void* state,
    H264EncoderFrameType* pCurrentFrame )
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32s frame_num = pCurrentFrame->m_FrameNum;
    // Right now, we always work with frame nums modulo 256
    core_enc->m_SliceHeader.frame_num = frame_num % core_enc->m_uTRWrapAround;

    // Assumes there is only one picture parameter set to choose from
    core_enc->m_SliceHeader.pic_parameter_set_id = core_enc->m_PicParamSet.pic_parameter_set_id;

    core_enc->m_SliceHeader.field_pic_flag = (Ipp8u)(pCurrentFrame->m_PictureStructureForDec<FRM_STRUCTURE);
    core_enc->m_SliceHeader.bottom_field_flag = (Ipp8u)(pCurrentFrame->m_bottom_field_flag[core_enc->m_field_index]);
    core_enc->m_SliceHeader.MbaffFrameFlag = (core_enc->m_SeqParamSet.mb_adaptive_frame_field_flag)&&(!core_enc->m_SliceHeader.field_pic_flag);
    core_enc->m_SliceHeader.delta_pic_order_cnt_bottom = core_enc->m_SeqParamSet.frame_mbs_only_flag == 0;

    //core_enc->m_TopPicOrderCnt = core_enc->m_PicOrderCnt;
    //core_enc->m_BottomPicOrderCnt = core_enc->m_TopPicOrderCnt + 1; // ????

    if (core_enc->m_SeqParamSet.pic_order_cnt_type == 0) {
        core_enc->m_SliceHeader.pic_order_cnt_lsb = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(
            core_enc->m_pCurrentFrame,
            core_enc->m_field_index,
            0) & ~(0xffffffff << core_enc->m_SeqParamSet.log2_max_pic_order_cnt_lsb);
    }
    core_enc->m_SliceHeader.adaptive_ref_pic_marking_mode_flag = 0;

    for(Ipp32s i = 0; i < core_enc->m_info.num_slices*((core_enc->m_pCurrentFrame->m_PictureStructureForDec<FRM_STRUCTURE)+1); i++) { //TODO fix for PicAFF/AFRM
        H264SliceType *curr_slice = core_enc->m_Slices + i;
        curr_slice->num_ref_idx_l0_active = core_enc->m_PicParamSet.num_ref_idx_l0_active;
        curr_slice->num_ref_idx_l1_active = core_enc->m_PicParamSet.num_ref_idx_l1_active;

        curr_slice->num_ref_idx_active_override_flag =
            (
            (curr_slice->num_ref_idx_l0_active != core_enc->m_PicParamSet.num_ref_idx_l0_active)
            || (curr_slice->num_ref_idx_l1_active != core_enc->m_PicParamSet.num_ref_idx_l1_active)
        );

        curr_slice->m_disable_deblocking_filter_idc = core_enc->m_info.deblocking_filter_idc;
    }
}

/*************************************************************
 *  Name:         encodeFrameHeader
 *  Description:  Write out the frame header to the bit stream.
 ************************************************************/
Status H264ENC_MAKE_NAME(H264CoreEncoder_encodeFrameHeader)(
    void* state,
    H264BsRealType* bs,
    MediaData* dst,
    bool bIDR_Pic,
    bool& startPicture )
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Status ps = UMC_OK;

    // First, write a access unit delimiter for the frame.
    if (core_enc->m_info.write_access_unit_delimiters)
    {
        ps = H264ENC_MAKE_NAME(H264BsReal_PutPicDelimiter)(bs, core_enc->m_PicType);

        H264BsBase_WriteTrailingBits(&bs->m_base);

        // Copy PicDelimiter RBSP to the end of the output buffer after
        // Adding start codes and SC emulation prevention.
        dst->SetDataSize(
            dst->GetDataSize() +
                H264ENC_MAKE_NAME(H264BsReal_EndOfNAL)(
                    bs,
                    (Ipp8u*)dst->GetDataPointer() + dst->GetDataSize(),
                    0,
                    NAL_UT_AUD,
                    startPicture));
    }

    // If this is an IDR picture, write the seq and pic parameter sets
    if (bIDR_Pic)
    {
        // Write the seq_parameter_set_rbsp
        ps = H264ENC_MAKE_NAME(H264BsReal_PutSeqParms)(bs, core_enc->m_SeqParamSet);

        H264BsBase_WriteTrailingBits(&bs->m_base);

        // Copy Sequence Parms RBSP to the end of the output buffer after
        // Adding start codes and SC emulation prevention.
        dst->SetDataSize(
            dst->GetDataSize() +
                H264ENC_MAKE_NAME(H264BsReal_EndOfNAL)(
                    bs,
                    (Ipp8u*)dst->GetDataPointer() + dst->GetDataSize(),
                    1,
                    NAL_UT_SPS,
                    startPicture));

        if(core_enc->m_SeqParamSet.pack_sequence_extension) {
            // Write the seq_parameter_set_extension_rbsp when needed.
            ps = H264ENC_MAKE_NAME(H264BsReal_PutSeqExParms)(bs, core_enc->m_SeqParamSet);

            H264BsBase_WriteTrailingBits(&bs->m_base);

            // Copy Sequence Parms RBSP to the end of the output buffer after
            // Adding start codes and SC emulation prevention.
            dst->SetDataSize(
                dst->GetDataSize() +
                    H264ENC_MAKE_NAME(H264BsReal_EndOfNAL)(
                        bs,
                        (Ipp8u*)dst->GetDataPointer() + dst->GetDataSize(),
                        1,
                        NAL_UT_SEQEXT,
                        startPicture));
        }

        ps = H264ENC_MAKE_NAME(H264BsReal_PutPicParms)(
            bs,
            core_enc->m_PicParamSet,
            core_enc->m_SeqParamSet);

        H264BsBase_WriteTrailingBits(&bs->m_base);

        // Copy Picture Parms RBSP to the end of the output buffer after
        // Adding start codes and SC emulation prevention.
        dst->SetDataSize(
            dst->GetDataSize() +
                H264ENC_MAKE_NAME(H264BsReal_EndOfNAL)(
                    bs,
                    (Ipp8u*)dst->GetDataPointer() + dst->GetDataSize(),
                    1,
                    NAL_UT_PPS,
                    startPicture));

#ifdef PRESET_SEI
#include <ippversion.h>
        if( core_enc->m_uFrames_Num == 0 ){
            char buf[1024];
            H264EncoderParams* par = &core_enc->m_info;
            Ipp32s len;
#if defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)
#define snprintf _snprintf
#endif
            len = snprintf( buf, 1023, "INTEL H.264 ENCODER: IPP %d.%d.%d : GOP=%d IDR=%d B=%d Bref=%d Nref=%d Ns=%d RC=%d Brate=%d ME=%d Split=%d MEx=%d MEy=%d DirType=%d Q/S=%d OptQ=%d",
                IPP_VERSION_MAJOR, IPP_VERSION_MINOR, IPP_VERSION_BUILD,
                par->key_frame_controls.interval, par->key_frame_controls.idr_interval,
                par->B_frame_rate, par->treat_B_as_reference,
                par->num_ref_frames, par->num_slices,
                par->rate_controls.method, par->info.bitrate,
                par->mv_search_method, par->me_split_mode, par->me_search_x, par->me_search_y,
                par->direct_pred_mode,
                par->m_QualitySpeed, par->quant_opt_level
                );
            ps = H264ENC_MAKE_NAME(H264BsReal_PutSEI_UserDataUnregistred)( bs, buf, len );

            dst->SetDataSize(
                dst->GetDataSize() +
                    H264ENC_MAKE_NAME(H264BsReal_EndOfNAL)( bs, (Ipp8u*)dst->GetDataPointer() + dst->GetDataSize(), 0, NAL_UT_SEI, startPicture));
        }
#endif
    }

    return ps;
}

// ===========================================================================
//
// Apply the "Profile Rules" to this profile frame type
//
//
//      Option         Single Layer
// ------------   -------------------------
// Progressive
//      Stills
//                 Purge frames in the
//                 Ready and Wait queues.
//                 Reset the profile index.
//
//                -------------------------
// Key frame
//  interval       Wait for next 'P' in the
//                 profile to make an I frame
//                 Do process intervening B
//                 frames in the profile.
//                 No profile index reset.
//                 The key frame interval
//                 counter restarts after
//                 the key frame is sent.
//                -------------------------
// Forced Key
//      Frame      Regardless of the profile
//                 frame type; purge the
//                 the Ready queue.
//                 If destuctive key frame request
//                 then purge the Wait queue
//                 Reset profile index.
//
//                -------------------------
// switch B
// frame to P      If this profile frame type
//                 indicates a 'B' it will be
//                 en-queued, encoded and
//                 emitted as a P frame. Prior
//                 frames (P|B) in the queue
//                 are correctly emitted using
//                 the (new) P as a ref frame
//
// ------------   -------------------------
EnumPicCodType H264ENC_MAKE_NAME(H264CoreEncoder_DetermineFrameType)(
    void* state,
    Ipp32s)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    EnumPicCodType ePictureType = core_enc->eFrameType[core_enc->m_iProfileIndex];

    if (core_enc->m_uFrames_Num == 0)
    {
        core_enc->m_iProfileIndex++;
        if (core_enc->m_iProfileIndex == core_enc->profile_frequency)
            core_enc->m_iProfileIndex = 0;

        core_enc->m_bMakeNextFrameKey = false;
        return INTRAPIC;
    }

    // see if the layer's key frame interval has expired
    if (H264_KFCM_INTERVAL == core_enc->m_info.key_frame_controls.method)
    {
        // a value of one means every frame is a key frame
        // The interval can be expressed as: "Every Nth frame is a key frame"
        core_enc->m_uIntraFrameInterval--;
        if (1 == core_enc->m_uIntraFrameInterval)
        {
            core_enc->m_bMakeNextFrameKey = true;
        }
    }

    // change to an I frame
    if (core_enc->m_bMakeNextFrameKey)
    {
        ePictureType = INTRAPIC;

        // When a frame is forced INTRA, which happens internally for the
        // first frame, explicitly by the application, on a decode error
        // or for CPU scalability - the interval counter gets reset.
        if (H264_KFCM_INTERVAL == core_enc->m_info.key_frame_controls.method)
        {
            core_enc->m_uIntraFrameInterval = core_enc->m_info.key_frame_controls.interval + 1;
            core_enc->m_uIDRFrameInterval--;
            if (1 == core_enc->m_uIDRFrameInterval)
            {
                if (core_enc->m_iProfileIndex != 1 && core_enc->profile_frequency > 1) // if sequence PBBB was completed
                {   // no
                    ePictureType = PREDPIC;
                    core_enc->m_uIDRFrameInterval = 2;
                    core_enc->m_uIntraFrameInterval = 2;
                    core_enc->m_l1_cnt_to_start_B = 1;
                } else {
                    core_enc->m_bMakeNextFrameIDR = true;
                    core_enc->m_uIDRFrameInterval = core_enc->m_info.key_frame_controls.idr_interval + 1;
                    core_enc->m_iProfileIndex = 0;
                }
            } else {
                if (!core_enc->m_info.m_do_weak_forced_key_frames)
                {
                    core_enc->m_iProfileIndex = 0;
                }
            }
        }

        core_enc->m_bMakeNextFrameKey = false;
    }

    core_enc->m_iProfileIndex++;
    if (core_enc->m_iProfileIndex == core_enc->profile_frequency)
        core_enc->m_iProfileIndex = 0;

    return ePictureType;
}

/*************************************************************
 *  Name:         EncodeDummyFrame
 *  Description:  Writes out a blank frame to the bitstream in
                  case of buffer overflow.
 ************************************************************/
Status H264ENC_MAKE_NAME(H264CoreEncoder_EncodeDummyFrame)(
    void* state,
    MediaData* dst)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32s uMBRow, uMB;
    Status status = UMC_OK;
    bool startAccessUnit = true;

    H264ENC_MAKE_NAME(H264BsReal_Reset)(core_enc->m_bs1);

    H264ENC_MAKE_NAME(H264CoreEncoder_encodeFrameHeader)(state, core_enc->m_bs1, dst, false, startAccessUnit);

    for (uMBRow=0; uMBRow < core_enc->m_HeightInMBs; uMBRow++)
    {
        for (uMB=0; uMB < core_enc->m_WidthInMBs; uMB++)
        {
            if (core_enc->m_PicType == INTRAPIC)
            {
                H264ENC_MAKE_NAME(H264BsReal_PutBit)(core_enc->m_bs1, 1); // MBTYPE: INTRA_16x16
                H264ENC_MAKE_NAME(H264BsReal_PutBits)(core_enc->m_bs1, 0, 2); // AIC 16x16: DC
                H264ENC_MAKE_NAME(H264BsReal_PutBit)(core_enc->m_bs1, 1); // CBP: 0
                H264ENC_MAKE_NAME(H264BsReal_PutBit)(core_enc->m_bs1, 1); // 16x16 Coef: EOB
            }
            else if (core_enc->m_PicType == PREDPIC ||
                core_enc->m_PicType == BPREDPIC)

            {
                H264ENC_MAKE_NAME(H264BsReal_PutBit)(core_enc->m_bs1, 1); // skipped MB
            }
            else
            {
                status = UMC_ERR_FAILED;
                goto done;
            }

        }
    }

done:
    return status;
}

Status H264ENC_MAKE_NAME(H264CoreEncoder_Init)(
    void* state,
    BaseCodecParams* init,
    MemoryAllocator* pMemAlloc)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    H264EncoderParams *info = DynamicCast<H264EncoderParams, BaseCodecParams> (init);
    if(info == NULL) {
        VideoEncoderParams *VideoParams = DynamicCast<VideoEncoderParams, BaseCodecParams> (init);
        if(VideoParams == NULL) {
            return(UMC::UMC_ERR_INIT);
        }
        core_enc->m_info.info.clip_info.width       = VideoParams->info.clip_info.width;
        core_enc->m_info.info.clip_info.height      = VideoParams->info.clip_info.height;
        core_enc->m_info.info.bitrate         = VideoParams->info.bitrate;
        core_enc->m_info.info.framerate       = VideoParams->info.framerate;
        //core_enc->m_info.numFramesToEncode = VideoParams->numFramesToEncode;
        core_enc->m_info.numEncodedFrames  = VideoParams->numEncodedFrames;
        core_enc->m_info.numThreads      = VideoParams->numThreads;
        core_enc->m_info.qualityMeasure  = VideoParams->qualityMeasure;
        info = &core_enc->m_info;
    }else
        core_enc->m_info = *info;

    Status status = H264ENC_MAKE_NAME(H264CoreEncoder_CheckEncoderParameters)(state);
    if (status != UMC_OK)
        return status;
    H264ENC_MAKE_NAME(H264CoreEncoder_SetSequenceParameters)(state);
    core_enc->m_SubME_Algo = (core_enc->m_info.mv_search_method == 0) ? 0 : 2;
    core_enc->m_Analyse = ANALYSE_I_4x4 | ANALYSE_ME_SUBPEL | ANALYSE_CHECK_SKIP_INTPEL | ANALYSE_CHECK_SKIP_BESTCAND | ANALYSE_CHECK_SKIP_SUBPEL;
    if (core_enc->m_info.transform_8x8_mode_flag)
        core_enc->m_Analyse |= ANALYSE_I_8x8;
    core_enc->m_Analyse |= ANALYSE_FRAME_TYPE;
    //core_enc->m_Analyse |= ANALYSE_ME_CONTINUED_SEARCH;
    //core_enc->m_Analyse |= ANALYSE_FAST_INTRA; // QS <= 3
    //core_enc->m_Analyse |= ANALYSE_ME_SUBPEL_SAD; // QS <= 3
    //core_enc->m_Analyse |= ANALYSE_FLATNESS; // QS <= 1
    //core_enc->m_Analyse |= ANALYSE_INTRA_IN_ME; // QS <= 1
    //core_enc->m_Analyse |= ANALYSE_ME_FAST_MULTIREF; // QS <= 1
    //core_enc->m_Analyse |= ANALYSE_ME_EARLY_EXIT;
    //core_enc->m_Analyse |= ANALYSE_CBP_EMPTY;
    //core_enc->m_Analyse |= ANALYSE_SPLIT_SMALL_RANGE;
    //core_enc->m_Analyse |= ANALYSE_ME_EXT_CANDIDATES;
    core_enc->m_Analyse |= ANALYSE_RECODE_FRAME;
    if (core_enc->m_info.me_split_mode != 0)
        core_enc->m_Analyse |= ANALYSE_P_8x8 | ANALYSE_B_8x8;
    if (core_enc->m_info.me_split_mode > 1)
        core_enc->m_Analyse |= ANALYSE_P_4x4 | ANALYSE_B_4x4;
    if (core_enc->m_info.direct_pred_mode == 2)
        core_enc->m_Analyse |= ANALYSE_ME_AUTO_DIRECT;
    if (core_enc->m_info.m_QualitySpeed == 0)
        core_enc->m_Analyse |= ANALYSE_SAD;
    if (core_enc->m_info.m_QualitySpeed <= 1)
        core_enc->m_Analyse |= ANALYSE_FLATNESS | ANALYSE_INTRA_IN_ME | ANALYSE_ME_FAST_MULTIREF | ANALYSE_ME_PRESEARCH;
    if (core_enc->m_info.m_QualitySpeed >= 2)
        core_enc->m_Analyse |= ANALYSE_ME_CHROMA;
    if ((core_enc->m_info.m_QualitySpeed >= 2) && core_enc->m_info.entropy_coding_mode)
        core_enc->m_Analyse |= ANALYSE_RD_MODE;
    if ((core_enc->m_info.m_QualitySpeed >= 3) && core_enc->m_info.entropy_coding_mode)
        core_enc->m_Analyse |= ANALYSE_RD_OPT | ANALYSE_B_RD_OPT;
    if (core_enc->m_info.m_QualitySpeed <= 3)
        core_enc->m_Analyse |= ANALYSE_ME_SUBPEL_SAD | ANALYSE_FAST_INTRA;
    if (core_enc->m_info.m_QualitySpeed >= 4)
      core_enc->m_Analyse |= /*ANALYSE_B_RD_OPT |*/ ANALYSE_ME_BIDIR_REFINE;
    if (core_enc->m_info.m_QualitySpeed >= 5)
        core_enc->m_Analyse |= ANALYSE_ME_ALL_REF;
    if ((core_enc->m_Analyse & ANALYSE_RD_MODE) || (core_enc->m_Analyse & ANALYSE_RD_OPT))
        core_enc->m_Analyse |= ANALYSE_ME_CHROMA;
    if (core_enc->m_info.coding_type > 0) {
        if (core_enc->m_info.direct_pred_mode == 2) {
            core_enc->m_Analyse &= ~ANALYSE_ME_AUTO_DIRECT;
            core_enc->m_info.direct_pred_mode = 1;
        }
        //core_enc->m_Analyse &= ~ANALYSE_FRAME_TYPE;
    }
    //External memory allocator
    core_enc->memAlloc = pMemAlloc;
    core_enc->m_dpb.memAlloc = pMemAlloc;
    core_enc->m_cpb.memAlloc = pMemAlloc;

    Ipp32s numOfSliceEncs = core_enc->m_info.num_slices*((core_enc->m_info.coding_type == 1) + 1);
    H264ENC_CALL_NEW_ARR(status, H264Slice, core_enc->m_Slices, numOfSliceEncs); //TODO fix for PicAFF/AFRM
    if (status != UMC_OK)
        return status;
    Ipp32s i;
    for (i = 0; i < numOfSliceEncs; i++) { //TODO fix for PicAFF/AFRM
        status = H264ENC_MAKE_NAME(H264Slice_Init)(&core_enc->m_Slices[i], core_enc->m_info);
        if (status != UMC_OK)
            return status;
    }
    core_enc->profile_frequency = core_enc->m_info.B_frame_rate + 1;
    if (core_enc->eFrameSeq)
        H264_Free(core_enc->eFrameSeq);
    core_enc->eFrameSeq = (H264EncoderFrameType **)H264_Malloc((core_enc->profile_frequency + 1) * sizeof(H264EncoderFrameType *));
    for (i = 0; i <= core_enc->profile_frequency; i++)
        core_enc->eFrameSeq[i] = NULL;
    if (core_enc->eFrameType != NULL)
        H264_Free(core_enc->eFrameType);
    core_enc->eFrameType = (EnumPicCodType *)H264_Malloc(core_enc->profile_frequency * sizeof(EnumPicCodType));
    core_enc->eFrameType[0] = PREDPIC;
    for (i = 1; i < core_enc->profile_frequency; i++)
        core_enc->eFrameType[i] = BPREDPIC;

    // Set up for the 8 bit/default frame rate
    core_enc->m_uTRWrapAround = TR_WRAP;
    core_enc->m_uFrames_Num = 0;
    core_enc->m_uFrameCounter = 0;
    core_enc->m_PaddedSize.width  = (core_enc->m_info.info.clip_info.width  + 15) & ~15;
    core_enc->m_PaddedSize.height = (core_enc->m_info.info.clip_info.height + (16<<(1 - core_enc->m_SeqParamSet.frame_mbs_only_flag)) - 1) & ~((16<<(1 - core_enc->m_SeqParamSet.frame_mbs_only_flag)) - 1);
    core_enc->m_WidthInMBs = core_enc->m_PaddedSize.width >> 4;
    core_enc->m_HeightInMBs  = core_enc->m_PaddedSize.height >> 4;
    core_enc->m_Pitch = CalcPitchFromWidth(core_enc->m_PaddedSize.width, sizeof(PIXTYPE)) / sizeof(PIXTYPE);
    core_enc->m_bMakeNextFrameKey = true; // Ensure that we always start with a key frame.
    core_enc->m_bMakeNextFrameIDR = false;
    if (H264_KFCM_INTERVAL == core_enc->m_info.key_frame_controls.method) {
        core_enc->m_uIntraFrameInterval = core_enc->m_info.key_frame_controls.interval + 1;
        core_enc->m_uIDRFrameInterval = core_enc->m_info.key_frame_controls.idr_interval + 1;
    } else {
        core_enc->m_uIDRFrameInterval = core_enc->m_uIntraFrameInterval = 0;
    }
    memset(&core_enc->m_AdaptiveMarkingInfo,0,sizeof(core_enc->m_AdaptiveMarkingInfo));
    memset(&core_enc->m_ReorderInfoL0,0,sizeof(core_enc->m_ReorderInfoL0));
    memset(&core_enc->m_ReorderInfoL1,0,sizeof(core_enc->m_ReorderInfoL1));
    if ((core_enc->m_EmptyThreshold = (Ipp32u*)H264_Malloc(52*sizeof(Ipp32u))) == NULL)
        return UMC_ERR_ALLOC;
    if ((core_enc->m_DirectBSkipMEThres = (Ipp32u*)H264_Malloc(52*sizeof(Ipp32u))) == NULL)
        return UMC_ERR_ALLOC;
    if ((core_enc->m_PSkipMEThres = (Ipp32u*)H264_Malloc(52*sizeof(Ipp32u))) == NULL)
        return UMC_ERR_ALLOC;
    if ((core_enc->m_BestOf5EarlyExitThres = (Ipp32s*)H264_Malloc(52*sizeof(Ipp32s))) == NULL)
        return UMC_ERR_ALLOC;
    if (core_enc->m_SeqParamSet.qpprime_y_zero_transform_bypass_flag) {
        ippsZero_8u((Ipp8u*)core_enc->m_EmptyThreshold, 52 * sizeof(*core_enc->m_EmptyThreshold));
        ippsZero_8u((Ipp8u*)core_enc->m_DirectBSkipMEThres, 52 * sizeof(*core_enc->m_DirectBSkipMEThres));
        ippsZero_8u((Ipp8u*)core_enc->m_PSkipMEThres, 52 * sizeof(*core_enc->m_PSkipMEThres));
        ippsZero_8u((Ipp8u*)core_enc->m_BestOf5EarlyExitThres, 52 * sizeof(*core_enc->m_BestOf5EarlyExitThres));
    } else {
        ippsCopy_8u((const Ipp8u*)UMC_H264_ENCODER::EmptyThreshold, (Ipp8u*)core_enc->m_EmptyThreshold, 52 * sizeof(*core_enc->m_EmptyThreshold));
        ippsCopy_8u((const Ipp8u*)UMC_H264_ENCODER::DirectBSkipMEThres, (Ipp8u*)core_enc->m_DirectBSkipMEThres, 52 * sizeof(*core_enc->m_DirectBSkipMEThres));
        ippsCopy_8u((const Ipp8u*)UMC_H264_ENCODER::PSkipMEThres, (Ipp8u*)core_enc->m_PSkipMEThres, 52 * sizeof(*core_enc->m_PSkipMEThres));
        ippsCopy_8u((const Ipp8u*)UMC_H264_ENCODER::BestOf5EarlyExitThres, (Ipp8u*)core_enc->m_BestOf5EarlyExitThres, 52 * sizeof(*core_enc->m_BestOf5EarlyExitThres));
    }

    Ipp32u bsSize = core_enc->m_PaddedSize.width * core_enc->m_PaddedSize.height * sizeof(PIXTYPE);
    bsSize += (bsSize >> 1) + 4096;
    // TBD: see if buffer size can be reduced

    core_enc->m_pAllocEncoderInst = (Ipp8u*)H264_Malloc(numOfSliceEncs * bsSize + DATA_ALIGN);
    if (core_enc->m_pAllocEncoderInst == NULL)
        return UMC_ERR_ALLOC;
    core_enc->m_pBitStream = align_pointer<Ipp8u*>(core_enc->m_pAllocEncoderInst, DATA_ALIGN);

    core_enc->m_pbitstreams = (H264BsRealType**)H264_Malloc(numOfSliceEncs * sizeof(H264BsRealType*));
    if (core_enc->m_pbitstreams == NULL)
        return UMC_ERR_ALLOC;


    for (i = 0; i < numOfSliceEncs; i++) {
        core_enc->m_pbitstreams[i] = (H264BsRealType*)H264_Malloc(sizeof(H264BsRealType));
        if (!core_enc->m_pbitstreams[i])
            return UMC_ERR_ALLOC;
        H264ENC_MAKE_NAME(H264BsReal_Create)(core_enc->m_pbitstreams[i], core_enc->m_pBitStream + i * bsSize, bsSize, core_enc->m_info.chroma_format_idc, status);
        if (status != UMC_OK)
            return status;
        core_enc->m_Slices[i].m_pbitstream = (H264BsBase*)core_enc->m_pbitstreams[i];
    }
    core_enc->m_bs1 = core_enc->m_pbitstreams[0]; // core_enc->m_bs1 is the main stream.

    Ipp32s nMBCount = core_enc->m_WidthInMBs * core_enc->m_HeightInMBs;
#ifdef DEBLOCK_THREADING
    core_enc->mbs_deblock_ready = (Ipp8u*)H264_Malloc(sizeof(Ipp8u) * nMBCount);
    memset((void*)core_enc->mbs_deblock_ready, 0, sizeof( Ipp8u )*nMBCount);
#endif

#ifdef SLICE_THREADING_LOAD_BALANCING
    // Load balancing for slice level multithreading
    core_enc->m_B_ticks_per_macroblock = (Ipp64s*)H264_Malloc(sizeof(Ipp64s) * nMBCount);
    if (!core_enc->m_B_ticks_per_macroblock)
        return UMC_ERR_ALLOC;
    core_enc->m_P_ticks_per_macroblock = (Ipp64s*)H264_Malloc(sizeof(Ipp64s) * nMBCount);
    if (!core_enc->m_P_ticks_per_macroblock)
        return UMC_ERR_ALLOC;
    core_enc->m_B_ticks_data_available = 0;
    core_enc->m_P_ticks_data_available = 0;
#endif // SLICE_THREADING_LOAD_BALANCING

    Ipp32s len = (sizeof(H264MacroblockMVs) +
                  sizeof(H264MacroblockMVs) +
                  sizeof(H264MacroblockCoeffsInfo) +
                  sizeof(H264MacroblockLocalInfo) +
                  sizeof(H264MacroblockIntraTypes) +
                  sizeof(T_EncodeMBOffsets)
                 ) * nMBCount + ALIGN_VALUE * 6;
    core_enc->m_pParsedDataNew = (Ipp8u*)H264_Malloc(len);
    if (NULL == core_enc->m_pParsedDataNew)
        return UMC_ERR_ALLOC;
    core_enc->m_mbinfo.MVDeltas[0] = align_pointer<H264MacroblockMVs *> (core_enc->m_pParsedDataNew, ALIGN_VALUE);
    core_enc->m_mbinfo.MVDeltas[1] = align_pointer<H264MacroblockMVs *> (core_enc->m_mbinfo.MVDeltas[0] + nMBCount, ALIGN_VALUE);
    core_enc->m_mbinfo.MacroblockCoeffsInfo = align_pointer<H264MacroblockCoeffsInfo *> (core_enc->m_mbinfo.MVDeltas[1] + nMBCount, ALIGN_VALUE);
    core_enc->m_mbinfo.mbs = align_pointer<H264MacroblockLocalInfo *> (core_enc->m_mbinfo.MacroblockCoeffsInfo + nMBCount, ALIGN_VALUE);
    core_enc->m_mbinfo.intra_types = align_pointer<H264MacroblockIntraTypes *> (core_enc->m_mbinfo.mbs + nMBCount, ALIGN_VALUE);
    core_enc->m_pMBOffsets = align_pointer<T_EncodeMBOffsets*> (core_enc->m_mbinfo.intra_types  + nMBCount, ALIGN_VALUE);
    // Block offset -- initialize table, indexed by current block (0-23)
    // with the value to add to the offset of the block into the plane
    // to advance to the next block
    for (i = 0; i < 16; i++) {
        // 4 Cases to cover:
        if (!(i & 1)) {   // Even # blocks, next block to the right
            core_enc->m_EncBlockOffsetInc[0][i] = 4;
            core_enc->m_EncBlockOffsetInc[1][i] = 4;
        } else if (!(i & 2)) {  // (1,5,9 & 13), down and one to the left
            core_enc->m_EncBlockOffsetInc[0][i] = (core_enc->m_Pitch<<2) - 4;
            core_enc->m_EncBlockOffsetInc[1][i] = (core_enc->m_Pitch<<3) - 4;
        } else if (i == 7) { // beginning of next row
            core_enc->m_EncBlockOffsetInc[0][i] = (core_enc->m_Pitch<<2) - 12;
            core_enc->m_EncBlockOffsetInc[1][i] = (core_enc->m_Pitch<<3) - 12;
        } else { // (3 & 11) up and one to the right
            core_enc->m_EncBlockOffsetInc[0][i] = -(Ipp32s)((core_enc->m_Pitch<<2) - 4);
            core_enc->m_EncBlockOffsetInc[1][i] = -(Ipp32s)((core_enc->m_Pitch<<3) - 4);
        }
    }
    Ipp32s last_block = 16+(4<<core_enc->m_info.chroma_format_idc) - 1; // - last increment
    core_enc->m_EncBlockOffsetInc[0][last_block] = 0;
    core_enc->m_EncBlockOffsetInc[1][last_block] = 0;
    switch (core_enc->m_info.chroma_format_idc) {
        case 1:
        case 2:
            for (i = 16; i < last_block; i++){
                if (i & 1) {
                    core_enc->m_EncBlockOffsetInc[0][i] = (core_enc->m_Pitch<<2) - 4;
                    core_enc->m_EncBlockOffsetInc[1][i] = (core_enc->m_Pitch<<3) - 4;
                } else {
                    core_enc->m_EncBlockOffsetInc[0][i] = 4;
                    core_enc->m_EncBlockOffsetInc[1][i] = 4;
                }
            }
            break;
        case 3:
            //Copy from luma
            memcpy(&core_enc->m_EncBlockOffsetInc[0][16], &core_enc->m_EncBlockOffsetInc[0][0], 16*sizeof(Ipp32s));
            memcpy(&core_enc->m_EncBlockOffsetInc[0][32], &core_enc->m_EncBlockOffsetInc[0][0], 16*sizeof(Ipp32s));
            memcpy(&core_enc->m_EncBlockOffsetInc[1][16], &core_enc->m_EncBlockOffsetInc[1][0], 16*sizeof(Ipp32s));
            memcpy(&core_enc->m_EncBlockOffsetInc[1][32], &core_enc->m_EncBlockOffsetInc[1][0], 16*sizeof(Ipp32s));
            break;
    }
    core_enc->m_InitialOffsets[0][0] = core_enc->m_InitialOffsets[1][1] = 0;
    core_enc->m_InitialOffsets[0][1] = (Ipp32s)core_enc->m_Pitch;
    core_enc->m_InitialOffsets[1][0] = -(Ipp32s)core_enc->m_Pitch;

    Ipp32s bitDepth = IPP_MAX(core_enc->m_info.bit_depth_aux, IPP_MAX(core_enc->m_info.bit_depth_chroma, core_enc->m_info.bit_depth_luma));
    Ipp32s chromaSampling = core_enc->m_info.chroma_format_idc;
    Ipp32s alpha = core_enc->m_info.aux_format_idc ? 1 : 0;
    switch (core_enc->m_info.rate_controls.method) {
        case H264_RCM_CBR:
#ifdef ALT_RC
            H264_AVBR_Init_AltRC(&core_enc->avbr, core_enc->m_info.numFramesToEncode, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate, core_enc->m_info.info.clip_info.width * core_enc->m_info.info.clip_info.height, bitDepth, chromaSampling, alpha, core_enc->m_uIntraFrameInterval, core_enc->eFrameType, core_enc->m_info.B_frame_rate);
#else
            H264_AVBR_Init(&core_enc->avbr, 8, 4, 8, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate, core_enc->m_info.info.clip_info.width * core_enc->m_info.info.clip_info.height, bitDepth, chromaSampling, alpha);
#endif
            break;
        case H264_RCM_VBR:
#ifdef ALT_RC
            H264_AVBR_Init_AltRC(&core_enc->avbr, core_enc->m_info.numFramesToEncode, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate, core_enc->m_info.info.clip_info.width * core_enc->m_info.info.clip_info.height, bitDepth, chromaSampling, alpha, core_enc->m_uIntraFrameInterval, core_enc->eFrameType, core_enc->m_info.B_frame_rate);
#else
            H264_AVBR_Init(&core_enc->avbr, 0, 0, 0, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate, core_enc->m_info.info.clip_info.width * core_enc->m_info.info.clip_info.height, bitDepth, chromaSampling, alpha);
#endif
            break;
        case H264_RCM_CBR_SLICE:
            H264_AVBR_Init(&core_enc->avbr, 8, 4, 8, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate * core_enc->m_info.num_slices, core_enc->m_info.info.clip_info.width * core_enc->m_info.info.clip_info.height / core_enc->m_info.num_slices, bitDepth, chromaSampling, alpha);
            break;
        case H264_RCM_VBR_SLICE:
            H264_AVBR_Init(&core_enc->avbr, 0, 0, 0, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate * core_enc->m_info.num_slices, core_enc->m_info.info.clip_info.width * core_enc->m_info.info.clip_info.height / core_enc->m_info.num_slices, bitDepth, chromaSampling, alpha);
        default:
            break;
    }
    core_enc->m_iProfileIndex = 0;
    core_enc->cflags = core_enc->cnotes = 0;
    core_enc->m_pLastFrame = NULL; //Init pointer to last frame

    Ipp32s fs = core_enc->m_info.info.clip_info.width * core_enc->m_info.info.clip_info.height;
    core_enc->m_info.m_SuggestedOutputSize = fs;
    if (alpha)
        core_enc->m_info.m_SuggestedOutputSize += fs;
    if (chromaSampling == 1)
        core_enc->m_info.m_SuggestedOutputSize += fs / 2;
    else if (chromaSampling == 2)
        core_enc->m_info.m_SuggestedOutputSize += fs;
    else if (chromaSampling == 3)
        core_enc->m_info.m_SuggestedOutputSize += fs * 2;
    core_enc->m_info.m_SuggestedOutputSize = core_enc->m_info.m_SuggestedOutputSize * bitDepth / 8;
#ifdef FRAME_QP_FROM_FILE
        FILE* f;
        int qp,fn=0;
        char ft;

        if( (f = fopen(FRAME_QP_FROM_FILE, "r")) == NULL){
            fprintf(stderr,"Can't open file %s\n", FRAME_QP_FROM_FILE);
            exit(-1);
        }
        while(fscanf(f,"%c %d\n",&ft,&qp) == 2){
            frame_type.push_back(ft);
            frame_qp.push_back(qp);
            fn++;
        }
        fclose(f);
        //fprintf(stderr,"frames read = %d\n",fn);
#endif
    return UMC_OK;
}

Status H264ENC_MAKE_NAME(H264CoreEncoder_GetFrame)(
    void* state,
    MediaData *in,
    MediaData *out)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Status res = UMC_OK;

    if (in)
    {
        VideoData *vin = DynamicCast<VideoData ,MediaData> (in);
        if(!vin) {
            // Only VideoData compatible objects are allowed.
            return(UMC_ERR_INVALID_STREAM);
        }
#if defined (ALPHA_BLENDING_H264)
        if(core_enc->m_SeqParamSet.aux_format_idc != 0 && ( (core_enc->m_info.chroma_format_idc != 0 && vin->GetNumPlanes()<4) ||
            (core_enc->m_info.chroma_format_idc == 0 && vin->GetNumPlanes() != 2) )) {
                return(UMC_ERR_INVALID_STREAM);
        }
#endif // ALPHA_BLENDING_H264
        res = H264ENC_MAKE_NAME(H264CoreEncoder_Encode)(state, vin, out, core_enc->cflags, core_enc->cnotes);
        if (core_enc->cnotes & H264_ECN_NO_FRAME) return UMC_OK;
    }else{
        core_enc->cflags |= H264_ECF_LAST_FRAME;
        res = H264ENC_MAKE_NAME(H264CoreEncoder_Encode)(state, NULL, out, core_enc->cflags, core_enc->cnotes);
        if (core_enc->cnotes & H264_ECN_NO_FRAME)
            return UMC_ERR_END_OF_STREAM;
    }
    // Set FrameType
    if (NULL != out && UMC_OK == res) {
      FrameType frame_type = NONE_PICTURE;
      switch (core_enc->m_pCurrentFrame->m_PicCodType) {
        case INTRAPIC: frame_type = I_PICTURE; vm_debug_trace_s(VM_DEBUG_VERBOSE, "I_PICTURE"); break;
        case PREDPIC:  frame_type = P_PICTURE; vm_debug_trace_s(VM_DEBUG_VERBOSE, "P_PICTURE"); break;
        case BPREDPIC: frame_type = B_PICTURE; vm_debug_trace_s(VM_DEBUG_VERBOSE, "B_PICTURE"); break;
      }
      out->SetFrameType(frame_type);
    }
    return(res);
}

VideoData* H264ENC_MAKE_NAME(H264CoreEncoder_GetReconstructedFrame)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32s makeNULL = 0;
   //Deblock for non referece B frames
   if( core_enc->m_pCurrentFrame->m_PicCodType == BPREDPIC && !core_enc->m_pCurrentFrame->m_RefPic ){
    if( core_enc->m_Analyse & ANALYSE_RECODE_FRAME && core_enc->m_pReconstructFrame == NULL){
        core_enc->m_pReconstructFrame = core_enc->m_pCurrentFrame;
        makeNULL = 1;
    }
    Ipp32s field_index, slice;
    for (field_index=0;field_index<=(Ipp8u)(core_enc->m_pCurrentFrame->m_PictureStructureForDec<FRM_STRUCTURE);field_index++)
        for (slice = (Ipp32s)core_enc->m_info.num_slices*field_index; slice < core_enc->m_info.num_slices*(field_index+1); slice++)
            H264ENC_MAKE_NAME(H264CoreEncoder_DeblockSlice)(
                state,
                core_enc->m_Slices + slice,
                core_enc->m_Slices[slice].m_first_mb_in_slice + core_enc->m_WidthInMBs * core_enc->m_HeightInMBs * field_index,
                core_enc->m_Slices[slice].m_MB_Counter );
    }
    if( core_enc->m_Analyse & ANALYSE_RECODE_FRAME && makeNULL ){
        core_enc->m_pReconstructFrame = NULL;
    }

    return &core_enc->m_pCurrentFrame->m_data;
}

// Get codec working (initialization) parameter(s)
Status H264ENC_MAKE_NAME(H264CoreEncoder_GetInfo)(
    void* state,
    BaseCodecParams *info)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    H264EncoderParams* pInfo = DynamicCast<H264EncoderParams, BaseCodecParams>(info);

    core_enc->m_info.qualityMeasure = 100 - (core_enc->qscale[0]+core_enc->qscale[1]+core_enc->qscale[2])*100/(3*112);
    core_enc->m_info.info.stream_type = H264_VIDEO;

    if(pInfo)
    {
        *pInfo = core_enc->m_info;
    } else if(info)
    {
        VideoEncoderParams* pInfo = DynamicCast<VideoEncoderParams, BaseCodecParams>(info);
        if(pInfo) {
            *pInfo = core_enc->m_info;
        } else {
            return UMC_ERR_INVALID_STREAM;
        }
    } else {
        return UMC_ERR_NULL_PTR;
    }


    return UMC_OK;
}

const H264SeqParamSet* H264ENC_MAKE_NAME(H264CoreEncoder_GetSeqParamSet)(void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    return &core_enc->m_SeqParamSet;
}

const H264PicParamSet* H264ENC_MAKE_NAME(H264CoreEncoder_GetPicParamSet)(void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    return &core_enc->m_PicParamSet;
}

Status H264ENC_MAKE_NAME(H264CoreEncoder_Close)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;

    if (core_enc->m_EmptyThreshold) {
        H264_Free(core_enc->m_EmptyThreshold);
        core_enc->m_EmptyThreshold = NULL;
    }
    if(core_enc->m_DirectBSkipMEThres) {
        H264_Free(core_enc->m_DirectBSkipMEThres);
        core_enc->m_DirectBSkipMEThres = NULL;
    }
    if(core_enc->m_PSkipMEThres) {
        H264_Free(core_enc->m_PSkipMEThres);
        core_enc->m_PSkipMEThres = NULL;
    }
    if(core_enc->m_BestOf5EarlyExitThres) {
        H264_Free(core_enc->m_BestOf5EarlyExitThres);
        core_enc->m_BestOf5EarlyExitThres = NULL;
    }
    if (core_enc->m_pbitstreams) {
        Ipp32s i;
        for (i = 0; i < core_enc->m_info.num_slices*((core_enc->m_info.coding_type == 1) + 1); i++) {  //TODO fix for PicAFF/AFRM
            if (core_enc->m_pbitstreams[i]) {
                H264_Free(core_enc->m_pbitstreams[i]);
                core_enc->m_pbitstreams[i] = NULL;
            }
        }
        H264_Free(core_enc->m_pbitstreams);
        core_enc->m_pbitstreams = NULL;
    }
    if (core_enc->m_pAllocEncoderInst) {
        H264_Free(core_enc->m_pAllocEncoderInst);
        core_enc->m_pAllocEncoderInst = NULL;
    }
    if( core_enc->eFrameType != NULL ){
        H264_Free(core_enc->eFrameType);
        core_enc->eFrameType = NULL;
    }
    if( core_enc->eFrameSeq != NULL ){
        H264_Free(core_enc->eFrameSeq);
        core_enc->eFrameSeq = NULL;
    }
    core_enc->m_pReconstructFrame = NULL;
    if (core_enc->m_pParsedDataNew) {
        H264_Free(core_enc->m_pParsedDataNew);
        core_enc->m_pParsedDataNew = NULL;
    }
    if (core_enc->m_Slices != NULL) {
        H264ENC_CALL_DELETE_ARR(H264Slice, core_enc->m_Slices);
        core_enc->m_Slices = NULL;
    }
#ifdef DEBLOCK_THREADING
    if(core_enc->mbs_deblock_ready != NULL)
    {
        H264_Free((void*)core_enc->mbs_deblock_ready);
        core_enc->mbs_deblock_ready = NULL;
    }
#endif

#ifdef SLICE_THREADING_LOAD_BALANCING
    // Load balancing for slice level multithreading
    if(core_enc->m_B_ticks_per_macroblock != NULL)
    {
        H264_Free(core_enc->m_B_ticks_per_macroblock);
        core_enc->m_B_ticks_per_macroblock = NULL;
    }
    if(core_enc->m_P_ticks_per_macroblock != NULL)
    {
        H264_Free(core_enc->m_P_ticks_per_macroblock);
        core_enc->m_P_ticks_per_macroblock = NULL;
    }
#endif // SLICE_THREADING_LOAD_BALANCING
    return UMC_OK;
}

Status H264ENC_MAKE_NAME(H264CoreEncoder_Reset)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32s i;

    H264ENC_MAKE_NAME(H264CoreEncoder_Close)(state);

    core_enc->profile_frequency = 1;
    core_enc->m_iProfileIndex = 0;
    core_enc->m_is_mb_data_initialized = false;
    if( core_enc->eFrameType == NULL ){
        core_enc->eFrameType = (EnumPicCodType *)H264_Malloc(sizeof(EnumPicCodType));
        core_enc->eFrameSeq = (H264EncoderFrameType **)H264_Malloc(sizeof(H264EncoderFrameType*));
        core_enc->eFrameType[0] = PREDPIC;
    }
    core_enc->m_uIntraFrameInterval = 0;
    core_enc->m_uIDRFrameInterval = 0;
    core_enc->m_PicOrderCnt = 0;
    core_enc->m_PicOrderCnt_Accu = 0;
    core_enc->m_l1_cnt_to_start_B = 0;
    core_enc->m_total_bits_encoded = 0;
    core_enc->use_implicit_weighted_bipred = false;
    core_enc->m_is_cur_pic_afrm = false;
    core_enc->m_PaddedSize.width = core_enc->m_PaddedSize.height = 0;
    core_enc->m_DirectTypeStat[0]=core_enc->m_DirectTypeStat[1]=0;
#if 0
    core_enc->m_pBitStream = core_enc->memAlloc =
    core_enc->m_pAllocEncoderInst = core_enc->m_pbitstreams = core_enc->m_bs1 = core_enc->m_dpb = core_enc->m_cpb =
    core_enc->m_pParsedDataNew = core_enc->m_pReconstructFrame = core_enc->m_pMBOffsets =
    core_enc->m_EmptyThreshold = core_enc->m_DirectBSkipMEThres = core_enc->m_PSkipMEThres =
    core_enc->m_BestOf5EarlyExitThres = core_enc->m_Slices = NULL;
#endif
    // Initialize the BRCState local variables based on the default
    // settings in core_enc->m_info.

    // If these assertions fail, then uTargetFrmSize needs to be set to
    // something other than 0.
    // Initialize the sequence parameter set structure.

    core_enc->m_SeqParamSet.profile_idc = H264_PROFILE_MAIN;
    core_enc->m_SeqParamSet.level_idc = 0;
    core_enc->m_SeqParamSet.constraint_set0_flag = 0;
    core_enc->m_SeqParamSet.constraint_set1_flag = 0;
    core_enc->m_SeqParamSet.constraint_set2_flag = 0;
    core_enc->m_SeqParamSet.chroma_format_idc = 1;
    core_enc->m_SeqParamSet.seq_parameter_set_id = 0;
    core_enc->m_SeqParamSet.log2_max_frame_num = 0;
    core_enc->m_SeqParamSet.pic_order_cnt_type = 0;
    core_enc->m_SeqParamSet.delta_pic_order_always_zero_flag = 0;
    core_enc->m_SeqParamSet.frame_mbs_only_flag = 0;
    core_enc->m_SeqParamSet.gaps_in_frame_num_value_allowed_flag = 0;
    core_enc->m_SeqParamSet.mb_adaptive_frame_field_flag = 0;
    core_enc->m_SeqParamSet.direct_8x8_inference_flag = 0;
    core_enc->m_SeqParamSet.vui_parameters_present_flag = 0;
    core_enc->m_SeqParamSet.log2_max_pic_order_cnt_lsb = 0;
    core_enc->m_SeqParamSet.offset_for_non_ref_pic = 0;
    core_enc->m_SeqParamSet.offset_for_top_to_bottom_field = 0;
    core_enc->m_SeqParamSet.num_ref_frames_in_pic_order_cnt_cycle = 0;
    core_enc->m_SeqParamSet.poffset_for_ref_frame = NULL;
    core_enc->m_SeqParamSet.num_ref_frames = 0;
    core_enc->m_SeqParamSet.frame_width_in_mbs = 0;
    core_enc->m_SeqParamSet.frame_height_in_mbs = 0;
    core_enc->m_SeqParamSet.frame_cropping_flag = 0;
    core_enc->m_SeqParamSet.frame_crop_left_offset = 0;
    core_enc->m_SeqParamSet.frame_crop_right_offset = 0;
    core_enc->m_SeqParamSet.frame_crop_top_offset = 0;
    core_enc->m_SeqParamSet.frame_crop_bottom_offset = 0;
    core_enc->m_SeqParamSet.bit_depth_luma = 8;
    core_enc->m_SeqParamSet.bit_depth_chroma = 8;
    core_enc->m_SeqParamSet.bit_depth_aux = 8;
    core_enc->m_SeqParamSet.alpha_incr_flag = 0;
    core_enc->m_SeqParamSet.alpha_opaque_value = 8;
    core_enc->m_SeqParamSet.alpha_transparent_value = 0;
    core_enc->m_SeqParamSet.aux_format_idc = 0;
    core_enc->m_SeqParamSet.seq_scaling_matrix_present_flag = false;
    for( i=0; i<8; i++) core_enc->m_SeqParamSet.seq_scaling_list_present_flag[i]=false;
    core_enc->m_SeqParamSet.pack_sequence_extension = false;
    core_enc->m_SeqParamSet.qpprime_y_zero_transform_bypass_flag = 0;
    core_enc->m_SeqParamSet.residual_colour_transform_flag = false;
    core_enc->m_SeqParamSet.additional_extension_flag = 0;

    // Initialize the picture parameter set structure.

    core_enc->m_PicParamSet.pic_parameter_set_id = 0;
    core_enc->m_PicParamSet.seq_parameter_set_id = 0;
    core_enc->m_PicParamSet.entropy_coding_mode = 0;
    core_enc->m_PicParamSet.pic_order_present_flag = 0;
    core_enc->m_PicParamSet.weighted_pred_flag = 0;
    core_enc->m_PicParamSet.weighted_bipred_idc = 0;
    core_enc->m_PicParamSet.pic_init_qp = 0;
    core_enc->m_PicParamSet.pic_init_qs = 0;
    core_enc->m_PicParamSet.chroma_qp_index_offset = 0;
    core_enc->m_PicParamSet.deblocking_filter_variables_present_flag = 0;
    core_enc->m_PicParamSet.constrained_intra_pred_flag = 0;
    core_enc->m_PicParamSet.redundant_pic_cnt_present_flag = 0;
    core_enc->m_PicParamSet.num_slice_groups = 1;
    core_enc->m_PicParamSet.SliceGroupInfo.slice_group_map_type = 0;
    core_enc->m_PicParamSet.SliceGroupInfo.t3.pic_size_in_map_units = 0;
    core_enc->m_PicParamSet.SliceGroupInfo.t3.pSliceGroupIDMap = NULL;
    core_enc->m_PicParamSet.num_ref_idx_l0_active = 0;
    core_enc->m_PicParamSet.num_ref_idx_l1_active = 0;

    core_enc->m_PicParamSet.second_chroma_qp_index_offset = core_enc->m_PicParamSet.chroma_qp_index_offset;
    core_enc->m_PicParamSet.pic_scaling_matrix_present_flag = 0;
    core_enc->m_PicParamSet.transform_8x8_mode_flag = false;

    // Initialize the slice header structure.
    core_enc->m_SliceHeader.pic_parameter_set_id = 0;
    core_enc->m_SliceHeader.field_pic_flag = 0;
    core_enc->m_SliceHeader.MbaffFrameFlag = 0;
    core_enc->m_SliceHeader.bottom_field_flag = 0;
    core_enc->m_SliceHeader.direct_spatial_mv_pred_flag = 1;
    core_enc->m_SliceHeader.long_term_reference_flag = 0;
    core_enc->m_SliceHeader.sp_for_switch_flag = 0;
    core_enc->m_SliceHeader.slice_qs_delta = 0;
    core_enc->m_SliceHeader.frame_num = 0;
    core_enc->m_SliceHeader.idr_pic_id = 0;
    core_enc->m_SliceHeader.pic_order_cnt_lsb = 0;
    core_enc->m_SliceHeader.delta_pic_order_cnt[0] = 0;
    core_enc->m_SliceHeader.delta_pic_order_cnt[1] = 0;
    core_enc->m_SliceHeader.redundant_pic_cnt = 0;
    core_enc->m_SliceHeader.slice_group_change_cycle = 0;
    core_enc->m_SliceHeader.delta_pic_order_cnt_bottom = 0;

    //Clear dpb and cpb
    H264ENC_MAKE_NAME(H264EncoderFrameList_clearFrameList)(&core_enc->m_cpb);
    H264ENC_MAKE_NAME(H264EncoderFrameList_clearFrameList)(&core_enc->m_dpb);

    return H264ENC_MAKE_NAME(H264CoreEncoder_Init)(state, &core_enc->m_info, core_enc->memAlloc);
}

void H264ENC_MAKE_NAME(H264CoreEncoder_SetDPBSize)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32u MaxDPBx2;
    Ipp32u dpbLevel;

    // MaxDPB, per Table A-1, Level Limits
    switch (core_enc->m_info.level_idc)
    {
    case 9:
    case 10:
        MaxDPBx2 = 297;
        break;
    case 11:
        MaxDPBx2 = 675;
        break;
    case 12:
    case 13:
    case 20:
        MaxDPBx2 = 891*2;
        break;
    case 21:
        MaxDPBx2 = 1782*2;
        break;
    case 22:
    case 30:
        MaxDPBx2 = 6075;
        break;
    case 31:
        MaxDPBx2 = 6750*2;
        break;
    case 32:
        MaxDPBx2 = 7680*2;
        break;
    case 40:
    case 41:
    case 42:
        MaxDPBx2 = 12288*2;
        break;
    case 50:
        MaxDPBx2 = 41400*2;
        break;
    default:
    case 51:
        MaxDPBx2 = 69120*2;
        break;
    }

    Ipp32u width, height;

    width = ((core_enc->m_info.info.clip_info.width+15)>>4)<<4;
    height = (((core_enc->m_info.info.clip_info.height+15)>>4) << (1 - (core_enc->m_info.coding_type ? 0:1)))*16;

    //Restrict maximum DPB size to 3xresolution according to IVT requirements
    //MaxDPBx2 = MIN( MaxDPBx2, (3*((width * height) + ((width * height)>>1))>>9)); // /512

    dpbLevel = (MaxDPBx2 * 512) / ((width * height) + ((width * height)>>1));
    core_enc->m_dpbSize = MIN(16, dpbLevel);
}

#undef H264BsRealType
#undef H264BsFakeType
#undef H264SliceType
#undef EncoderRefPicListType
#undef H264CoreEncoderType
#undef H264EncoderFrameType
#undef H264ENC_MAKE_NAME
#undef COEFFSTYPE
#undef PIXTYPE
