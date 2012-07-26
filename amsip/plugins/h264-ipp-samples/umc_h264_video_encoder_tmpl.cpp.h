//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 - 2011 Intel Corporation. All Rights Reserved.
//

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
#define H264CurrentMacroblockDescriptorType H264ENC_MAKE_NAME(H264CurrentMacroblockDescriptor)
#define H264EncoderFrameType H264ENC_MAKE_NAME(H264EncoderFrame)
#define EncoderRefPicListType H264ENC_MAKE_NAME(EncoderRefPicList)
#define EncoderRefPicListStructType H264ENC_MAKE_NAME(EncoderRefPicListStruct)

Status H264ENC_MAKE_NAME(H264CoreEncoder_CheckEncoderParameters)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Status status = UMC_OK;

    if (core_enc->m_info.coding_type > 1)
        core_enc->m_info.coding_type = 0;
    if (core_enc->m_info.rate_controls.method == H264_RCM_DEBUG)
        core_enc->m_info.rate_controls.method = H264_RCM_QUANT;
    if (core_enc->m_info.info.bitrate <= 0)
        core_enc->m_info.rate_controls.method = H264_RCM_QUANT;
    if (core_enc->m_info.info.clip_info.width < 1 || core_enc->m_info.info.clip_info.height < 1)
        return UMC::UMC_ERR_INIT;
    if (core_enc->m_info.key_frame_controls.interval == 1) { // all frame as I
        core_enc->m_info.B_frame_rate = 0;
        core_enc->m_info.num_ref_frames = 1;
    }
    if (core_enc->m_info.key_frame_controls.method != 1)
        core_enc->m_info.key_frame_controls.method = 1;
    if (core_enc->m_info.key_frame_controls.interval < 1)
        core_enc->m_info.key_frame_controls.interval = 250;
    if (core_enc->m_info.key_frame_controls.idr_interval < 0)
        core_enc->m_info.key_frame_controls.idr_interval = 4;
    if (core_enc->m_info.B_frame_rate < 0)
        core_enc->m_info.B_frame_rate = 0;
    if (core_enc->m_info.treat_B_as_reference < 0)
        core_enc->m_info.treat_B_as_reference = 0;
    if (core_enc->m_info.treat_B_as_reference > 2)
        core_enc->m_info.treat_B_as_reference = 2;
    if (core_enc->m_info.num_ref_frames < 1)
        core_enc->m_info.num_ref_frames = 1;
    if (core_enc->m_info.num_ref_frames > 16)
        core_enc->m_info.num_ref_frames = 16;
    if (core_enc->m_info.B_frame_rate > 0 && (core_enc->m_info.num_ref_to_start_code_B_slice <= 0 || core_enc->m_info.num_ref_to_start_code_B_slice > core_enc->m_info.num_ref_frames - 1)){
        //Correct number of reference frames
        if( core_enc->m_info.num_ref_to_start_code_B_slice <= 0 ) core_enc->m_info.num_ref_to_start_code_B_slice = 1;
        core_enc->m_info.num_ref_frames = core_enc->m_info.num_ref_to_start_code_B_slice + 1;
    }
#if defined (_OPENMP)
    if (core_enc->m_info.numThreads < 1)
        core_enc->m_info.numThreads = omp_get_max_threads();
#else
    core_enc->m_info.numThreads = 1;
#endif // _OPENMP

#ifdef SLICE_CHECK_LIMIT
    if (core_enc->m_info.num_slices < 0){
        core_enc->m_MaxSliceSize = ABS(core_enc->m_info.num_slices);
        core_enc->m_info.num_slices = 1;
    }
#ifdef AMD_FIX_SLICE_CHECK_LIMIT
    if (core_enc->m_info.num_slices < 1)
        core_enc->m_info.num_slices = (Ipp16s)IPP_MIN(core_enc->m_info.numThreads, 0x7FFF);
#else
	if (core_enc->m_info.num_slices == 0)
        core_enc->m_info.num_slices = (Ipp16s)core_enc->m_info.numThreads;
#endif
#else
    if (core_enc->m_info.num_slices < 1)
        core_enc->m_info.num_slices = (Ipp16s)IPP_MIN(core_enc->m_info.numThreads, 0x7FFF);
#endif
    Ipp32s nMB = ((core_enc->m_info.info.clip_info.height + 15) >> 4) * ((core_enc->m_info.info.clip_info.width + 15) >> 4);
    if (core_enc->m_info.num_slices > nMB)
        core_enc->m_info.num_slices = (Ipp16s)IPP_MIN(nMB, 0x7FFF);
    if (core_enc->m_info.num_slices < core_enc->m_info.numThreads)
        core_enc->m_info.numThreads = core_enc->m_info.num_slices;
#if defined (_OPENMP)
    omp_set_num_threads(core_enc->m_info.numThreads);
#endif // _OPENMP

    switch (core_enc->m_info.level_idc)
    {
        case 0:
        case 10: case 11: case 12: case 13:
        case 20: case 21: case 22:
        case 30: case 31: case 32:
        case 40: case 41: case 42:
        case 50: case 51:
        break;
    default:
        //Set default level to autoselect
        core_enc->m_info.level_idc = 0;
    }

    // Calculate the Level Based on encoding parameters .
    Ipp32u MB_per_frame = ((core_enc->m_info.info.clip_info.width+15)>>4) * ((core_enc->m_info.info.clip_info.height+15)>>4);
    Ipp32u MB_per_sec = (Ipp32u)(MB_per_frame * core_enc->m_info.info.framerate);
    if (core_enc->m_info.level_idc==0)
    {
        if ((MB_per_frame <= 99) && (MB_per_sec <= 1485)) {
            core_enc->m_info.level_idc = 10;   // Level 1
        } else if ((MB_per_frame <= 396) && (MB_per_sec <= 3000)) {
            core_enc->m_info.level_idc = 11;   // Level 1.1
        } else if ((MB_per_frame <= 396) && (MB_per_sec <= 6000)) {
            core_enc->m_info.level_idc = 12;   // Level 1.2
        } else if ((MB_per_frame <= 396) && (MB_per_sec <= 11880)) {
            core_enc->m_info.level_idc = 20;   // Level 2
        } else if ((MB_per_frame <= 792) && (MB_per_sec <= 19800)) {
            core_enc->m_info.level_idc = 21;   // Level 2.1
        } else if ((MB_per_frame <= 1620) && (MB_per_sec <= 20250)) {
            core_enc->m_info.level_idc = 22;   // Level 2.2
        } else if ((MB_per_frame <= 1620) && (MB_per_sec <= 40500)) {
            core_enc->m_info.level_idc = 30;   // Level 3
        } else if ((MB_per_frame <= 3600) && (MB_per_sec <= 108000)) {
            core_enc->m_info.level_idc = 31;   // Level 3.1
        } else if ((MB_per_frame <= 5120) && (MB_per_sec <= 216000)) {
            core_enc->m_info.level_idc = 32;   // Level 3.2
        } else if ((MB_per_frame <= 8192) && (MB_per_sec <= 245760)) {
            core_enc->m_info.level_idc = 40;   // Level 4
//        } else if ((MB_per_frame <= 8192) && (MB_per_sec <= 245760)) {
//            core_enc->m_SeqParamSet.level_idc = 41;   // Level 4.1
        } else if ((MB_per_frame <= 8704) && (MB_per_sec <= 522240)) {
            core_enc->m_info.level_idc = 42;   // Level 4.2
        } else if ((MB_per_frame <= 22080) && (MB_per_sec <= 589824)) {
            core_enc->m_info.level_idc= 50;    // Level 5
        } else {
            core_enc->m_info.level_idc= 51;    // Level 5.1
        }
    }

    //Switch appropriate profile
    bool profile_check = true;
    while( profile_check ){
        switch (core_enc->m_info.profile_idc)
        {
            case H264_PROFILE_BASELINE:
                if( core_enc->m_info.B_frame_rate != 0 ){ //B frames are not allowed for Base Profile
                    core_enc->m_info.profile_idc = H264_PROFILE_MAIN;
                    //return UMC_ERR_INVALID_STREAM;
                    break;
                }
            case H264_PROFILE_MAIN:
            case H264_PROFILE_EXTENDED:

                if(    core_enc->m_info.bit_depth_aux != 8
                    || core_enc->m_info.alpha_incr_flag != 0
                    || core_enc->m_info.alpha_opaque_value != 0
                    || core_enc->m_info.alpha_transparent_value != 0
                    || core_enc->m_SeqParamSet.aux_format_idc != 0
                    || core_enc->m_info.bit_depth_chroma != 8
                    || core_enc->m_info.bit_depth_luma != 8
                    || core_enc->m_info.chroma_format_idc != 1
                    || core_enc->m_info.transform_8x8_mode_flag
                    || core_enc->m_info.qpprime_y_zero_transform_bypass_flag) {
                    //return(UMC_ERR_INVALID_STREAM);
                    core_enc->m_info.profile_idc = H264_PROFILE_HIGH;
                    break;
                }
                profile_check = false;
                break;
            case H264_PROFILE_HIGH:
                if(    core_enc->m_info.bit_depth_aux != 8
                    || core_enc->m_info.bit_depth_chroma != 8
                    || core_enc->m_info.bit_depth_luma != 8
                    || core_enc->m_info.chroma_format_idc > 1) {
                    core_enc->m_info.profile_idc = H264_PROFILE_HIGH10;
                    //return(UMC_ERR_INVALID_STREAM);
                    break;
                }
            case H264_PROFILE_HIGH10:
            case H264_PROFILE_HIGH422:
               if(core_enc->m_info.qpprime_y_zero_transform_bypass_flag) {
                    core_enc->m_info.profile_idc = H264_PROFILE_HIGH444;
                    //return(UMC_ERR_INVALID_STREAM);
                    break;
               }
                   /*
                if(core_enc->m_info.chroma_format_idc == 2) {
                    return(UMC_ERR_NOT_IMPLEMENTED);
                }*/
            case H264_PROFILE_HIGH444:
                if(    core_enc->m_info.bit_depth_aux < 8
                    || core_enc->m_info.bit_depth_chroma < 8
                    || core_enc->m_info.bit_depth_luma < 8
                    || core_enc->m_info.bit_depth_aux > 12
                    || core_enc->m_info.bit_depth_chroma > 12
                    || core_enc->m_info.bit_depth_luma > 12) {
                    return(UMC_ERR_INVALID_STREAM);
                }
#if !defined BITDEPTH_9_12
                if(    core_enc->m_info.bit_depth_aux != 8
                    || core_enc->m_info.bit_depth_chroma != 8
                    || core_enc->m_info.bit_depth_luma != 8) {
                        return(UMC_ERR_NOT_IMPLEMENTED);
                }
#endif // BITDEPTH_9_12
                if(core_enc->m_info.chroma_format_idc > 2) {
                    return(UMC_ERR_NOT_IMPLEMENTED);
                }
                profile_check = false;
                break;
            default:
                //Reset paremeters to use appropriate encoding
                return(UMC_ERR_INVALID_STREAM);
        }
    }

    switch(core_enc->m_info.rate_controls.method) {
        case H264_RCM_QUANT:
        case H264_RCM_CBR:
        case H264_RCM_VBR:
        case H264_RCM_CBR_SLICE:
        case H264_RCM_VBR_SLICE:
        case H264_RCM_DEBUG:
            break;
        default:
            core_enc->m_info.rate_controls.method = H264_RCM_VBR;
            //return(UMC_ERR_INVALID_STREAM);
            break;
    }

    H264ENC_MAKE_NAME(H264CoreEncoder_SetDPBSize)(state);

    if (core_enc->m_info.num_ref_frames > core_enc->m_dpbSize)
        core_enc->m_info.num_ref_frames = core_enc->m_dpbSize;
    if (core_enc->m_info.rate_controls.quantI < -6*(core_enc->m_info.bit_depth_luma - 8) || core_enc->m_info.rate_controls.quantI > 51)
        core_enc->m_info.rate_controls.quantI = 0;
    if (core_enc->m_info.rate_controls.quantP < -6*(core_enc->m_info.bit_depth_luma - 8) || core_enc->m_info.rate_controls.quantP > 51)
        core_enc->m_info.rate_controls.quantP = 0;
    if (core_enc->m_info.rate_controls.quantB < -6*(core_enc->m_info.bit_depth_luma - 8) || core_enc->m_info.rate_controls.quantB > 51)
        core_enc->m_info.rate_controls.quantB = 0;
    if (core_enc->m_info.mv_search_method < 0 || core_enc->m_info.mv_search_method > 31)
        core_enc->m_info.mv_search_method = 2;
    if (core_enc->m_info.me_split_mode < 0 || core_enc->m_info.me_split_mode > 3)
        core_enc->m_info.me_split_mode = 1;
    if (core_enc->m_info.use_weighted_pred != 0 && core_enc->m_info.use_weighted_pred != 1)
        core_enc->m_info.use_weighted_pred = 0;
    if (core_enc->m_info.use_weighted_bipred != 0 && core_enc->m_info.use_weighted_bipred != 1)
        core_enc->m_info.use_weighted_bipred = 0;
    if (core_enc->m_info.use_implicit_weighted_bipred != 0 && core_enc->m_info.use_implicit_weighted_bipred != 1)
        core_enc->m_info.use_implicit_weighted_bipred = 0;
    if (core_enc->m_info.direct_pred_mode < 0 || core_enc->m_info.direct_pred_mode > 2)
        core_enc->m_info.direct_pred_mode = 1;
    if (core_enc->m_info.use_direct_inference != 0 && core_enc->m_info.use_direct_inference != 1)
        core_enc->m_info.use_direct_inference = 1;
    if (core_enc->m_info.profile_idc == H264_PROFILE_EXTENDED || core_enc->m_info.level_idc >= 30 || core_enc->m_info.coding_type )
        core_enc->m_info.use_direct_inference = 1;
    if (core_enc->m_info.deblocking_filter_idc != 0 && core_enc->m_info.deblocking_filter_idc != 1 && core_enc->m_info.deblocking_filter_idc != 2)
        core_enc->m_info.deblocking_filter_idc = 0;
    if (core_enc->m_info.deblocking_filter_alpha < -12 || core_enc->m_info.deblocking_filter_alpha > 12)
        core_enc->m_info.deblocking_filter_alpha = 6;
    if (core_enc->m_info.deblocking_filter_beta < -12 || core_enc->m_info.deblocking_filter_beta > 12)
        core_enc->m_info.deblocking_filter_beta = 6;
    if (core_enc->m_info.entropy_coding_mode != 0 && core_enc->m_info.entropy_coding_mode != 1)
        core_enc->m_info.entropy_coding_mode = 0;
    if (core_enc->m_info.cabac_init_idc < 0 || core_enc->m_info.cabac_init_idc > 2)
        core_enc->m_info.cabac_init_idc = 0;
    return status;
}

Status H264ENC_MAKE_NAME(H264CoreEncoder_SetParams)(
    void* state,
    BaseCodecParams* params)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    VideoEncoderParams *gen_info = DynamicCast<VideoEncoderParams, BaseCodecParams> (params);
    //H264EncoderParams *h264_info = DynamicCast<H264EncoderParams, BaseCodecParams> (params);

    if (gen_info == NULL)
        return UMC_ERR_NULL_PTR;
    if (gen_info->info.bitrate == core_enc->m_info.info.bitrate && gen_info->info.framerate == core_enc->m_info.info.framerate)
        return UMC_ERR_UNSUPPORTED;
    if (gen_info->info.bitrate != core_enc->m_info.info.bitrate || gen_info->info.framerate != core_enc->m_info.info.framerate) {
        core_enc->m_info.info.bitrate = gen_info->info.bitrate;
        core_enc->m_info.info.framerate = gen_info->info.framerate;
        Ipp32s bitDepth = IPP_MAX(core_enc->m_info.bit_depth_aux, IPP_MAX(core_enc->m_info.bit_depth_chroma, core_enc->m_info.bit_depth_luma));
        Ipp32s chromaSampling = core_enc->m_info.chroma_format_idc;
        Ipp32s alpha = core_enc->m_info.aux_format_idc ? 1 : 0;
        if (core_enc->m_info.rate_controls.method == H264_RCM_CBR) {
            H264_AVBR_Init(&core_enc->avbr, 8, 4, 8, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate, core_enc->m_info.info.clip_info.width*core_enc->m_info.info.clip_info.height, bitDepth, chromaSampling, alpha);
        } else if (core_enc->m_info.rate_controls.method == H264_RCM_VBR) {
            H264_AVBR_Init(&core_enc->avbr, 0, 0, 0, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate, core_enc->m_info.info.clip_info.width*core_enc->m_info.info.clip_info.height, bitDepth, chromaSampling, alpha);
        } else if (core_enc->m_info.rate_controls.method == H264_RCM_CBR_SLICE) {
            H264_AVBR_Init(&core_enc->avbr, 8, 4, 8, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate*core_enc->m_info.num_slices, core_enc->m_info.info.clip_info.width*core_enc->m_info.info.clip_info.height/core_enc->m_info.num_slices, bitDepth, chromaSampling, alpha);
        } else if (core_enc->m_info.rate_controls.method == H264_RCM_VBR_SLICE) {
            H264_AVBR_Init(&core_enc->avbr, 0, 0, 0, core_enc->m_info.info.bitrate, core_enc->m_info.info.framerate*core_enc->m_info.num_slices, core_enc->m_info.info.clip_info.width*core_enc->m_info.info.clip_info.height/core_enc->m_info.num_slices, bitDepth, chromaSampling, alpha);
        }else
            return UMC_ERR_UNSUPPORTED;
    }
    return UMC_OK;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_UpdateRefPicListCommon)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    H264EncoderFrameType *pFrm;
    H264EncoderFrameType *pHead = core_enc->m_dpb.m_pHead;

    Ipp32u uMaxFrameNum = (1<<core_enc->m_SeqParamSet.log2_max_frame_num);
    //Ipp32u uMaxPicNum = (core_enc->m_SliceHeader.field_pic_flag == 0) ? uMaxFrameNum : uMaxFrameNum<<1;

    for (pFrm = pHead; pFrm; pFrm = pFrm->m_pFutureFrame)
    {
        // update FrameNumWrap and PicNum if frame number wrap occurred,
        // for short-term frames
        // TBD: modify for fields
        H264ENC_MAKE_NAME(H264EncoderFrame_UpdateFrameNumWrap)(
            pFrm,
            (Ipp32s)core_enc->m_SliceHeader.frame_num,
            uMaxFrameNum,
            core_enc->m_pCurrentFrame->m_PictureStructureForRef+
            core_enc->m_pCurrentFrame->m_bottom_field_flag[core_enc->m_field_index]);

        // For long-term references, update LongTermPicNum. Note this
        // could be done when LongTermFrameIdx is set, but this would
        // only work for frames, not fields.
        // TBD: modify for fields
        H264ENC_MAKE_NAME(H264EncoderFrame_UpdateLongTermPicNum)(
            pFrm,
            core_enc->m_pCurrentFrame->m_PictureStructureForRef +
                core_enc->m_pCurrentFrame->m_bottom_field_flag[core_enc->m_field_index]);
    }
}

Status H264ENC_MAKE_NAME(H264CoreEncoder_UpdateRefPicList)(
    void* state,
    H264SliceType *curr_slice,
    EncoderRefPicListType * ref_pic_list,
    H264SliceHeader &SHdr,
    RefPicListReorderInfo *pReorderInfo_L0,
    RefPicListReorderInfo *pReorderInfo_L1)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    EnumSliceType slice_type = curr_slice->m_slice_type;
    Status ps = UMC_OK;
    Ipp32u NumShortTermRefs, NumLongTermRefs;
    H264SeqParamSet *sps;
    Ipp32u uMaxFrameNum;
    Ipp32u uMaxPicNum;

    VM_ASSERT(core_enc->m_pCurrentFrame);

    H264EncoderFrameType **pRefPicList0 = ref_pic_list->m_RefPicListL0.m_RefPicList;
    H264EncoderFrameType **pRefPicList1 = ref_pic_list->m_RefPicListL1.m_RefPicList;
    Ipp8s *pFields0 = ref_pic_list->m_RefPicListL0.m_Prediction;
    Ipp8s *pFields1 = ref_pic_list->m_RefPicListL1.m_Prediction;
    Ipp32s * pPOC0 = ref_pic_list->m_RefPicListL0.m_POC;
    Ipp32s * pPOC1 = ref_pic_list->m_RefPicListL1.m_POC;


    curr_slice->m_NumRefsInL0List = 0;
    curr_slice->m_NumRefsInL1List = 0;

    // Spec reference: 8.2.4, "Decoding process for reference picture lists
    // construction"

    // get pointers to the picture and sequence parameter sets currently in use
    //pps = &core_enc->m_PicParamSet;
    sps = &core_enc->m_SeqParamSet;
    uMaxFrameNum = (1<<sps->log2_max_frame_num);
    uMaxPicNum = (SHdr.field_pic_flag == 0) ? uMaxFrameNum : uMaxFrameNum<<1;

    if ((slice_type != INTRASLICE) && (slice_type != S_INTRASLICE)){
        // Detect and report no available reference frames
        H264ENC_MAKE_NAME(H264EncoderFrameList_countActiveRefs)(
            &core_enc->m_dpb,
            NumShortTermRefs,
            NumLongTermRefs);
        if ((NumShortTermRefs + NumLongTermRefs) == 0){
            VM_ASSERT(0);
            ps = UMC_ERR_INVALID_STREAM;
        }

        if (ps == UMC_OK){
            // Initialize the reference picture lists
            if ((slice_type == PREDSLICE) || (slice_type == S_PREDSLICE))
                H264ENC_MAKE_NAME(H264CoreEncoder_InitPSliceRefPicList)(state, curr_slice, SHdr.field_pic_flag != 0, pRefPicList0);
            else
                H264ENC_MAKE_NAME(H264CoreEncoder_InitBSliceRefPicLists)(state, curr_slice, SHdr.field_pic_flag != 0, pRefPicList0, pRefPicList1);

            // Reorder the reference picture lists
            Ipp32s num_ST_Ref0 = 0, num_LT_Ref = 0;
            Ipp32s num_ST_Ref1;
            if (SHdr.field_pic_flag){
                H264ENC_MAKE_NAME(H264CoreEncoder_AdjustRefPicListForFields)(state, pRefPicList0, pFields0);
                num_ST_Ref0 = core_enc->m_NumShortEntriesInList;
                num_LT_Ref = core_enc->m_NumLongEntriesInList;
            } else
                for (Ipp32s i=0;i<MAX_NUM_REF_FRAMES;i++)  pFields0[i]=0;

            if (BPREDSLICE == slice_type) {
                if (SHdr.field_pic_flag) {
                    H264ENC_MAKE_NAME(H264CoreEncoder_AdjustRefPicListForFields)(state, pRefPicList1, pFields1);
                    num_ST_Ref1 = core_enc->m_NumShortEntriesInList;
                    if ((curr_slice->m_NumRefsInL0List == 0 || curr_slice->m_NumRefsInL1List == 0) &&
                        ((num_ST_Ref0+num_ST_Ref1+num_LT_Ref) > 1))//handling special case for fields
                    {
                        pRefPicList1[0] = pRefPicList0[1];
                        pRefPicList1[1] = pRefPicList0[0];
                        pFields1[0] = pFields0[1];
                        pFields1[1] = pFields0[0];

                    }

                    curr_slice->m_NumRefsInL0List = num_ST_Ref0 + num_LT_Ref;
                    curr_slice->m_NumRefsInL1List = num_ST_Ref1 + num_LT_Ref;
                }else{
                    for (Ipp32s i=0;i<MAX_NUM_REF_FRAMES;i++)  pFields1[i]=0;
                    curr_slice->m_NumRefsInL0List += curr_slice->m_NumRefsInLTList;
                    curr_slice->m_NumRefsInL1List += curr_slice->m_NumRefsInLTList;
                }
            }

            if (pReorderInfo_L0->num_entries > 0)
                H264ENC_MAKE_NAME(H264CoreEncoder_ReOrderRefPicList)(state, SHdr.field_pic_flag != 0,pRefPicList0, pFields0,pReorderInfo_L0, uMaxPicNum, curr_slice->num_ref_idx_l0_active);

            if (BPREDSLICE == slice_type && pReorderInfo_L1->num_entries > 0)
                H264ENC_MAKE_NAME(H264CoreEncoder_ReOrderRefPicList)(state, SHdr.field_pic_flag != 0,pRefPicList1, pFields1,pReorderInfo_L1, uMaxPicNum, curr_slice->num_ref_idx_l1_active);

            if (SHdr.field_pic_flag){
                Ipp32s i;
                for (i = 0; i < curr_slice->m_NumRefsInL0List; i++)
                    pPOC0[i] = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList0[i], pFields0[i], 1);
                for (i = 0; i < curr_slice->m_NumRefsInL1List; i++)
                    pPOC1[i] = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList1[i], pFields1[i], 1);
            } else {
                Ipp32s i;
                for (i = 0; i < curr_slice->m_NumRefsInL0List; i++)
                    pPOC0[i] = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList0[i], 0, 3);
                for (i = 0; i < curr_slice->m_NumRefsInL1List; i++)
                    pPOC1[i] = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList1[i], 0, 3);
            }

            curr_slice->num_ref_idx_l0_active = MAX(curr_slice->m_NumRefsInL0List, 1);
            curr_slice->num_ref_idx_l1_active = MAX(curr_slice->m_NumRefsInL1List, 1);

/*            if( BPREDSLICE == slice_type ){
                curr_slice->num_ref_idx_l0_active = curr_slice->num_ref_idx_l1_active = MAX(curr_slice->m_NumRefsInL0List+curr_slice->m_NumRefsInL1List, 1);
            }
*/
            curr_slice->num_ref_idx_active_override_flag = ((curr_slice->num_ref_idx_l0_active != core_enc->m_PicParamSet.num_ref_idx_l0_active)
                                                         || (curr_slice->num_ref_idx_l1_active != core_enc->m_PicParamSet.num_ref_idx_l1_active));

            // If B slice, init scaling factors array
            if ((BPREDSLICE == slice_type) && (pRefPicList1[0] != NULL)){
                H264ENC_MAKE_NAME(H264CoreEncoder_InitDistScaleFactor)(state, curr_slice, curr_slice->num_ref_idx_l0_active, curr_slice->num_ref_idx_l1_active, pRefPicList0, pRefPicList1, pFields0,pFields1);
                H264ENC_MAKE_NAME(H264CoreEncoder_InitMapColMBToList0)(curr_slice, curr_slice->num_ref_idx_l0_active, pRefPicList0, pRefPicList1 );
//                InitMVScale(curr_slice, curr_slice->num_ref_idx_l0_active, curr_slice->num_ref_idx_l1_active, pRefPicList0, pRefPicList1, pFields0,pFields1);
            }
        }
        //update temporal refpiclists
        Ipp32s i;
        switch(core_enc->m_pCurrentFrame->m_PictureStructureForDec){
            case FLD_STRUCTURE:
            case FRM_STRUCTURE:
                for (i=0;i<MAX_NUM_REF_FRAMES;i++){
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL0.m_RefPicList[i]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL0.m_RefPicList[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL0.m_RefPicList[i]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL0.m_RefPicList[i]= pRefPicList0[i];
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL0.m_Prediction[i]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL0.m_Prediction[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL0.m_Prediction[i]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL0.m_Prediction[i]= pFields0[i];
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL0.m_POC[i]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL0.m_POC[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL0.m_POC[i]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL0.m_POC[i]= pPOC0[i];
                }
                for (i=0;i<MAX_NUM_REF_FRAMES;i++){
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL1.m_RefPicList[i]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL1.m_RefPicList[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL1.m_RefPicList[i]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL1.m_RefPicList[i]= pRefPicList1[i];
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL1.m_Prediction[i]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL1.m_Prediction[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL1.m_Prediction[i]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL1.m_Prediction[i]= pFields1[i];
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL1.m_POC[i]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL1.m_POC[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL1.m_POC[i]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL1.m_POC[i]= pPOC1[i];
            }
            break;
            case AFRM_STRUCTURE:
                for (i=0;i<MAX_NUM_REF_FRAMES / 2;i++){
                    //frame part
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL0.m_RefPicList[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL0.m_RefPicList[i]= pRefPicList0[i];
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL0.m_Prediction[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL0.m_Prediction[i]= 0;
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL0.m_POC[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL0.m_POC[i]= pPOC0[i];
                    //field part
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL0.m_RefPicList[2*i+0]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL0.m_RefPicList[2*i+0]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL0.m_RefPicList[2*i+1]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL0.m_RefPicList[2*i+1]= pRefPicList0[i];
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL0.m_Prediction[2*i+0]=0;
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL0.m_Prediction[2*i+1]=1;
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL0.m_Prediction[2*i+0]=1;
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL0.m_Prediction[2*i+1]=0;

                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL0.m_POC[2*i+0]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL0.m_POC[2*i+0]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL0.m_POC[2*i+1]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL0.m_POC[2*i+1]= pPOC0[i];
                }
                for (i=0;i<MAX_NUM_REF_FRAMES / 2;i++) {
                //frame part
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL1.m_RefPicList[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL1.m_RefPicList[i]= pRefPicList1[i];
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL1.m_Prediction[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL1.m_Prediction[i]= 0;
                    curr_slice->m_TempRefPicList[0][0].m_RefPicListL1.m_POC[i]=
                    curr_slice->m_TempRefPicList[0][1].m_RefPicListL1.m_POC[i]= pPOC1[i];
                    //field part
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL1.m_RefPicList[2*i+0]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL1.m_RefPicList[2*i+0]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL1.m_RefPicList[2*i+1]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL1.m_RefPicList[2*i+1]= pRefPicList1[i];
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL1.m_Prediction[2*i+0]=0;
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL1.m_Prediction[2*i+1]=1;
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL1.m_Prediction[2*i+0]=1;
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL1.m_Prediction[2*i+1]=0;

                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL1.m_POC[2*i+0]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL1.m_POC[2*i+0]=
                    curr_slice->m_TempRefPicList[1][0].m_RefPicListL1.m_POC[2*i+1]=
                    curr_slice->m_TempRefPicList[1][1].m_RefPicListL1.m_POC[2*i+1]= pPOC1[i];
                }
                break;
            }
    }

    return ps;
}    // UpdateRefPicList

//////////////////////////////////////////////////////////////////////////////
// InitPSliceRefPicList
//////////////////////////////////////////////////////////////////////////////
void H264ENC_MAKE_NAME(H264CoreEncoder_InitPSliceRefPicList)(
    void* state,
    H264SliceType *curr_slice,
    bool bIsFieldSlice,
    H264EncoderFrameType **pRefPicList)   // pointer to start of list 0
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32s i, j, k;
    Ipp32s NumFramesInList;
    H264EncoderFrameType *pHead = core_enc->m_dpb.m_pHead;
    H264EncoderFrameType *pFrm;
    Ipp32s PicNum;

    VM_ASSERT(pRefPicList);

    for (i=0; i<MAX_NUM_REF_FRAMES; i++) pRefPicList[i] = NULL;
    NumFramesInList = 0;

    if (!bIsFieldSlice){
        // Frame. Ref pic list ordering: Short term largest pic num to
        // smallest, followed by long term, largest long term pic num to
        // smallest. Note that ref pic list has one extra slot to assist
        // with re-ordering.
        for (pFrm = pHead; pFrm; pFrm = pFrm->m_pFutureFrame){
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pFrm) == 3){
                PicNum = H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pFrm, 0, 0);
                j=0; // find insertion point
                while (j < NumFramesInList &&
                    H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList[j]) &&
                    H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pRefPicList[j], 0, 0) > PicNum)
                    j++;

                if (pRefPicList[j]){                 // make room if needed
                    if( NumFramesInList == MAX_NUM_REF_FRAMES ){  NumFramesInList--;  VM_ASSERT(0); } // Avoid writing beyond end of list, discard last element
                    for (k=NumFramesInList; k>j; k--) pRefPicList[k] = pRefPicList[k-1];
                }

                pRefPicList[j] = pFrm;  // add the short-term reference
                NumFramesInList++;
            }else if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pFrm) == 3){
                PicNum = H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pFrm, 0, 3);
                j=0; // find insertion point: Skip past short-term refs and long term refs with smaller long term pic num
                while (j < NumFramesInList &&
                    (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList[j]) ||
                    (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pRefPicList[j]) &&
                     H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pRefPicList[j], 0,2) < PicNum)))
                     j++;

                if (pRefPicList[j]){ // make room if needed
                    if( NumFramesInList == MAX_NUM_REF_FRAMES ){  NumFramesInList--;  VM_ASSERT(0); } // Avoid writing beyond end of list, discard last element
                    for (k=NumFramesInList; k>j; k--) pRefPicList[k] = pRefPicList[k-1];
                }

                pRefPicList[j] = pFrm; // add the short-term reference
                NumFramesInList++;
            }
        }
    }else{
        // TBD: field
        for (pFrm = pHead; pFrm; pFrm = pFrm->m_pFutureFrame){
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pFrm)){
                PicNum = pFrm->m_FrameNumWrap;
                j=0; // find insertion point
                while (j < NumFramesInList &&
                    H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList[j]) &&
                    pRefPicList[j]->m_FrameNumWrap > PicNum)
                    j++;

                if (pRefPicList[j]) { // make room if needed
                    if( NumFramesInList == MAX_NUM_REF_FRAMES ){  NumFramesInList--;  VM_ASSERT(0); }  // Avoid writing beyond end of list, discard last element
                    for (k=NumFramesInList; k>j; k--)  pRefPicList[k] = pRefPicList[k-1];
                }

                pRefPicList[j] = pFrm;  // add the long-term reference
                NumFramesInList++;
            } else if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pFrm)) {
                PicNum = H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pFrm, 0, 2);

                j=0; // find insertion point: Skip past short-term refs and long term refs with smaller long term pic num
                while (j < NumFramesInList &&
                    (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList[j]) ||
                    (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pRefPicList[j]) &&
                     H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pRefPicList[j], 0, 2) < PicNum)))
                     j++;

                // make room if needed
                if (pRefPicList[j]){
                    if( NumFramesInList == MAX_NUM_REF_FRAMES ){  NumFramesInList--;  VM_ASSERT(0); } // Avoid writing beyond end of list, discard last element
                    for (k=NumFramesInList; k>j; k--) pRefPicList[k] = pRefPicList[k-1];
                }

                pRefPicList[j] = pFrm; // add the long-term reference
                NumFramesInList++;
            }
        }
    }
#ifdef STORE_PICLIST
    if (refpic == NULL) refpic = fopen(__PICLIST_FILE__,VM_STRING("wt"));
    if (refpic){
        fprintf(refpic,"P Slice cur_poc %d %d field %d parity %d\n",
            H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(core_enc->m_pCurrentFrame, 0, 1),
            H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(core_enc->m_pCurrentFrame, 1, 1),
            core_enc->m_field_index,
            core_enc->m_pCurrentFrame->m_bottom_field_flag[core_enc->m_field_index]);
        fprintf(refpic,"RefPicList 0:\n");

        for (Ipp32s i=0;i<NumFramesInList;i++){
             fprintf(refpic,"Entry %d poc %d %d picnum %d %d FNW %d str %d\n",i,
                H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList[i], 0, 1),
                H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList[i], 1, 1),
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pRefPicList[i], 0, 0),
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pRefPicList[i], 1, 0),
                pRefPicList[i]->m_FrameNumWrap,
                H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList[i]));
        }
        fprintf(refpic,"\n");
        fflush(refpic);
    }
#endif

    curr_slice->m_NumRefsInL0List = NumFramesInList;
    curr_slice->m_NumRefsInL1List = 0;
}    // InitPSliceRefPicList

//////////////////////////////////////////////////////////////////////////////
// InitBSliceRefPicLists
//////////////////////////////////////////////////////////////////////////////
void H264ENC_MAKE_NAME(H264CoreEncoder_InitBSliceRefPicLists)(
    void* state,
    H264SliceType *curr_slice,
    bool bIsFieldSlice,
    H264EncoderFrameType **pRefPicList0,    // pointer to start of list 0
    H264EncoderFrameType **pRefPicList1)    // pointer to start of list 1
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32s i, j, k;
    Ipp32s NumFramesInL0List;
    Ipp32s NumFramesInL1List;
    Ipp32s NumFramesInLTList;
    H264EncoderFrameType *pHead = core_enc->m_dpb.m_pHead;
    H264EncoderFrameType *pFrm;
    Ipp32s FrmPicOrderCnt;
    H264EncoderFrameType *LTRefPicList[MAX_NUM_REF_FRAMES];    // temp storage for long-term ordered list
    Ipp32s LongTermPicNum;

    for (i=0; i<MAX_NUM_REF_FRAMES; i++){
        pRefPicList0[i] = NULL;
        pRefPicList1[i] = NULL;
        LTRefPicList[i] = NULL;
    }

    NumFramesInL0List = 0;
    NumFramesInL1List = 0;
    NumFramesInLTList = 0;

    if (!bIsFieldSlice){
        // Short term references:
        // Need L0 and L1 lists. Both contain 2 sets of reference frames ordered
        // by PicOrderCnt. The "previous" set contains the reference frames with
        // a PicOrderCnt < current frame. The "future" set contains the reference
        // frames with a PicOrderCnt > current frame. In both cases the ordering
        // is from closest to current frame to farthest. L0 has the previous set
        // followed by the future set; L1 has the future set followed by the previous set.
        // Accomplish this by one pass through the decoded frames list creating
        // the ordered previous list in the L0 array and the ordered future list
        // in the L1 array. Then copy from both to the other for the second set.

        // Long term references:
        // The ordered list is the same for L0 and L1, is ordered by ascending
        // LongTermPicNum. The ordered list is created using local temp then
        // appended to the L0 and L1 lists after the short term references.
        Ipp32s CurrPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(core_enc->m_pCurrentFrame, 0, 0);

        for (pFrm = pHead; pFrm; pFrm = pFrm->m_pFutureFrame){
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pFrm) == 3){
                FrmPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pFrm, 0, 3);

                if (FrmPicOrderCnt < CurrPicOrderCnt){
                    j=0; // Previous reference to L0, order large to small
                    while (j < NumFramesInL0List &&
                        (H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList0[j], 0, 3) > FrmPicOrderCnt))
                        j++;

                    // make room if needed
                    if (pRefPicList0[j]){
                        if( NumFramesInL0List == MAX_NUM_REF_FRAMES ){  NumFramesInL0List--;  VM_ASSERT(0); }  // Avoid writing beyond end of list, discard last element
                        for (k=NumFramesInL0List; k>j; k--) pRefPicList0[k] = pRefPicList0[k-1];
                    }
                    pRefPicList0[j] = pFrm; // add the short-term reference
                    NumFramesInL0List++;
                } else {
                    j=0; // Future reference to L1, order small to large
                    while (j < NumFramesInL1List &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList1[j], 0, 3) < FrmPicOrderCnt)
                        j++;

                    // make room if needed
                    if (pRefPicList1[j]) {
                        if( NumFramesInL1List == MAX_NUM_REF_FRAMES ){  NumFramesInL1List--;  VM_ASSERT(0); }  // Avoid writing beyond end of list, discard last element
                        for (k=NumFramesInL1List; k>j; k--) pRefPicList1[k] = pRefPicList1[k-1];
                    }

                    // add the short-term reference
                    pRefPicList1[j] = pFrm;
                    NumFramesInL1List++;
                }
            } else if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pFrm) == 3){
                LongTermPicNum = H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pFrm, 0, 3);
                j=0; // order smallest to largest
                while (j < NumFramesInLTList &&
                    H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(LTRefPicList[j], 0, 0) < LongTermPicNum)
                    j++;

                if (LTRefPicList[j]){ // make room if needed
                    if( NumFramesInLTList == MAX_NUM_REF_FRAMES ){  NumFramesInLTList--;  VM_ASSERT(0); }  // Avoid writing beyond end of list, discard last element
                    for (k=NumFramesInLTList; k>j; k--) LTRefPicList[k] = LTRefPicList[k-1];
                }
                LTRefPicList[j] = pFrm; // add the long-term reference
                NumFramesInLTList++;

            }    // long term reference
        }    // for pFrm

        if ((NumFramesInL0List+NumFramesInL1List+NumFramesInLTList) < MAX_NUM_REF_FRAMES) {
            // Complete L0 and L1 lists
            for (i=0; i<NumFramesInL1List; i++) // Add future short term references to L0 list, after previous
                pRefPicList0[NumFramesInL0List+i] = pRefPicList1[i];

            for (i=0; i<NumFramesInL0List; i++) // Add previous short term references to L1 list, after future
                pRefPicList1[NumFramesInL1List+i] = pRefPicList0[i];

            for (i=0; i<NumFramesInLTList; i++){ // Add long term list to both L0 and L1
                pRefPicList0[NumFramesInL0List+NumFramesInL1List+i] = LTRefPicList[i];
                pRefPicList1[NumFramesInL0List+NumFramesInL1List+i] = LTRefPicList[i];
            }

            // Special rule: When L1 has more than one entry and L0 == L1, all entries,
            // swap the first two entries of L1.
            // They can be equal only if there are no future or no previous short term
            // references.
            if ((NumFramesInL0List == 0 || NumFramesInL1List == 0) && ((NumFramesInL0List+NumFramesInL1List+NumFramesInLTList) > 1)){
                pRefPicList1[0] = pRefPicList0[1];
                pRefPicList1[1] = pRefPicList0[0];
            }
        }else{
            // too many reference frames
            VM_ASSERT(0);
        }
    } else {
        // Short term references:
        // Need L0 and L1 lists. Both contain 2 sets of reference frames ordered
        // by PicOrderCnt. The "previous" set contains the reference frames with
        // a PicOrderCnt < current frame. The "future" set contains the reference
        // frames with a PicOrderCnt > current frame. In both cases the ordering
        // is from closest to current frame to farthest. L0 has the previous set
        // followed by the future set; L1 has the future set followed by the previous set.
        // Accomplish this by one pass through the decoded frames list creating
        // the ordered previous list in the L0 array and the ordered future list
        // in the L1 array. Then copy from both to the other for the second set.

        // Long term references:
        // The ordered list is the same for L0 and L1, is ordered by ascending
        // LongTermPicNum. The ordered list is created using local temp then
        // appended to the L0 and L1 lists after the short term references.
        Ipp32s CurrPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(core_enc->m_pCurrentFrame, core_enc->m_field_index, 0);

        for (pFrm = pHead; pFrm; pFrm = pFrm->m_pFutureFrame){
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pFrm)){
                FrmPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pFrm, 0, 2);//returns POC of reference field (or min if both are reference)

                if (FrmPicOrderCnt < CurrPicOrderCnt) {
                    j=0; // Previous reference to L0, order large to small
                    while (j < NumFramesInL0List &&
                        (H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList0[j], 0, 2) > FrmPicOrderCnt))
                        j++;

                    if (pRefPicList0[j]){ // make room if needed
                        if( NumFramesInL0List == MAX_NUM_REF_FRAMES ){  NumFramesInL0List--;  VM_ASSERT(0); }  // Avoid writing beyond end of list, discard last element
                        for (k=NumFramesInL0List; k>j; k--) pRefPicList0[k] = pRefPicList0[k-1];
                    }
                    pRefPicList0[j] = pFrm; // add the short-term reference
                    NumFramesInL0List++;
                }else{
                    j=0; // Future reference to L1, order small to large
                    while (j < NumFramesInL1List &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList1[j], 0, 2) < FrmPicOrderCnt)
                        j++;

                    if (pRefPicList1[j]) { // make room if needed
                        if( NumFramesInL1List == MAX_NUM_REF_FRAMES ){  NumFramesInL1List--;  VM_ASSERT(0); }  // Avoid writing beyond end of list, discard last element
                        for (k=NumFramesInL1List; k>j; k--) pRefPicList1[k] = pRefPicList1[k-1];
                    }
                    pRefPicList1[j] = pFrm; // add the short-term reference
                    NumFramesInL1List++;
                }
            } else if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pFrm)) {
                LongTermPicNum = H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pFrm, 0, 2);
                j=0; // order smallest to largest
                while (j < NumFramesInLTList &&
                    H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(LTRefPicList[j], 0, 2) < LongTermPicNum)
                    j++;

                if (LTRefPicList[j]){ // make room if needed
                    if( NumFramesInLTList == MAX_NUM_REF_FRAMES ){  NumFramesInLTList--;  VM_ASSERT(0); }  // Avoid writing beyond end of list, discard last element
                    for (k=NumFramesInLTList; k>j; k--) LTRefPicList[k] = LTRefPicList[k-1];
                }
                LTRefPicList[j] = pFrm; // add the long-term reference
                NumFramesInLTList++;
            }    // long term reference

            if ((NumFramesInL0List+NumFramesInL1List+NumFramesInLTList) < MAX_NUM_REF_FRAMES){
                // Complete L0 and L1 lists
                // Add future short term references to L0 list, after previous
                for (i=0; i<NumFramesInL1List; i++) pRefPicList0[NumFramesInL0List+i] = pRefPicList1[i];

                // Add previous short term references to L1 list, after future
                for (i=0; i<NumFramesInL0List; i++) pRefPicList1[NumFramesInL1List+i] = pRefPicList0[i];

                // Add long term list to both L0 and L1
                for (i=0; i<NumFramesInLTList; i++){
                    pRefPicList0[NumFramesInL0List+NumFramesInL1List+i] = LTRefPicList[i];
                    pRefPicList1[NumFramesInL0List+NumFramesInL1List+i] = LTRefPicList[i];
                }

                // Special rule: When L1 has more than one entry and L0 == L1, all entries,
                // swap the first two entries of L1.
                // They can be equal only if there are no future or no previous short term
                // references. For fields will be done later.
                /*
                if ((NumFramesInL0List == 0 || NumFramesInL1List == 0) &&
                ((NumFramesInL0List+NumFramesInL1List+NumFramesInLTList) > 1))
                {
                pRefPicList1[0] = pRefPicList0[1];
                pRefPicList1[1] = pRefPicList0[0];
                }*/
            }
            else
            {
                // too many reference frames
                VM_ASSERT(0);
            }

        }    // for pFrm
    }

#ifdef STORE_PICLIST
    if (refpic == NULL)
        refpic = fopen(__PICLIST_FILE__, "wt");
    if (refpic){
        fprintf(refpic, "B Slice cur_poc %d %d\n",
            H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(core_enc->m_pCurrentFrame, 0, 1),
            H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(core_enc->m_pCurrentFrame, 1, 1));
        fprintf(refpic, "RefPicList 0:\n");
        Ipp32s i;
        for (i = 0; i < NumFramesInL0List; i++){
            fprintf(refpic,"Entry %d poc %d %d picnum %d %d FNW %d str %d\n", i,
                H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList0[i], 0, 1),
                H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList0[i], 1, 1),
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pRefPicList0[i], 0, 0),
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pRefPicList0[i], 1, 0),
                pRefPicList0[i]->m_FrameNumWrap,
                H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList0[i]));
        }
        fprintf(refpic, "RefPicList 1:\n");

        for (i = 0; i < NumFramesInL1List; i++){
            fprintf(refpic, "Entry %d poc %d %d picnum %d %d FNW %d str %d\n",i,
                H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList1[i], 0, 1),
                H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList1[i], 1, 1),
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pRefPicList1[i], 0, 0),
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pRefPicList1[i], 1, 0),
                pRefPicList1[i]->m_FrameNumWrap,
                H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList1[i]));
        }
        fprintf(refpic, "\n");
        fflush(refpic);
    }
#endif

    curr_slice->m_NumRefsInL0List = NumFramesInL0List;
    curr_slice->m_NumRefsInL1List = NumFramesInL1List;
    curr_slice->m_NumRefsInLTList = NumFramesInLTList;

}    // InitBSliceRefPicLists

//////////////////////////////////////////////////////////////////////////////
// InitDistScaleFactor
//  Calculates the scaling factor used for B slice temporal motion vector
//  scaling and for B slice bidir predictin weighting using the picordercnt
//  values from the current and both reference frames, saving the result
//  to the DistScaleFactor array for future use. The array is initialized
//  with out of range values whenever a bitstream unit is received that
//  might invalidate the data (for example a B slice header resulting in
//  modified reference picture lists). For scaling, the list1 [0] entry
//    is always used.
//////////////////////////////////////////////////////////////////////////////
void H264ENC_MAKE_NAME(H264CoreEncoder_InitDistScaleFactor)(
    void* state,
    H264SliceType *curr_slice,
    Ipp32s NumL0RefActive,
    Ipp32s NumL1RefActive,
    H264EncoderFrameType **pRefPicList0,
    H264EncoderFrameType **pRefPicList1,
    Ipp8s       *pFields0,
    Ipp8s       *pFields1)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32s L0Index,L1Index;
    Ipp32u picCntRef0;
    Ipp32u picCntRef1;
    Ipp32u picCntCur;
    Ipp32s DistScaleFactor;

    Ipp32s tb;
    Ipp32s td;
    Ipp32s tx;

    VM_ASSERT(NumL0RefActive <= MAX_NUM_REF_FRAMES);
    VM_ASSERT(pRefPicList1[0]);

    picCntCur = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(
        core_enc->m_pCurrentFrame,
        core_enc->m_field_index,
        0);//m_PicOrderCnt;  Current POC
    for( L1Index = 0; L1Index<NumL1RefActive; L1Index++ ){
        if(core_enc->m_pCurrentFrame->m_PictureStructureForRef>=FRM_STRUCTURE){
            picCntRef1 = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList1[L1Index], 0, 0);
        }else{
            Ipp32s RefField = H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                pRefPicList1[L1Index],
                GetReferenceField(pFields1, L1Index));
            picCntRef1 = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList1[L1Index], RefField, 0);
        }

        for (L0Index=0; L0Index<NumL0RefActive; L0Index++){
            if(core_enc->m_pCurrentFrame->m_PictureStructureForRef>=FRM_STRUCTURE)
            {
                VM_ASSERT(pRefPicList0[L0Index]);
                picCntRef0 = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList0[L0Index], 0, 0);
            }
            else
            {
                Ipp8s RefFieldTop;
                //ref0
                RefFieldTop  = GetReferenceField(pFields0,L0Index);
                VM_ASSERT(pRefPicList0[L0Index]);
                picCntRef0 = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(
                    pRefPicList0[L0Index],
                    H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                        pRefPicList1[L1Index],
                        RefFieldTop),
                    0);
            }
            CalculateDSF(L0Index);
        }
    }

#if 0  //FIXME need to fix it for AFRM pictures
    if (core_enc->m_pCurrentFrame->m_PictureStructureForRef==AFRM_STRUCTURE){
        // [curmb field],[ref1field],[ref0field]
        pDistScaleFactor = curr_slice->DistScaleFactorAFF[0][0][0];        //complementary field pairs, cf=top r1=top,r0=top
        pDistScaleFactorMV = curr_slice->DistScaleFactorMVAFF[0][0][0];  //complementary field pairs, cf=top r1=top,r0=top

        picCntCur = core_enc->m_pCurrentFrame->PicOrderCnt(0,1);
        picCntRef1 = pRefPicList1[0]->PicOrderCnt(0,1);

        for (L0Index=0; L0Index<NumL0RefActive; L0Index++){
            VM_ASSERT(pRefPicList0[L0Index]);

            picCntRef0 = pRefPicList0[L0Index]->PicOrderCnt(0,1);
            CalculateDSF(L0Index);
        }

        pDistScaleFactor =curr_slice->DistScaleFactorAFF[0][0][1];        //complementary field pairs, cf=top r1=top,r0=bottom
        pDistScaleFactorMV = curr_slice->DistScaleFactorMVAFF[0][0][1];  //complementary field pairs, cf=top r1=top,r0=bottom

        picCntCur = core_enc->m_pCurrentFrame->PicOrderCnt(0,1);
        picCntRef1 = pRefPicList1[0]->PicOrderCnt(0,1);

        for (L0Index=0; L0Index<NumL0RefActive; L0Index++){
            VM_ASSERT(pRefPicList0[L0Index]);

            picCntRef0 = pRefPicList0[L0Index]->PicOrderCnt(1,1);
            CalculateDSF(L0Index);
        }
        pDistScaleFactor = curr_slice->DistScaleFactorAFF[0][1][0];        //complementary field pairs, cf=top r1=bottom,r0=top
        pDistScaleFactorMV = curr_slice->DistScaleFactorMVAFF[0][1][0];  //complementary field pairs, cf=top r1=bottom,r0=top

        picCntCur = core_enc->m_pCurrentFrame->PicOrderCnt(0,1);
        picCntRef1 = pRefPicList1[0]->PicOrderCnt(1,1);

        for (L0Index=0; L0Index<NumL0RefActive; L0Index++){
            VM_ASSERT(pRefPicList0[L0Index]);

            picCntRef0 = pRefPicList0[L0Index]->PicOrderCnt(0,1);
            CalculateDSF(L0Index);
        }
        pDistScaleFactor = curr_slice->DistScaleFactorAFF[0][1][1];        //complementary field pairs, cf=top r1=bottom,r0=bottom
        pDistScaleFactorMV = curr_slice->DistScaleFactorMVAFF[0][1][1];  //complementary field pairs, cf=top r1=bottom,r0=bottom

        picCntCur = core_enc->m_pCurrentFrame->PicOrderCnt(0,1);
        picCntRef1 = pRefPicList1[0]->PicOrderCnt(1,1);

        for (L0Index=0; L0Index<NumL0RefActive; L0Index++){
            VM_ASSERT(pRefPicList0[L0Index]);

            picCntRef0 = pRefPicList0[L0Index]->PicOrderCnt(1,1);
            CalculateDSF(L0Index);
        }

        /*--------------------------------------------------------------------*/
        pDistScaleFactor = curr_slice->DistScaleFactorAFF[1][0][0];        //complementary field pairs, cf=bottom r1=top,r0=top
        pDistScaleFactorMV = curr_slice->DistScaleFactorMVAFF[1][0][0];  //complementary field pairs, cf=bottom r1=top,r0=top


        picCntCur = core_enc->m_pCurrentFrame->PicOrderCnt(1,1);
        picCntRef1 = pRefPicList1[0]->PicOrderCnt(0,1);

        for (L0Index=0; L0Index<NumL0RefActive; L0Index++){
            VM_ASSERT(pRefPicList0[L0Index]);

            picCntRef0 = pRefPicList0[L0Index]->PicOrderCnt(0,1);
            CalculateDSF(L0Index);
        }

        pDistScaleFactor = curr_slice->DistScaleFactorAFF[1][0][1];        //complementary field pairs, cf=bottom r1=top,r0=bottom
        pDistScaleFactorMV = curr_slice->DistScaleFactorMVAFF[1][0][1];  //complementary field pairs, cf=bottom r1=top,r0=bottom

        picCntCur = core_enc->m_pCurrentFrame->PicOrderCnt(1,1);
        picCntRef1 = pRefPicList1[0]->PicOrderCnt(0,1);

        for (L0Index=0; L0Index<NumL0RefActive; L0Index++){
            VM_ASSERT(pRefPicList0[L0Index]);

            picCntRef0 = pRefPicList0[L0Index]->PicOrderCnt(1,1);
            CalculateDSF(L0Index);
        }
        pDistScaleFactor = curr_slice->DistScaleFactorAFF[1][1][0];        //complementary field pairs, cf=bottom r1=bottom,r0=top
        pDistScaleFactorMV = curr_slice->DistScaleFactorMVAFF[1][1][0];  //complementary field pairs, cf=bottom r1=bottom,r0=top

        picCntCur = core_enc->m_pCurrentFrame->PicOrderCnt(1,1);
        picCntRef1 = pRefPicList1[0]->PicOrderCnt(1,1);

        for (L0Index=0; L0Index<NumL0RefActive; L0Index++){
            VM_ASSERT(pRefPicList0[L0Index]);

            picCntRef0 = pRefPicList0[L0Index]->PicOrderCnt(0,1);
            CalculateDSF(L0Index);
        }
        pDistScaleFactor = curr_slice->DistScaleFactorAFF[1][1][1];        //complementary field pairs, cf=bottom r1=bottom,r0=bottom
        pDistScaleFactorMV = curr_slice->DistScaleFactorMVAFF[1][1][1];  //complementary field pairs, cf=bottom r1=bottom,r0=bottom

        picCntCur = core_enc->m_pCurrentFrame->PicOrderCnt(1,1);
        picCntRef1 = pRefPicList1[0]->PicOrderCnt(1,1);

        for (L0Index=0; L0Index<NumL0RefActive; L0Index++){
            VM_ASSERT(pRefPicList0[L0Index]);

            picCntRef0 = pRefPicList0[L0Index]->PicOrderCnt(1,1);
            CalculateDSF(L0Index);
        }
    }

#endif
}    // InitDistScaleFactor

Ipp32s H264ENC_MAKE_NAME(MapColToList0)(
    EncoderRefPicListStructType *ref_pic_list_struct,
    H264EncoderFrameType **pRefPicList0,
    Ipp32s RefIdxCol,
    Ipp32s numRefL0Active)
{
    bool bFound = false;
    Ipp32s ref_idx = 0;
    // Translate the reference index of the colocated to current
    // L0 index to the same reference picture, using PicNum or
    // LongTermPicNum as id criteria.
    if( ref_pic_list_struct->m_RefPicList[RefIdxCol] == NULL ) return -1;

    if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(ref_pic_list_struct->m_RefPicList[RefIdxCol]))
    {
        Ipp32s RefPicNum = H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(
            ref_pic_list_struct->m_RefPicList[RefIdxCol],
            0,
            3);
        Ipp32s RefPOC = ref_pic_list_struct->m_POC[RefIdxCol];

        while (!bFound){         // find matching reference frame on current slice list 0
            if( ref_idx >= numRefL0Active ) break; //Don't go beyond L0 active refs
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList0[ref_idx]) &&
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pRefPicList0[ref_idx], 0, 3) == RefPicNum &&
                H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList0[ref_idx], 0, 3) == RefPOC)
                bFound = true;
            else
                ref_idx++;
        }

        if (!bFound)  return -1;
    } else if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(ref_pic_list_struct->m_RefPicList[RefIdxCol])){
        Ipp32s RefPicNum = H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(
            ref_pic_list_struct->m_RefPicList[RefIdxCol],
            0,
            3);
        Ipp32s RefPOC = ref_pic_list_struct->m_POC[RefIdxCol];

        while (!bFound){ // find matching reference frame on current slice list 0
            if( ref_idx >= numRefL0Active ) break; //Don't go beyond L0 active refs
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pRefPicList0[ref_idx]) &&
                H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pRefPicList0[ref_idx], 0, 3) == RefPicNum &&
                H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefPicList0[ref_idx], 0, 3) == RefPOC)
                bFound = true;
            else
                ref_idx++;
        }
        if (!bFound) return -1;
    } else return -1;
        // colocated is in a reference that is not marked as Ipp16s-term
        // or long-term, should not happen
        // Well it can be happen since in case of (num_ref_frames == 1) and
        // this frame is already unmarked as reference
        // So we could not use temporal direct prediction.
    return ref_idx;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_InitMapColMBToList0)(
    H264SliceType *curr_slice,
    Ipp32s NumL0RefActive,
    H264EncoderFrameType **pRefPicList0,
    H264EncoderFrameType **pRefPicList1)
{
    Ipp32s ref;
    EncoderRefPicListStructType *ref_pic_list_struct;
    VM_ASSERT(pRefPicList1[0]);

    for(ref=0; ref<16; ref++ ){
        ref_pic_list_struct = H264ENC_MAKE_NAME(H264EncoderFrame_GetRefPicList)(
            pRefPicList1[0],
            curr_slice->m_slice_number,
            LIST_0);
        curr_slice->MapColMBToList0[ref][LIST_0] = H264ENC_MAKE_NAME(MapColToList0)(ref_pic_list_struct, pRefPicList0, ref, NumL0RefActive );

        ref_pic_list_struct = H264ENC_MAKE_NAME(H264EncoderFrame_GetRefPicList)(
            pRefPicList1[0],
            curr_slice->m_slice_number,
            LIST_1);
        curr_slice->MapColMBToList0[ref][LIST_1] = H264ENC_MAKE_NAME(MapColToList0)(ref_pic_list_struct, pRefPicList0, ref, NumL0RefActive );
    }
}

void H264ENC_MAKE_NAME(H264CoreEncoder_AdjustRefPicListForFields)(
    void* state,
    H264EncoderFrameType **pRefPicList,Ipp8s *pFields)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    H264EncoderFrameType *TempList[MAX_NUM_REF_FRAMES+1];
    Ipp8u TempFields[MAX_NUM_REF_FRAMES+1];
    //walk through list and set correct indices
    Ipp32s i=0,j=0,numSTR=0,numLTR=0;
    Ipp32s num_same_parity=0,num_opposite_parity=0;
    Ipp8s current_parity = core_enc->m_pCurrentFrame->m_bottom_field_flag[core_enc->m_field_index];
    //first scan the list to determine number of shortterm and longterm reference frames
    while (pRefPicList[numSTR] && H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList[numSTR]))
        numSTR++;
    while (pRefPicList[numSTR+numLTR] && H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pRefPicList[numSTR + numLTR]))
        numLTR++;
    while(num_same_parity<numSTR ||  num_opposite_parity<numSTR)
    {
        //try to fill shorttermref fields with the same parity first
        if (num_same_parity<numSTR)
        {
            Ipp32s ref_field = H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                pRefPicList[num_same_parity],
                current_parity);

            while (num_same_parity<numSTR &&
                !H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(pRefPicList[num_same_parity], ref_field))
                num_same_parity++;
            if (num_same_parity<numSTR)
            {
                TempList[i] = pRefPicList[num_same_parity];
                TempFields[i] = current_parity;
                i++;
                num_same_parity++;
            }
        }
        //now process opposite parity
        if (num_opposite_parity<numSTR)
        {
            Ipp32s ref_field = H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                pRefPicList[num_opposite_parity],
                !current_parity);

            while (num_opposite_parity < numSTR &&
                !H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(pRefPicList[num_opposite_parity], ref_field))
                num_opposite_parity++;
            if (num_opposite_parity<numSTR) //selected field is reference
            {
                TempList[i] = pRefPicList[num_opposite_parity];
                TempFields[i] = !current_parity;
                i++;
                num_opposite_parity++;
            }
        }
    }
    core_enc->m_NumShortEntriesInList = (Ipp8u) i;
    num_same_parity=num_opposite_parity=0;
    //now processing LongTermRef
    while(num_same_parity<numLTR ||  num_opposite_parity<numLTR)
    {
        //try to fill longtermref fields with the same parity first
        if (num_same_parity<numLTR)
        {
            Ipp32s ref_field = H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                pRefPicList[num_same_parity + numSTR],
                current_parity);

            while (num_same_parity < numLTR &&
                !H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(
                    pRefPicList[num_same_parity + numSTR],
                    ref_field))
                num_same_parity++;
            if (num_same_parity<numLTR)
            {
                TempList[i] = pRefPicList[num_same_parity+numSTR];
                TempFields[i] = current_parity;
                i++;
                num_same_parity++;
            }
        }
        //now process opposite parity
        if (num_opposite_parity<numLTR)
        {
            Ipp32s ref_field = H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                pRefPicList[num_opposite_parity + numSTR],
                !current_parity);

            while (num_opposite_parity < numLTR &&
                !H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(
                    pRefPicList[num_opposite_parity + numSTR],
                    ref_field))
                num_opposite_parity++;
            if (num_opposite_parity<numLTR) //selected field is reference
            {
                TempList[i] = pRefPicList[num_opposite_parity+numSTR];
                TempFields[i] = !current_parity;
                i++;
                num_opposite_parity++;
            }
        }
    }
    core_enc->m_NumLongEntriesInList = (Ipp8u) (i - core_enc->m_NumShortEntriesInList);
    j=0;
#ifdef STORE_PICLIST
    if (refpic==NULL) refpic=fopen(__PICLIST_FILE__,VM_STRING("wt"));
    if (refpic)
        fprintf(refpic,"Reordering for fields:\n");
#endif
    while(j<i)//copy data back to list
    {
        pRefPicList[j]=TempList[j];
        pFields[j] = TempFields[j];
#ifdef STORE_PICLIST
        if (refpic)
            fprintf(refpic,"Entry %d poc %d parity %d\n",
                j,
                H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(
                    pRefPicList[j],
                    H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                        pRefPicList[j],
                        pFields[j]),
                    1),
                pFields[j]);
#endif

        j++;
    }
#ifdef STORE_PICLIST
    if (refpic){
        fprintf(refpic,"\n");
        fflush(refpic);
    }
#endif
    while(j<MAX_NUM_REF_FRAMES)//fill remaining entries
    {
        pRefPicList[j]=NULL;
        pFields[j] = -1;
        j++;
    }
    return;
}


//////////////////////////////////////////////////////////////////////////////
// ReOrderRefPicList
//  Use reordering info from the slice header to reorder (update) L0 or L1
//  reference picture list.
//////////////////////////////////////////////////////////////////////////////
void H264ENC_MAKE_NAME(H264CoreEncoder_ReOrderRefPicList)(
    void* state,
    bool bIsFieldSlice,
    H264EncoderFrameType **pRefPicList,
    Ipp8s         *pFields,
    RefPicListReorderInfo *pReorderInfo,
    Ipp32s MaxPicNum,
    Ipp32s NumRefActive)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32u i;
    Ipp32s j;
    Ipp32s PicNumNoWrap;
    Ipp32s PicNum;
    Ipp32s PicNumPred;
    Ipp32s PicNumCurr;
    H264EncoderFrameType *tempFrame[2];
    Ipp8s tempFields[2];
    Ipp32u NumDuplicates;

    // Reference: Reordering process for reference picture lists, 8.2.4.3
    if (!bIsFieldSlice)
    {
        PicNumCurr = H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(core_enc->m_pCurrentFrame, 0, 3);
        PicNumPred = PicNumCurr;

        for (i=0; i<pReorderInfo->num_entries; i++)
        {
            if (pReorderInfo->reordering_of_pic_nums_idc[i] < 2)
            {
                // short term reorder
                if (pReorderInfo->reordering_of_pic_nums_idc[i] == 0)
                {
                    PicNumNoWrap = PicNumPred - pReorderInfo->reorder_value[i];
                    if (PicNumNoWrap < 0)
                        PicNumNoWrap += MaxPicNum;
                }
                else
                {
                    PicNumNoWrap = PicNumPred + pReorderInfo->reorder_value[i];
                    if (PicNumNoWrap >= MaxPicNum)
                        PicNumNoWrap -= MaxPicNum;
                }
                PicNumPred = PicNumNoWrap;

                PicNum = PicNumNoWrap;
                if (PicNum > PicNumCurr)
                    PicNum -= MaxPicNum;

                // Find the PicNum frame.
                for (j=0; pRefPicList[j] !=NULL; j++)
                    if (pRefPicList[j] != NULL &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pRefPicList[j]) &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pRefPicList[j], 0, 3) == PicNum)
                        break;

                // error if not found, should not happen
                VM_ASSERT(pRefPicList[j]);

                // Place picture with PicNum on list, shifting pictures
                // down by one while removing any duplication of picture with PicNum.
                tempFrame[0] = pRefPicList[j];    // PicNum frame just found
                NumDuplicates = 0;
                for (j=i; j<NumRefActive || pRefPicList[j] !=NULL; j++)
                {
                    if (NumDuplicates == 0)
                    {
                        // shifting pictures down
                        tempFrame[1] = pRefPicList[j];
                        pRefPicList[j] = tempFrame[0];
                        tempFrame[0] = tempFrame[1];
                    }
                    else if (NumDuplicates == 1)
                    {
                        // one duplicate of PicNum made room for new entry, just
                        // look for more duplicates to eliminate
                        tempFrame[0] = pRefPicList[j];
                    }
                    else
                    {
                        // >1 duplicate found, shifting pictures up
                        pRefPicList[j - NumDuplicates + 1] = tempFrame[0];
                        tempFrame[0] = pRefPicList[j];
                    }
                    if (tempFrame[0] == NULL)
                        break;        // end of valid reference frames
                    if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(tempFrame[0]) &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(tempFrame[0], 0, 3) == PicNum)
                        NumDuplicates++;
                }
            }    // short term reorder
            else
            {
                // long term reorder
                PicNum = pReorderInfo->reorder_value[i];

                // Find the PicNum frame.
                for (j=0; pRefPicList[j] !=NULL; j++)
                    if (pRefPicList[j] != NULL &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pRefPicList[j]) &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pRefPicList[j], 0, 3) == PicNum)
                        break;

                // error if not found, should not happen
                VM_ASSERT(pRefPicList[j]);

                // Place picture with PicNum on list, shifting pictures
                // down by one while removing any duplication of picture with PicNum.
                tempFrame[0] = pRefPicList[j];    // PicNum frame just found
                NumDuplicates = 0;
                for (j=i; j<NumRefActive || pRefPicList[j] !=NULL; j++)
                {
                    if (NumDuplicates == 0)
                    {
                        // shifting pictures down
                        tempFrame[1] = pRefPicList[j];
                        pRefPicList[j] = tempFrame[0];
                        tempFrame[0] = tempFrame[1];
                    }
                    else if (NumDuplicates == 1)
                    {
                        // one duplicate of PicNum made room for new entry, just
                        // look for more duplicates to eliminate
                        tempFrame[0] = pRefPicList[j];
                    }
                    else
                    {
                        // >1 duplicate found, shifting pictures up
                        pRefPicList[j - NumDuplicates + 1] = tempFrame[0];
                        tempFrame[0] = pRefPicList[j];
                    }
                    if (tempFrame[0] == NULL)
                        break;        // end of valid reference frames
                    if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(tempFrame[0]) &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(tempFrame[0], 0, 3) == PicNum)
                        NumDuplicates++;
                }
            }    // long term reorder
        }    // for i
    }
    else
    {
        //NumRefActive<<=1;
        PicNumCurr = H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(
            core_enc->m_pCurrentFrame,
            core_enc->m_field_index,
            0);
        PicNumPred = PicNumCurr;

        for (i=0; i<pReorderInfo->num_entries; i++)
        {
            if (pReorderInfo->reordering_of_pic_nums_idc[i] < 2)
            {
                // short term reorder
                if (pReorderInfo->reordering_of_pic_nums_idc[i] == 0)
                {
                    PicNumNoWrap = PicNumPred - pReorderInfo->reorder_value[i];
                    if (PicNumNoWrap < 0)
                        PicNumNoWrap += MaxPicNum;
                }
                else
                {
                    PicNumNoWrap = PicNumPred + pReorderInfo->reorder_value[i];
                    if (PicNumNoWrap >= MaxPicNum)
                        PicNumNoWrap -= MaxPicNum;
                }
                PicNumPred = PicNumNoWrap;

                PicNum = PicNumNoWrap;
                if (PicNum > PicNumCurr)
                    PicNum -= MaxPicNum;

                // Find the PicNum frame.
                for (j=0; pRefPicList[j] !=NULL; j++)
                    if (pRefPicList[j] != NULL &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(
                            pRefPicList[j],
                            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                                pRefPicList[j],
                                pFields[j])) &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(
                            pRefPicList[j],
                            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                                pRefPicList[j],
                                pFields[j]),
                            1) == PicNum)
                        break;

                // error if not found, should not happen
                VM_ASSERT(pRefPicList[j]);

                // Place picture with PicNum on list, shifting pictures
                // down by one while removing any duplication of picture with PicNum.
                tempFrame[0] = pRefPicList[j];    // PicNum frame just found
                tempFields[0] = pFields[j];

                NumDuplicates = 0;
                for (j=i; j<NumRefActive || pRefPicList[j] !=NULL; j++)
                {
                    if (NumDuplicates == 0)
                    {
                        // shifting pictures down
                        tempFrame[1] = pRefPicList[j];
                        pRefPicList[j] = tempFrame[0];
                        tempFrame[0] = tempFrame[1];

                        tempFields[1] = pFields[j];
                        pFields[j] = tempFields[0];
                        tempFields[0] = tempFields[1];
                    }
                    else if (NumDuplicates == 1)
                    {
                        // one duplicate of PicNum made room for new entry, just
                        // look for more duplicates to eliminate
                        tempFrame[0] = pRefPicList[j];
                        tempFields[0] = pFields[j];

                    }
                    else
                    {
                        // >1 duplicate found, shifting pictures up
                        pRefPicList[j - NumDuplicates + 1] = tempFrame[0];
                        tempFrame[0] = pRefPicList[j];

                        pFields[j - NumDuplicates + 1] = tempFields[0];
                        tempFields[0] = pFields[j];

                    }
                    if (tempFrame[0] == NULL)
                        break;        // end of valid reference frames
                    if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(
                            tempFrame[0],
                            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                                tempFrame[0],
                                tempFields[0])) &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(
                            tempFrame[0],
                            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                                tempFrame[0],
                                tempFields[0]),
                            1) == PicNum)
                        NumDuplicates++;
                }
            }    // short term reorder
            else
            {
                // long term reorder
                PicNum = pReorderInfo->reorder_value[i];

                // Find the PicNum frame.
                for (j=0; pRefPicList[j] !=NULL; j++)
                    if (pRefPicList[j] != NULL &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(
                            pRefPicList[j],
                            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                                pRefPicList[j],
                                pFields[j])) &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(
                            pRefPicList[j],
                            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                                pRefPicList[j],
                                pFields[j]),
                            1) == PicNum)
                        break;

                // error if not found, should not happen
                VM_ASSERT(pRefPicList[j]);

                // Place picture with PicNum on list, shifting pictures
                // down by one while removing any duplication of picture with PicNum.
                tempFrame[0] = pRefPicList[j];    // PicNum frame just found
                tempFields[0] = pFields[j];

                NumDuplicates = 0;
                for (j=i; j<NumRefActive || pRefPicList[j] !=NULL; j++)
                {
                    if (NumDuplicates == 0)
                    {
                        // shifting pictures down
                        tempFrame[1] = pRefPicList[j];
                        pRefPicList[j] = tempFrame[0];
                        tempFrame[0] = tempFrame[1];

                        tempFields[1] = pFields[j];
                        pFields[j] = tempFields[0];
                        tempFields[0] = tempFields[1];
                    }
                    else if (NumDuplicates == 1)
                    {
                        // one duplicate of PicNum made room for new entry, just
                        // look for more duplicates to eliminate
                        tempFrame[0] = pRefPicList[j];
                        tempFields[0] = pFields[j];
                    }
                    else
                    {
                        // >1 duplicate found, shifting pictures up
                        pRefPicList[j - NumDuplicates + 1] = tempFrame[0];
                        tempFrame[0] = pRefPicList[j];

                        pFields[j - NumDuplicates + 1] = tempFields[0];
                        tempFields[0] = pFields[j];
                    }
                    if (tempFrame[0] == NULL)
                        break;        // end of valid reference frames
                    if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(
                            tempFrame[0],
                            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                                tempFrame[0],
                                tempFields[0])) &&
                        H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(
                            tempFrame[0],
                            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                                tempFrame[0],
                                tempFields[0]),
                            1) == PicNum)
                        NumDuplicates++;
                }
            }    // long term reorder
        }    // for i
    }
}    // ReOrderRefPicList

//////////////////////////////////////////////////////////////////////////////
// updateRefPicMarking
//  Called at the completion of decoding a frame to update the marking of the
//  reference pictures in the decoded frames buffer.
//////////////////////////////////////////////////////////////////////////////
Status H264ENC_MAKE_NAME(H264CoreEncoder_UpdateRefPicMarking)(
    void* state)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Status ps = UMC_OK;
    Ipp32u arpmmf_idx;
    Ipp32s PicNum;
    Ipp32s LongTermFrameIdx;
    bool bCurrentisST = true;
    Ipp32u NumShortTermRefs, NumLongTermRefs;

    if (core_enc->m_pCurrentFrame->m_bIsIDRPic){
        // mark all reference pictures as unused
        H264ENC_MAKE_NAME(H264EncoderFrameList_removeAllRef)(&core_enc->m_dpb);
        H264ENC_MAKE_NAME(H264EncoderFrameList_IncreaseRefPicListResetCount)(
            &core_enc->m_dpb,
            core_enc->m_pCurrentFrame);

        if (core_enc->m_SliceHeader.long_term_reference_flag){
            H264ENC_MAKE_NAME(H264EncoderFrame_SetisLongTermRef)(
                core_enc->m_pCurrentFrame,
                core_enc->m_field_index);
            core_enc->m_MaxLongTermFrameIdx = 0;
        }else{
            H264ENC_MAKE_NAME(H264EncoderFrame_SetisShortTermRef)(
                core_enc->m_pCurrentFrame,
                core_enc->m_field_index);
            core_enc->m_MaxLongTermFrameIdx = -1;        // no long term frame indices
        }
    }else{
        Ipp32s LastLongTermFrameIdx = -1;
        // not IDR picture
        if (core_enc->m_SliceHeader.adaptive_ref_pic_marking_mode_flag == 0){
            // sliding window ref pic marking
            // find out how many active reference frames currently in decoded frames buffer
            H264ENC_MAKE_NAME(H264EncoderFrameList_countActiveRefs)(
                &core_enc->m_dpb,
                NumShortTermRefs,
                NumLongTermRefs);
            if (((NumShortTermRefs + NumLongTermRefs) >= (Ipp32u)core_enc->m_SeqParamSet.num_ref_frames) && !core_enc->m_field_index ){
                // mark oldest short term reference as unused
                VM_ASSERT(NumShortTermRefs > 0);
                H264ENC_MAKE_NAME(H264EncoderFrameList_freeOldestShortTermRef)(&core_enc->m_dpb);
            }
        }else{
            // adaptive ref pic marking
            if (core_enc->m_AdaptiveMarkingInfo.num_entries > 0){
                for (arpmmf_idx=0; arpmmf_idx<core_enc->m_AdaptiveMarkingInfo.num_entries; arpmmf_idx++){
                    switch (core_enc->m_AdaptiveMarkingInfo.mmco[arpmmf_idx]) {
                        case 1:
                            // mark a short-term picture as unused for reference
                            // Value is difference_of_pic_nums_minus1
                            PicNum = H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(
                                core_enc->m_pCurrentFrame,
                                core_enc->m_field_index,
                                0) - (core_enc->m_AdaptiveMarkingInfo.value[arpmmf_idx * 2] + 1);
                            H264ENC_MAKE_NAME(H264EncoderFrameList_freeShortTermRef)(
                                &core_enc->m_dpb,
                                PicNum);
                            break;
                        case 2:
                            // mark a long-term picture as unused for reference
                            // value is long_term_pic_num
                            PicNum = core_enc->m_AdaptiveMarkingInfo.value[arpmmf_idx*2];
                            H264ENC_MAKE_NAME(H264EncoderFrameList_freeLongTermRef)(
                                &core_enc->m_dpb,
                                PicNum);
                            break;
                        case 3:
                            // Assign a long-term frame idx to a short-term picture
                            // Value is difference_of_pic_nums_minus1 followed by
                            // long_term_frame_idx. Only this case uses 2 value entries.
                            PicNum = H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(
                                core_enc->m_pCurrentFrame,
                                core_enc->m_field_index,
                                0) - (core_enc->m_AdaptiveMarkingInfo.value[arpmmf_idx*2] + 1);
                            LongTermFrameIdx = core_enc->m_AdaptiveMarkingInfo.value[arpmmf_idx*2+1];

                            // First free any existing LT reference with the LT idx
                            if (LastLongTermFrameIdx !=LongTermFrameIdx) //this is needed since both fields may have equal Idx
                                H264ENC_MAKE_NAME(H264EncoderFrameList_freeLongTermRefIdx)(
                                    &core_enc->m_dpb,
                                    LongTermFrameIdx);

                            H264ENC_MAKE_NAME(H264EncoderFrameList_changeSTtoLTRef)(
                                &core_enc->m_dpb,
                                PicNum,
                                LongTermFrameIdx);
                            LastLongTermFrameIdx = LongTermFrameIdx;
                            break;
                        case 4:
                            // Specify IPP_MAX long term frame idx
                            // Value is max_long_term_frame_idx_plus1
                            // Set to "no long-term frame indices" (-1) when value == 0.
                            core_enc->m_MaxLongTermFrameIdx = core_enc->m_AdaptiveMarkingInfo.value[arpmmf_idx*2] - 1;

                            // Mark any long-term reference frames with a larger LT idx
                            // as unused for reference.
                            H264ENC_MAKE_NAME(H264EncoderFrameList_freeOldLongTermRef)(
                                &core_enc->m_dpb,
                                core_enc->m_MaxLongTermFrameIdx);
                            break;
                        case 5:
                            // Mark all as unused for reference
                            // no value
                            H264ENC_MAKE_NAME(H264EncoderFrameList_removeAllRef)(&core_enc->m_dpb);
                            H264ENC_MAKE_NAME(H264EncoderFrameList_IncreaseRefPicListResetCount)(
                                &core_enc->m_dpb,
                                NULL);
                            core_enc->m_MaxLongTermFrameIdx = -1;        // no long term frame indices
                            // set "previous" picture order count vars for future
                            //m_PicOrderCntMsb = 0;
                            //m_PicOrderCntLsb = 0;
                            core_enc->m_FrameNumOffset = 0;
                            core_enc->m_FrameNum = 0;
                            // set frame_num to zero for this picture, for correct
                            // FrameNumWrap
                            core_enc->m_pCurrentFrame->m_FrameNum = 0;
                            core_enc->m_pCurrentFrame->m_FrameNum = 0;
                            // set POC to zero for this picture, for correct display order
                            //m_pCurrentFrame->setPicOrderCnt(core_enc->m_PicOrderCnt,0);
                            //m_pCurrentFrame->setPicOrderCnt(core_enc->m_PicOrderCnt,1);
                            break;
                        case 6:
                            // Assign long term frame idx to current picture
                            // Value is long_term_frame_idx
                            LongTermFrameIdx = core_enc->m_AdaptiveMarkingInfo.value[arpmmf_idx*2];

                            // First free any existing LT reference with the LT idx
                            H264ENC_MAKE_NAME(H264EncoderFrameList_freeLongTermRefIdx)(
                                &core_enc->m_dpb,
                                LongTermFrameIdx);

                            // Mark current
                            H264ENC_MAKE_NAME(H264EncoderFrame_SetisLongTermRef)(
                                core_enc->m_pCurrentFrame,
                                core_enc->m_field_index);
                            core_enc->m_pCurrentFrame->m_LongTermFrameIdx = LongTermFrameIdx;
                            bCurrentisST = false;
                            break;
                        case 0:
                        default:
                            // invalid mmco command in bitstream
                            VM_ASSERT(0);
                            ps = UMC_ERR_INVALID_STREAM;
                    }    // switch
                }    // for arpmmf_idx
            }
        }    // adaptive ref pic marking
    }    // not IDR picture

    if (bCurrentisST)   // set current as short term
        H264ENC_MAKE_NAME(H264EncoderFrame_SetisShortTermRef)(
            core_enc->m_pCurrentFrame,
            core_enc->m_field_index);

    return ps;
}    // updateRefPicMarking

/*************************************************************************************************/
void H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaNonMBAFF)(
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num)) // luma
    {
        Block->block_num+=3;
        Block->mb_num=cur_mb.MacroblockNeighbours.mb_A;
    } else {
        Block->block_num--;
        Block->mb_num=cur_mb.uMB;
    }
    return;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBChromaNonMBAFF)(
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    if (CHROMA_BLOCK_IS_ON_LEFT_EDGE(Block->block_num)) //chroma
    {
        Block->block_num+=1;
        Block->mb_num=cur_mb.MacroblockNeighbours.mb_A;
    } else {
        Block->block_num--;
        Block->mb_num=cur_mb.uMB;
    }
    return;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaMBAFF)(
    void* state,
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block,
    Ipp32s AdditionalDecrement)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    bool curmbfff = !pGetMBFieldDecodingFlag(cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = (cur_mb.uMB&1)==0;
    Ipp32s MB_X = cur_mb.MacroblockNeighbours.mb_A;
    Ipp32s MB_N;
        //luma
    if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num))
    {
        if (MB_X>=0)
        {
            Ipp8u xfff=!GetMBFieldDecodingFlag(core_enc->m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
            if (curmbfff)
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        // 1 1 1
                        MB_N=MB_X;
                    }
                    else
                    {
                        // 1 1 0
                        Ipp32u yN = Block->block_num/4;
                        yN*=4;
                        yN-=AdditionalDecrement;
                        yN/=2;
                        Block->block_num=(yN/4)*4;
                        if (AdditionalDecrement)
                            MB_N=MB_X  + 1;
                        else
                            MB_N=MB_X;
                        AdditionalDecrement=0;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 1 0 1
                        MB_N=MB_X + 1;
                    }
                    else
                    {
                        // 1 0 0
                        Ipp32u yN = Block->block_num/4;
                        yN*=4;
                        yN-=AdditionalDecrement;
                        yN+=16;
                        yN/=2;
                        Block->block_num=(yN/4)*4;
                        if (AdditionalDecrement)
                            MB_N=MB_X + 1;
                        else
                            MB_N=MB_X;
                        AdditionalDecrement=0;
                    }
                }
            }
            else
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        //0 1 1
                        Ipp32u yN = Block->block_num/4;
                        yN*=4;
                        yN-=AdditionalDecrement;
                        yN*=2;
                        if (yN<16)
                        {
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN-=16;
                            MB_N=MB_X + 1;
                        }
                        Block->block_num=(yN/4)*4;
                        AdditionalDecrement=0;
                    }
                    else
                    {
                        // 0 1 0
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 0 0 1
                        Ipp32u yN = Block->block_num/4;
                        yN*=4;
                        yN-=AdditionalDecrement;
                        yN*=2;
                        if (yN<15)
                        {
                            yN++;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN-=15;
                            MB_N=MB_X + 1;
                        }
                        Block->block_num=(yN/4)*4;
                        AdditionalDecrement=0;
                    }
                    else
                    {
                        // 0 0 0
                        MB_N=MB_X + 1;
                    }
                }
            }
        }
        else
        {
            Block->mb_num = -1;//no left neighbours
            return;
        }
        Block->block_num+=3-4*AdditionalDecrement;
        Block->mb_num = MB_N;
    }
    else
    {
        Block->block_num--;
        Block->mb_num = cur_mb.uMB;
    }
    return;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBChromaMBAFF)(
    void* state,
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    bool curmbfff = !pGetMBFieldDecodingFlag(cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = (cur_mb.uMB&1)==0;
    Ipp32s MB_X = cur_mb.MacroblockNeighbours.mb_A;
    Ipp32s MB_N;
    //chroma
    if (CHROMA_BLOCK_IS_ON_LEFT_EDGE(Block->block_num))
    {
        Ipp32u dec_value=16;
        if (Block->block_num>=20) dec_value=20;
        Block->block_num-=dec_value;
        if (MB_X>=0) //left mb addr vaild?
        {
            Ipp8u xfff=!GetMBFieldDecodingFlag(core_enc->m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
            if (curmbfff)
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        // 1 1 1
                        MB_N=MB_X;
                    }
                    else
                    {
                        // 1 1 0
                        Ipp32u yN = Block->block_num/2, xN=Block->block_num%2;
                        yN/=2;
                        Block->block_num=yN*2+xN;
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 1 0 1
                        MB_N=MB_X+1;
                    }
                    else
                    {
                        // 1 0 0
                        Ipp32u yN = Block->block_num/2, xN=Block->block_num%2;
                        yN+=2;
                        yN/=2;
                        Block->block_num=yN*2+xN;
                        MB_N=MB_X;
                    }
                }
            }
            else
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        //0 1 1
                        Ipp32u yN = Block->block_num/2, xN=Block->block_num%2;
                        if (yN<1)
                        {
                            yN*=2;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN*=2;
                            yN-=2;
                            MB_N=MB_X+1;
                        }
                        Block->block_num=yN*2+xN;
                    }
                    else
                    {
                        // 0 1 0
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 0 0 1
                        Ipp32u yN = Block->block_num/2, xN=Block->block_num%2;
                        if (yN<1)
                        {
                            yN*=8;
                            yN++;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN*=8;
                            yN-=7;
                            MB_N=MB_X + 1;
                        }
                        Block->block_num=(yN/4)*2+xN;
                    }
                    else
                    {
                        // 0 0 0
                        MB_N=MB_X + 1;
                    }
                }
            }
        }
        else
        {
            Block->mb_num = -1;//no left neighbours
            return;
        }
        Block->block_num+=dec_value;
        Block->block_num+=1;
        Block->mb_num = MB_N;
    }
    else
    {
        Block->block_num--;
        Block->mb_num = cur_mb.uMB;
    }
    return;
}
void H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBLumaNonMBAFF)(
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    if (BLOCK_IS_ON_TOP_EDGE(Block->block_num)) //luma
    {
        Block->block_num+=12;
        Block->mb_num = cur_mb.MacroblockNeighbours.mb_B;
    } else {
        Block->block_num -= 4;
        Block->mb_num = cur_mb.uMB;
    }
    return;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBChromaNonMBAFF)(
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    if (CHROMA_BLOCK_IS_ON_TOP_EDGE(Block->block_num)) //chroma
    {
        Block->block_num+=2;
        Block->mb_num  = cur_mb.MacroblockNeighbours.mb_B;
    } else {
        Block->block_num-=2;
        Block->mb_num = cur_mb.uMB;
    }
    return;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBLumaMBAFF)(
    void* state,
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block,
    bool is_deblock_calls)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    bool curmbfff = !pGetMBFieldDecodingFlag(cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = (cur_mb.uMB&1)==0;
    Ipp32s pair_offset = curmbtf? 1:-1;
    Ipp32s MB_X;
    Ipp32s MB_N;
    //luma
    if (BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        if (curmbfff && !curmbtf)
        {
            MB_N = cur_mb.uMB + pair_offset;
            Block->block_num+=12;
        }
        else
        {
            MB_X = cur_mb.MacroblockNeighbours.mb_B;
            if (MB_X>=0)
            {
                Ipp8u xfff=!GetMBFieldDecodingFlag(core_enc->m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
                MB_N=MB_X;
                Block->block_num+=12;
                if (curmbfff || !curmbtf || xfff)
                {
                    if (!(curmbfff && curmbtf && !xfff && is_deblock_calls))
                        MB_N+= 1;
                }
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }
        Block->mb_num = MB_N;
        return;
    }

    Block->block_num-=4;
    Block->mb_num = cur_mb.uMB;
    return;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBChromaMBAFF)(
    void* state,
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    bool curmbfff = !pGetMBFieldDecodingFlag(cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = (cur_mb.uMB&1)==0;
    Ipp32s pair_offset = curmbtf? 1:-1;
    Ipp32s MB_X;
    Ipp32s MB_N;
    //chroma
    if (CHROMA_BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        if (curmbfff && !curmbtf)
        {
            MB_N = cur_mb.uMB + pair_offset;
            Block->block_num+=2;
        }
        else
        {
            MB_X = cur_mb.MacroblockNeighbours.mb_B;
            if (MB_X>=0)
            {
                Ipp8u xfff=!GetMBFieldDecodingFlag(core_enc->m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
                if (!curmbfff && curmbtf && !xfff)
                {
                    MB_N=MB_X;
                    Block->block_num+=2;
                }
                else
                {
                    //if (!curmbff && curmbtf && xfff)
                    //    Block->block_num+=0;
                    //else
                    Block->block_num+=2;
                    MB_N=MB_X + 1;
                }
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }
        Block->mb_num = MB_N;
        return;
    }

    Block->block_num-=2;
    Block->mb_num = cur_mb.uMB;
    return;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLeftLocationForCurrentMBLumaNonMBAFF)(
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    //luma
    if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num) && BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num+=15;
        Block->mb_num = cur_mb.MacroblockNeighbours.mb_D;
    }
    else if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num))
    {
        Block->block_num--;
        Block->mb_num = cur_mb.MacroblockNeighbours.mb_A;
    }
    else if (BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num+=11;
        Block->mb_num = cur_mb.MacroblockNeighbours.mb_B;
    }
    else
    {
        Block->block_num-=5;
        Block->mb_num = cur_mb.uMB;
    }
    return;
}
void H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLeftLocationForCurrentMBLumaMBAFF)(
    void* state,
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    bool curmbfff = !pGetMBFieldDecodingFlag(cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = (cur_mb.uMB&1)==0;
    Ipp32s MB_X;
    Ipp32s MB_N;
    //luma
    if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num) && BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        if (curmbfff && !curmbtf)
        {
            MB_X = cur_mb.MacroblockNeighbours.mb_A;
            if (MB_X<0)
            {
                Block->mb_num = -1;
                return;
            }
            if (!GetMBFieldDecodingFlag(core_enc->m_pCurrentFrame->m_mbinfo.mbs[MB_X]))
            {
                MB_N = MB_X;
                Block->block_num+=15;
            }
            else
            {
                MB_N = MB_X + 1;
                Block->block_num+=7;
            }
        }
        else
        {
            MB_X = cur_mb.MacroblockNeighbours.mb_D;
            if (MB_X>=0)
            {
                if (curmbfff==curmbtf)
                {
                    MB_N=MB_X + 1;
                }
                else
                {
                    if (!GetMBFieldDecodingFlag(core_enc->m_pCurrentFrame->m_mbinfo.mbs[MB_X]))
                    {
                        MB_N=MB_X + 1;
                    }
                    else
                    {
                        MB_N=MB_X;
                    }
                }
                Block->block_num+=15;
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }

        Block->mb_num = MB_N;
        return;
    }
    else if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num))
    {
        //Block->block_num-=4;
        H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaMBAFF)(state, cur_mb, Block,1);
        return;
    }
    else if (BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num--;
        H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBLumaMBAFF)(state, cur_mb, Block,0);
        return;
    }

    Block->block_num-=5;
    Block->mb_num = cur_mb.uMB;
    return;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_GetTopRightLocationForCurrentMBLumaNonMBAFF)(
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    //luma
    if (Block->block_num==3)
    {
        Block->block_num+=9;
        Block->mb_num = cur_mb.MacroblockNeighbours.mb_C;
    }
    else if (BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num+=13;
        Block->mb_num = cur_mb.MacroblockNeighbours.mb_B;
    }
    else if (!UMC_H264_ENCODER::above_right_avail_4x4[block_subblock_mapping[Block->block_num]])
    {
        Block->mb_num = -1;
    }
    else
    {
        Block->block_num-=3;
        Block->mb_num = cur_mb.uMB;
    }
    return;
}
void H264ENC_MAKE_NAME(H264CoreEncoder_GetTopRightLocationForCurrentMBLumaMBAFF)(
    void* state,
    H264CurrentMacroblockDescriptorType& cur_mb,
    H264BlockLocation *Block)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    bool curmbfff = !pGetMBFieldDecodingFlag(cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = (cur_mb.uMB&1)==0;
    Ipp32s MB_X;
    Ipp32s MB_N;
    //luma
    if (Block->block_num==3)
    {
        if (curmbfff && !curmbtf)
        {
            Block->mb_num = -1;
            return;
        }
        else
        {
            MB_X = cur_mb.MacroblockNeighbours.mb_C;
            if (MB_X>=0)
            {
                if (curmbfff==curmbtf)
                {
                    MB_N=MB_X + 1;
                }
                else
                {
                    if (!GetMBFieldDecodingFlag(core_enc->m_pCurrentFrame->m_mbinfo.mbs[MB_X]))
                    {
                        MB_N=MB_X + 1;
                    }
                    else
                    {
                        MB_N=MB_X;
                    }
                }
                Block->block_num+=9;
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }
        Block->mb_num = MB_N;
        return;
    }
    else if (BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num++;
        H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBLumaMBAFF)(state, cur_mb, Block,0);
        return;
    }
    else if (!UMC_H264_ENCODER::above_right_avail_4x4_lin[Block->block_num])
    {
        Block->mb_num = -1;
        return;
    }
    else
    {
        Block->block_num-=3;
        Block->mb_num = cur_mb.uMB;
    }
    return;
}

void H264ENC_MAKE_NAME(H264CoreEncoder_UpdateCurrentMBInfo)(
    void* state,
    H264SliceType *curr_slice)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    H264CurrentMacroblockDescriptorType &cur_mb = curr_slice->m_cur_mb;
    H264MBAddr curMBAddr = cur_mb.uMB;

    cur_mb.GlobalMacroblockInfo = &core_enc->m_pCurrentFrame->m_mbinfo.mbs[curMBAddr];
    cur_mb.LocalMacroblockInfo = &core_enc->m_mbinfo.mbs[curMBAddr];
    cur_mb.MacroblockCoeffsInfo = &core_enc->m_mbinfo.MacroblockCoeffsInfo[curMBAddr];
    cur_mb.MVs[LIST_0] = &core_enc->m_pCurrentFrame->m_mbinfo.MV[LIST_0][curMBAddr];
    cur_mb.MVs[LIST_1] = &core_enc->m_pCurrentFrame->m_mbinfo.MV[LIST_1][curMBAddr];
    cur_mb.MVs[2] = &core_enc->m_mbinfo.MVDeltas[0][curMBAddr];
    cur_mb.MVs[3] = &core_enc->m_mbinfo.MVDeltas[1][curMBAddr];
    cur_mb.RefIdxs[LIST_0] = &core_enc->m_pCurrentFrame->m_mbinfo.RefIdxs[LIST_0][curMBAddr];
    cur_mb.RefIdxs[LIST_1] = &core_enc->m_pCurrentFrame->m_mbinfo.RefIdxs[LIST_1][curMBAddr];
    cur_mb.intra_types = core_enc->m_mbinfo.intra_types[curMBAddr].intra_types;
    if ((curMBAddr & 1) == 0) {
        if (core_enc->m_SliceHeader.MbaffFrameFlag)
            cur_mb.uMBpair = curMBAddr + 1;
        else
            cur_mb.uMBpair = curMBAddr;
    } else {
        cur_mb.uMBpair = curMBAddr - 1;
    }
    cur_mb.GlobalMacroblockPairInfo = &core_enc->m_pCurrentFrame->m_mbinfo.mbs[cur_mb.uMBpair];
    cur_mb.LocalMacroblockPairInfo = &core_enc->m_mbinfo.mbs[cur_mb.uMBpair];

    Ipp32s curMB_X = cur_mb.uMBx;
    Ipp32s curMB_Y = cur_mb.uMBy;
    if ((curMBAddr & 1) == 0 || !core_enc->m_is_cur_pic_afrm) { //update only if top mb received
        Ipp32s mb_top_offset = core_enc->m_WidthInMBs*(core_enc->m_SliceHeader.MbaffFrameFlag + 1);
        Ipp32s mb_left_offset = core_enc->m_SliceHeader.MbaffFrameFlag + 1;
        cur_mb.MacroblockNeighbours.mb_A = curMB_X > 0 ? curMBAddr-mb_left_offset : -1;
        cur_mb.MacroblockNeighbours.mb_B = curMB_Y > 0 ? curMBAddr-mb_top_offset : -1;
        cur_mb.MacroblockNeighbours.mb_C = curMB_Y > 0 && curMB_X < core_enc->m_WidthInMBs-1 ? curMBAddr-mb_top_offset+mb_left_offset : -1;
        cur_mb.MacroblockNeighbours.mb_D = curMB_Y > 0 && curMB_X > 0 ? curMBAddr-mb_top_offset-mb_left_offset : -1;
        if (core_enc->m_NeedToCheckMBSliceEdges){
            if (cur_mb.MacroblockNeighbours.mb_A >= 0 && (cur_mb.GlobalMacroblockInfo->slice_id != core_enc->m_pCurrentFrame->m_mbinfo.mbs[cur_mb.MacroblockNeighbours.mb_A].slice_id))
                cur_mb.MacroblockNeighbours.mb_A = -1;
            if (cur_mb.MacroblockNeighbours.mb_B >= 0 && (cur_mb.GlobalMacroblockInfo->slice_id != core_enc->m_pCurrentFrame->m_mbinfo.mbs[cur_mb.MacroblockNeighbours.mb_B].slice_id))
                cur_mb.MacroblockNeighbours.mb_B = -1;
            if (cur_mb.MacroblockNeighbours.mb_C >= 0 && (cur_mb.GlobalMacroblockInfo->slice_id != core_enc->m_pCurrentFrame->m_mbinfo.mbs[cur_mb.MacroblockNeighbours.mb_C].slice_id))
                cur_mb.MacroblockNeighbours.mb_C = -1;
            if (cur_mb.MacroblockNeighbours.mb_D >= 0 && (cur_mb.GlobalMacroblockInfo->slice_id != core_enc->m_pCurrentFrame->m_mbinfo.mbs[cur_mb.MacroblockNeighbours.mb_D].slice_id))
                cur_mb.MacroblockNeighbours.mb_D = -1;
        }
    }
    cur_mb.BlockNeighbours.mb_above.block_num = 0;
    cur_mb.BlockNeighbours.mb_above_chroma[0].block_num = 16;
    cur_mb.BlockNeighbours.mb_above_left.block_num = 0;
    cur_mb.BlockNeighbours.mb_above_right.block_num = 3;
    cur_mb.BlockNeighbours.mbs_left[0].block_num = 0;
    cur_mb.BlockNeighbours.mbs_left[1].block_num = 4;
    cur_mb.BlockNeighbours.mbs_left[2].block_num = 8;
    cur_mb.BlockNeighbours.mbs_left[3].block_num = 12;
    if (core_enc->m_PicParamSet.chroma_format_idc != 0) {
        cur_mb.BlockNeighbours.mbs_left_chroma[0][0].block_num = 16;
        cur_mb.BlockNeighbours.mbs_left_chroma[0][1].block_num = 18;
    }
    if (core_enc->m_SliceHeader.MbaffFrameFlag) {
        H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mbs_left[0], 0);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mbs_left[1], 0);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mbs_left[2], 0);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mbs_left[3], 0);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBLumaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mb_above, false);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetTopRightLocationForCurrentMBLumaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mb_above_right);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLeftLocationForCurrentMBLumaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mb_above_left);
        if(core_enc->m_PicParamSet.chroma_format_idc != 0) {
            H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBChromaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mb_above_chroma[0]);
            H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBChromaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mbs_left_chroma[0][0]);
            H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBChromaMBAFF)(state, cur_mb, &cur_mb.BlockNeighbours.mbs_left_chroma[0][1]);
        }
    } else  {
        H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mbs_left[0]);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mbs_left[1]);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mbs_left[2]);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBLumaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mbs_left[3]);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBLumaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mb_above);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetTopRightLocationForCurrentMBLumaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mb_above_right);
        H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLeftLocationForCurrentMBLumaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mb_above_left);
        if (core_enc->m_PicParamSet.chroma_format_idc != 0) {
            H264ENC_MAKE_NAME(H264CoreEncoder_GetTopLocationForCurrentMBChromaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mb_above_chroma[0]);
            H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBChromaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mbs_left_chroma[0][0]);
            H264ENC_MAKE_NAME(H264CoreEncoder_GetLeftLocationForCurrentMBChromaNonMBAFF)(cur_mb, &cur_mb.BlockNeighbours.mbs_left_chroma[0][1]);
        }
    }
    if (core_enc->m_PicParamSet.chroma_format_idc != 0) {
        cur_mb.BlockNeighbours.mbs_left_chroma[1][0]=cur_mb.BlockNeighbours.mbs_left_chroma[0][0];
        cur_mb.BlockNeighbours.mbs_left_chroma[1][1]=cur_mb.BlockNeighbours.mbs_left_chroma[0][1];
        cur_mb.BlockNeighbours.mb_above_chroma[1] = cur_mb.BlockNeighbours.mb_above_chroma[0];
        cur_mb.BlockNeighbours.mbs_left_chroma[1][0].block_num += 4;
        cur_mb.BlockNeighbours.mbs_left_chroma[1][1].block_num += 4;
        cur_mb.BlockNeighbours.mb_above_chroma[1].block_num += 4;
    }
    cur_mb.cabac_data = curr_slice->Block_CABAC;
}


Ipp32s H264ENC_MAKE_NAME(H264CoreEncoder_GetColocatedLocation)(
    void* state,
    H264SliceType* curr_slice,
    H264EncoderFrameType* pRefFrame,
    Ipp8u Field,
    Ipp8s& block,
    Ipp8s* scale)
{
    H264CoreEncoderType* core_enc = (H264CoreEncoderType *)state;
    Ipp32u cur_pic_struct = core_enc->m_pCurrentFrame->m_PictureStructureForDec;
    Ipp32u ref_pic_struct = pRefFrame->m_PictureStructureForDec;
    Ipp8s xCol = block&3;
    Ipp8s yCol = block - xCol;
    H264MBAddr curMBAddr = curr_slice->m_cur_mb.uMB;

    if (cur_pic_struct==FRM_STRUCTURE && ref_pic_struct==FRM_STRUCTURE)
    {
        if(scale) *scale=0;
        return curMBAddr;
    } else if (cur_pic_struct==AFRM_STRUCTURE && ref_pic_struct==AFRM_STRUCTURE)
    {
        Ipp32s preColMBAddr = curMBAddr;
        H264MacroblockGlobalInfo *preColMB = &pRefFrame->m_mbinfo.mbs[preColMBAddr];
        Ipp32s cur_mbfdf = pGetMBFieldDecodingFlag(curr_slice->m_cur_mb.GlobalMacroblockInfo);
        Ipp32s ref_mbfdf = pGetMBFieldDecodingFlag(preColMB);

        if (cur_mbfdf==ref_mbfdf)
        {
            if(scale) *scale=0;
            return curMBAddr;
        }
        else if (cur_mbfdf>ref_mbfdf) //current - field reference - frame
        {
            preColMBAddr &= -2; //get top
            if (yCol>=8)
                preColMBAddr++;//get pair
            yCol *= 2;
            yCol &= 15;
            if(scale)
                *scale=1;
        } else {
            if (preColMBAddr&1)
            {
                Ipp32s curPOC = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(core_enc->m_pCurrentFrame, 0, 3);
                Ipp32s topPOC = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefFrame, 0, 1);
                Ipp32s bottomPOC = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefFrame, 1, 1);

                preColMBAddr--;//get pair
                if (abs(curPOC-topPOC)>=abs(curPOC-bottomPOC))
                    preColMBAddr++;//get pair again
            } else {
                if (H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefFrame, 0, 1) >=
                    H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefFrame, 1, 1))
                    preColMBAddr++;//get pair
            }
            yCol= (Ipp8s)((curMBAddr&1)*8+4*(yCol/8));
            if (scale)
                *scale=-1;
        }
        block=yCol+xCol;
        return preColMBAddr;
    } else if (cur_pic_struct==FLD_STRUCTURE && ref_pic_struct==FLD_STRUCTURE)
    {
        if(scale) *scale=0;
        Ipp32s RefField = H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(pRefFrame, Field);

        if(RefField > core_enc->m_field_index)
        {
            return(curMBAddr + (core_enc->m_pCurrentFrame->totalMBs>>1));
        }

        if(RefField < core_enc->m_field_index)
        {
            return(curMBAddr - (core_enc->m_pCurrentFrame->totalMBs>>1));
        } else {
            return(curMBAddr);
        }
    }
    else if (cur_pic_struct==FLD_STRUCTURE && ref_pic_struct==FRM_STRUCTURE)
    {
        Ipp32u PicWidthInMbs = core_enc->m_pCurrentFrame->m_macroBlockSize.width;
        Ipp32u CurrMbAddr = core_enc->m_field_index?(curMBAddr-(core_enc->m_pCurrentFrame->totalMBs>>1)):curMBAddr;
        if(scale) *scale=1;
        yCol=((2*yCol)&15);
        block=yCol+xCol;
        return 2*PicWidthInMbs*(CurrMbAddr/PicWidthInMbs)+(CurrMbAddr%PicWidthInMbs)+PicWidthInMbs*(yCol/8);

    }
    else if (cur_pic_struct==FRM_STRUCTURE && ref_pic_struct==FLD_STRUCTURE)
    {
        if(scale) *scale=-1;
        //Ipp32s RefFieldBottom = -pRefFrame->GetNumberByParity(Field);
        Ipp32u PicWidthInMbs = core_enc->m_pCurrentFrame->m_macroBlockSize.width;
        Ipp32u CurrMbAddr = curMBAddr;
        yCol = Ipp8s(8*((CurrMbAddr/PicWidthInMbs)&1) + 4 * (yCol/8));
        block=yCol+xCol;
        return (PicWidthInMbs*(CurrMbAddr/(2*PicWidthInMbs))+(CurrMbAddr%PicWidthInMbs))/*+
            (pRefFrame->totalMBs&RefFieldBottom)*/;
    }
    else if (cur_pic_struct==FLD_STRUCTURE && ref_pic_struct==AFRM_STRUCTURE)
    {
        Ipp32u CurrMbAddr = curMBAddr;
        if (core_enc->m_field_index) CurrMbAddr-= (core_enc->m_pCurrentFrame->totalMBs>>1);
        Ipp8u bottom_field_flag = core_enc->m_pCurrentFrame->m_bottom_field_flag[core_enc->m_field_index];
        Ipp32s preColMBAddr = CurrMbAddr;
        H264MacroblockGlobalInfo *preColMB = &pRefFrame->m_mbinfo.mbs[preColMBAddr];
        Ipp8u col_mbfdf = pGetMBFieldDecodingFlag(preColMB);

        if (!col_mbfdf)
        {
            if (yCol>8)
                preColMBAddr+=1;
            yCol=((2*yCol)&15);
            if(scale) *scale=1;
        } else {
            if (bottom_field_flag)
                preColMBAddr+=1;
            if(scale) *scale=0;
        }
        block=yCol+xCol;
        return preColMBAddr;
    }
    else if (cur_pic_struct==AFRM_STRUCTURE && ref_pic_struct==FLD_STRUCTURE)
    {
        Ipp32u CurrMbAddr = curMBAddr;
        Ipp32s preColMBAddr=CurrMbAddr;


        Ipp8u cur_mbfdf = pGetMBFieldDecodingFlag(curr_slice->m_cur_mb.GlobalMacroblockInfo);
        Ipp8u cur_mbbf = (curMBAddr&1)==1;
        Ipp32s curPOC = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(core_enc->m_pCurrentFrame, 0, 3);
        Ipp32s topPOC = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefFrame, 0, 1);
        Ipp32s bottomPOC = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pRefFrame, 1, 1);

        Ipp32s bottom_field_flag = cur_mbfdf ?
            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                pRefFrame,
                cur_mbbf) :
            H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
                pRefFrame,
                abs(curPOC - topPOC) >= abs(curPOC - bottomPOC));
        if (cur_mbbf)
                preColMBAddr-=1;
        preColMBAddr = preColMBAddr/2;

        if (!cur_mbfdf)
        {
            yCol=8*cur_mbbf+4*(yCol/8);
            if(scale) *scale=-1;
        } else {
            if(scale) *scale=0;
        }
        block=yCol+xCol;
        VM_ASSERT(preColMBAddr+(bottom_field_flag)*(pRefFrame->totalMBs < (core_enc->m_pCurrentFrame->totalMBs>>1) ));
        return preColMBAddr+(bottom_field_flag)*(pRefFrame->totalMBs>>1);
    }
    VM_ASSERT(0);
    return -1;
}

#undef EncoderRefPicListStructType
#undef EncoderRefPicListType
#undef H264EncoderFrameType
#undef H264CurrentMacroblockDescriptorType
#undef H264SliceType
#undef H264CoreEncoderType
#undef H264ENC_MAKE_NAME
#undef COEFFSTYPE
#undef PIXTYPE
