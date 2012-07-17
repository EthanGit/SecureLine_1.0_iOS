//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2004 - 2011 Intel Corporation. All Rights Reserved.

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

#define H264EncoderFrameType H264ENC_MAKE_NAME(H264EncoderFrame)
#define H264EncoderFrameListType H264ENC_MAKE_NAME(H264EncoderFrameList)
#define EncoderRefPicListType H264ENC_MAKE_NAME(EncoderRefPicList)


Status H264ENC_MAKE_NAME(H264EncoderFrame_Create)(
	Ipp32u  m_MaxSliceSize,
    H264EncoderFrameType* state,
    VideoData* in,
    MemoryAllocator *pMemAlloc
#if defined (ALPHA_BLENDING_H264)
    , Ipp32s alpha
#endif
    , Ipp32s downScale/* = 0*/)
{
    VideoData_Create(&state->m_data);

#ifdef SLICE_CHECK_LIMIT
#ifdef AMD_FIX_SLICE_CHECK_LIMIT
	state->m_SliceCheckMaxSize = m_MaxSliceSize;
#endif
#endif

    state->m_RefPic = false;
    state->m_wasEncoded = false;
    state->m_bIsIDRPic = false;
    state->frameData = 0;
    state->m_pRefPicList = 0;
    state->m_FrameNum = 0;
    state->m_NumSlices = 1;
    state->m_pPreviousFrame = NULL;
    state->m_pFutureFrame = NULL;
    state->pts_start = -1;
    state->pts_end = -1;
#if defined (ALPHA_BLENDING_H264)
    state->m_isAuxiliary = 0;
    state->alpha_plane = -1;
#endif
    state->memAlloc = pMemAlloc;
    state->frameDataID = 0;
    state->mbsDataID = 0;
    state->refListDataID = 0;
    state->m_paddedParsedFrameDataSize.height = 0;
    state->m_paddedParsedFrameDataSize.width = 0;

    state->m_pitchBytes = 0; //TODO call base class constructor
    state->m_pitchPixels = 0; //TODO call base class constructor
    state->mbsData = NULL;
    state->m_isShortTermRef[0] = false;
    state->m_isShortTermRef[1] = false;
    state->m_isLongTermRef[0] = false;
    state->m_isLongTermRef[1] = false;
    state->m_FrameNumWrap = -1;
    state->m_FrameNum  = -1;
    state->m_LongTermFrameIdx = -1;
    state->m_RefPicListResetCount[0] = 0;
    state->m_RefPicListResetCount[1] = 0;
    state->m_PicNum[0] = -1;
    state->m_PicNum[1] = -1;
    state->m_LongTermPicNum[0] = -1;
    state->m_PicNum[1] = -1;
    state->m_PicOrderCounterAccumulated = 0;
    state->m_PicOrderCnt[0] = 0;
    state->m_PicOrderCnt[1] = 0;
    state->m_macroBlockSize.height = 0;
    state->m_macroBlockSize.width = 0;
    state->m_pYPlane = NULL;
    state->m_pUPlane = NULL;
    state->m_pVPlane = NULL;

    //Init VideoData parameters
    ColorFormat cf = in->GetColorFormat();
    ColorFormat out_cf = YUV420;
    //Use color conversion for other formats
    switch( cf ){
        case GRAY:
        case YUV420:
        case YUV422:
        case NV12:
            out_cf = cf;
            break;
#if defined (ALPHA_BLENDING_H264)
        case GRAYA:
            if(!alpha) out_cf = GRAY;
            else out_cf = cf;
            state->alpha_plane = 1;
            break;
        case YUV420A:
            if(!alpha) out_cf = YUV420;
            else out_cf = cf;
            state->alpha_plane = 3;
            break;
        case YUV422A:
            if(!alpha) out_cf = YUV422;
            else out_cf = cf;
            state->alpha_plane = 3;
            break;
#endif
    }

    if (downScale)
    {
        state->uWidth = in->GetWidth() >> 1;
        state->uHeight = in->GetHeight() >> 1;
    }
    else
    {
        state->uWidth = in->GetWidth();
        state->uHeight = in->GetHeight();
    }

    VideoData_Init(&state->m_data, state->uWidth, state->uHeight, out_cf, 0);
    //Set bitdepth to maximum bitdepth
    Ipp32s max_bit_depth = VideoData_GetPlaneBitDepth(in, 0);
    Ipp32s i;
    for (i = 1; i < VideoData_GetNumPlanes(&state->m_data); i++)
        if (max_bit_depth < VideoData_GetPlaneBitDepth(in, i))
            max_bit_depth = VideoData_GetPlaneBitDepth(in, i);

    for (i = 0; i < VideoData_GetNumPlanes(&state->m_data); i++)
        VideoData_SetPlaneBitDepth(&state->m_data, max_bit_depth, i);

    return UMC_OK;
}

void H264ENC_MAKE_NAME(H264EncoderFrame_deallocateParsedFrameData)(
    H264EncoderFrameType* state)
{
    if (state->refListDataID){
        state->memAlloc->Unlock( state->refListDataID );
        state->memAlloc->Free( state->refListDataID );
        state->refListDataID = 0;
        state->refListData = 0;
        state->m_pRefPicList = 0;
    }

    if (state->mbsDataID){
        state->memAlloc->Unlock( state->mbsDataID );
        state->memAlloc->Free( state->mbsDataID );
        state->mbsDataID = 0;
        state->mbsData = NULL;
    }

    state->m_paddedParsedFrameDataSize.width = state->m_paddedParsedFrameDataSize.height = 0;
}

Status H264ENC_MAKE_NAME(H264EncoderFrame_allocateParsedFrameData)(
    H264EncoderFrameType* state,
    const IppiSize& size,
    Ipp32s num_slices)
{
    Status      ps = UMC_OK;
    IppiSize desiredPaddedSize;

    state->m_NumSlices = num_slices;

    desiredPaddedSize.width  = (size.width  + 15) & ~15;
    desiredPaddedSize.height = (size.height + 15) & ~15;

    // If our buffer and internal pointers are already set up for this
    // image size, then there's nothing more to do.
    if (state->m_paddedParsedFrameDataSize.height != desiredPaddedSize.height && state->m_paddedParsedFrameDataSize.width != desiredPaddedSize.width)
    {
        Ipp32u uRefPicListSize = num_slices * sizeof(EncoderRefPicListType);

        H264ENC_MAKE_NAME(H264EncoderFrame_deallocateParsedFrameData)(state);
        if (UMC_OK != state->memAlloc->Alloc(&state->refListDataID, uRefPicListSize + DATA_ALIGN, UMC_ALLOC_PERSISTENT))
            return UMC_ERR_ALLOC;
        state->refListData = (Ipp8u*)state->memAlloc->Lock(state->refListDataID);
        ippsZero_8u(state->refListData, uRefPicListSize);
        state->m_pRefPicList = (EncoderRefPicListType *)(Ipp8u*)align_pointer<Ipp8u*>(state->refListData, DATA_ALIGN);
        state->m_paddedParsedFrameDataSize = desiredPaddedSize;

        // allocate new MB structure(s)
        Ipp32s nMBCount = (desiredPaddedSize.width>>4) * (desiredPaddedSize.height>>4);

        // allocate buffer
        Ipp32u len = (sizeof(H264MacroblockMVs) + sizeof(H264MacroblockMVs) + sizeof(H264MacroblockRefIdxs) + sizeof(H264MacroblockRefIdxs) + sizeof(H264MacroblockGlobalInfo)) * nMBCount + ALIGN_VALUE * 5;
#if defined (ALPHA_BLENDING_H264)
        if( state->alpha_plane >= 0 ) len *= 2;
#endif
        // allocate buffer
        if (UMC_OK != state->memAlloc->Alloc(&state->mbsDataID, len, UMC_ALLOC_PERSISTENT))
                return UMC_ERR_ALLOC;
        state->mbsData = (Ipp8u *) state->memAlloc->Lock(state->mbsDataID);
        ippsZero_8u(state->mbsData, len);

        // set pointer(s)
        state->m_mbinfo.MV[0] = align_pointer<H264MacroblockMVs *> (state->mbsData, ALIGN_VALUE);
        state->m_mbinfo.MV[1] = align_pointer<H264MacroblockMVs *> (state->m_mbinfo.MV[0]+ nMBCount, ALIGN_VALUE);
        state->m_mbinfo.RefIdxs[0] = align_pointer<H264MacroblockRefIdxs *> (state->m_mbinfo.MV[1] + nMBCount, ALIGN_VALUE);
        state->m_mbinfo.RefIdxs[1] = align_pointer<H264MacroblockRefIdxs *> (state->m_mbinfo.RefIdxs[0] + nMBCount, ALIGN_VALUE);
        state->m_mbinfo.mbs = align_pointer<H264MacroblockGlobalInfo *> (state->m_mbinfo.RefIdxs[1] + nMBCount, ALIGN_VALUE);

#if defined (ALPHA_BLENDING_H264)
        if( state->alpha_plane >= 0 ){
            state->m_mbinfo_prim = state->m_mbinfo;
            state->m_mbinfo_alpha.MV[0] = align_pointer<H264MacroblockMVs *> (state->m_mbinfo_prim.mbs + nMBCount, ALIGN_VALUE);
            state->m_mbinfo_alpha.MV[1] = align_pointer<H264MacroblockMVs *> (state->m_mbinfo_alpha.MV[0]+ nMBCount, ALIGN_VALUE);
            state->m_mbinfo_alpha.RefIdxs[0] = align_pointer<H264MacroblockRefIdxs *> (state->m_mbinfo_alpha.MV[1] + nMBCount, ALIGN_VALUE);
            state->m_mbinfo_alpha.RefIdxs[1] = align_pointer<H264MacroblockRefIdxs *> (state->m_mbinfo_alpha.RefIdxs[0] + nMBCount, ALIGN_VALUE);
            state->m_mbinfo_alpha.mbs = align_pointer<H264MacroblockGlobalInfo *> (state->m_mbinfo_alpha.RefIdxs[1] + nMBCount, ALIGN_VALUE);
        }
#endif
    }
    return ps;
} // H264EncoderFrame::allocateParsedstate->frameData(const IppiSize &size)

Status H264ENC_MAKE_NAME(H264EncoderFrameList_Create)(
    H264EncoderFrameListType* state)
{
    state->m_pHead = 0;
    state->m_pTail = 0;
    state->m_pCurrent = 0;
    state->memAlloc = 0;
    state->test = 0;
    state->curPpoc = 0;
    return UMC_OK;
}

Status H264ENC_MAKE_NAME(H264EncoderFrameList_Create)(
    H264EncoderFrameListType* state,
    MemoryAllocator *pMemAlloc)
{
    state->m_pHead = 0;
    state->m_pTail = 0;
    state->m_pCurrent = 0;
    state->memAlloc = pMemAlloc;
    state->test = 0;
    state->curPpoc = 0;
    return UMC_OK;
}

void H264ENC_MAKE_NAME(H264EncoderFrame_Destroy)(
    H264EncoderFrameType* state)
{
    H264ENC_MAKE_NAME(H264EncoderFrame_deallocateParsedFrameData)(state);

    if (state->frameDataID)
    {
        state->memAlloc->Unlock(state->frameDataID);
        state->memAlloc->Free(state->frameDataID);
        state->frameData = NULL;
        state->frameDataID = 0;
#ifdef FRAME_TYPE_DETECT_DS
        H264_Free(state->m_pYPlane_DS);
#endif
    }
    VideoData_Destroy(&state->m_data);
}

Status H264ENC_MAKE_NAME(H264EncoderFrame_allocate)(
    H264EncoderFrameType* state,
    const IppiSize& picSize,
    Ipp32s num_slices)
{
    Status ps = UMC_OK;

    // Clear our state, since allocate is called when we are about to decode into this frame buffer.
    state->m_wasEncoded = false;

    // Don't reset m_activeReference or m_lockedForDisplay as these are handled depending on frame type or by the calling application, respectively
    ps = H264ENC_MAKE_NAME(H264EncoderFrame_allocateParsedFrameData)(state, picSize, num_slices);
    if (ps == UMC_OK){
        Ipp32s lumaSize;
        Ipp32s chromaSize = 0;
        Ipp32s pitchInBytes = 0;
        Ipp32s frameSize = 0;
        Ipp32s hPad = 0;

        pitchInBytes = CalcPitchFromWidth(picSize.width, sizeof(PIXTYPE));
        state->m_lumaSize = picSize;
        state->m_pitchBytes = pitchInBytes;
        state->m_pitchPixels = pitchInBytes / sizeof(PIXTYPE);
        ColorFormat cf = VideoData_GetColorFormat(&state->m_data);
        //hPad = (m_PictureStructureForDec == FLD_STRUCTURE) ? LUMA_PADDING * 2 : LUMA_PADDING;
        //f state->m_PictureStructureForDec is not setup at this moment
        hPad = LUMA_PADDING * 2;
        lumaSize = pitchInBytes * (picSize.height + hPad * 2);
        switch (cf) {
#if defined (ALPHA_BLENDING_H264)
            case GRAYA:
                frameSize += lumaSize;
#endif
            case GRAY:
                chromaSize = 0;
                break;
#if defined (ALPHA_BLENDING_H264)
            case YUV420A:
                frameSize += lumaSize;
#endif
            case NV12:
            case YUV420:
                chromaSize = pitchInBytes * ((picSize.height >> 1) + hPad);
                break;
#if defined (ALPHA_BLENDING_H264)
            case YUV422A:
                frameSize += lumaSize;
#endif
            case YUV422:
                chromaSize = lumaSize;
                break;
        }
        if (state->frameDataID) {
            state->memAlloc->Unlock( state->frameDataID );
            state->memAlloc->Free( state->frameDataID );
            state->frameDataID=0;
#ifdef FRAME_TYPE_DETECT_DS
            H264_Free(state->m_pYPlane_DS);
#endif
        }
        frameSize = lumaSize + chromaSize;
#ifdef FRAME_INTERPOLATION
        frameSize += 3 * lumaSize;
        state->m_PlaneSize = lumaSize / sizeof(PIXTYPE);
#endif
        if (UMC_OK != state->memAlloc->Alloc(&state->frameDataID, frameSize + DATA_ALIGN, UMC_ALLOC_PERSISTENT))
            return UMC_ERR_ALLOC;
        state->frameData = (Ipp8u*)state->memAlloc->Lock(state->frameDataID);

        state->m_pYPlane = align_pointer<PIXTYPE*> (state->frameData + hPad * pitchInBytes + LUMA_PADDING, DATA_ALIGN);
        VideoData_SetPlanePointer(&state->m_data, state->m_pYPlane, 0);
        VideoData_SetPlanePitch(&state->m_data, state->m_pitchBytes, 0);
        Ipp8u* curPtr = (Ipp8u*)state->m_pYPlane + lumaSize;
#ifdef FRAME_INTERPOLATION
        curPtr += 3 * lumaSize;
#endif
#if defined (ALPHA_BLENDING_H264)
        if (cf == YUV422A || cf == YUV420A || cf == GRAYA) {
            VideoData_SetPlanePointer(&state->m_data, (PIXTYPE*)curPtr, state->alpha_plane);
            VideoData_SetPlanePitch(&state->m_data, state->m_pitchBytes, state->alpha_plane);
            curPtr += lumaSize;
        }
#endif
        if (cf != GRAY
#ifdef ALPHA_BLENDING_H264
            && cf != GRAYA
#endif
            ) {
            state->m_pUPlane = (PIXTYPE*)curPtr;
            if (cf == YUV420
#ifdef ALPHA_BLENDING_H264
                || cf != YUV420A
#endif
                )
                state->m_pUPlane -= (hPad >> 1) * state->m_pitchPixels;
            if(cf==NV12){
                state->m_pVPlane = state->m_pUPlane + 1;
            }else{
                state->m_pVPlane = state->m_pUPlane + (state->m_pitchPixels >> 1);
            }
            VideoData_SetPlanePointer(&state->m_data, state->m_pUPlane, 1);
            VideoData_SetPlanePitch(&state->m_data, state->m_pitchBytes, 1);
            VideoData_SetPlanePointer(&state->m_data, state->m_pVPlane,2);
            VideoData_SetPlanePitch(&state->m_data, state->m_pitchBytes, 2);
        }
        state->m_macroBlockSize.width  = picSize.width  >> 4;
        state->m_macroBlockSize.height = picSize.height >> 4;
        state->totalMBs = state->m_macroBlockSize.width * state->m_macroBlockSize.height;
#ifdef FRAME_TYPE_DETECT_DS
        state->m_pYPlane_DS = (PIXTYPE*)H264_Malloc((state->m_macroBlockSize.width * 8 + 16) * (state->m_macroBlockSize.height * 8 + 16) * sizeof(PIXTYPE));
        if (state->m_pYPlane_DS == NULL)
            return UMC_ERR_ALLOC;
#endif
    }
    return ps;
}

void H264ENC_MAKE_NAME(H264EncoderFrame_UpdateFrameNumWrap)(
    H264EncoderFrameType* state,
    Ipp32s CurrFrameNum,
    Ipp32s MaxFrameNum,
    Ipp32s CurrPicStruct)
{
    if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(state))
    {
        state->m_FrameNumWrap = state->m_FrameNum;
        if (state->m_FrameNum > CurrFrameNum)
            state->m_FrameNumWrap -= MaxFrameNum;
        if (CurrPicStruct>=FRM_STRUCTURE)
        {
            H264ENC_MAKE_NAME(H264EncoderFrame_setPicNum)(state, state->m_FrameNumWrap, 0);
            state->m_PictureStructureForRef = FRM_STRUCTURE;
        }
        else
        {
            state->m_PictureStructureForRef = FLD_STRUCTURE;
            if (state->m_bottom_field_flag[0])
            {
                //1st - bottom, 2nd - top
                if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 0))
                    state->m_PicNum[0] = (2 * state->m_FrameNumWrap) + (CurrPicStruct == BOTTOM_FLD_STRUCTURE);
                if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 1))
                    state->m_PicNum[1] = (2 * state->m_FrameNumWrap) + (CurrPicStruct == TOP_FLD_STRUCTURE);
            }
            else
            {
                //1st - top , 2nd - bottom
                if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 0))
                    H264ENC_MAKE_NAME(H264EncoderFrame_setPicNum)(
                        state,
                        (2 * state->m_FrameNumWrap) + (CurrPicStruct == TOP_FLD_STRUCTURE),
                        0);

                if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 1))
                    H264ENC_MAKE_NAME(H264EncoderFrame_setPicNum)(
                        state,
                        (2 * state->m_FrameNumWrap) + (CurrPicStruct == BOTTOM_FLD_STRUCTURE),
                        1);
            }
        }
    }

}    // updateFrameNumWrap

//////////////////////////////////////////////////////////////////////////////
// updateLongTermPicNum
//  Updates m_LongTermPicNum for if long term reference, based upon
//  m_LongTermFrameIdx.
//////////////////////////////////////////////////////////////////////////////
void H264ENC_MAKE_NAME(H264EncoderFrame_UpdateLongTermPicNum)(
    H264EncoderFrameType* state,
    Ipp32s CurrPicStruct)
{
    if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(state))
    {
        if (CurrPicStruct>=FRM_STRUCTURE)
        {
            state->m_LongTermPicNum[0] = state->m_LongTermFrameIdx;
            state->m_LongTermPicNum[1] = state->m_LongTermFrameIdx;
        }
        else
        {
            if (state->m_bottom_field_flag[0])
            {
                //1st - bottom, 2nd - top
                state->m_LongTermPicNum[0] = 2 * state->m_LongTermFrameIdx + (CurrPicStruct == BOTTOM_FLD_STRUCTURE);
                state->m_LongTermPicNum[1] = 2 * state->m_LongTermFrameIdx + (CurrPicStruct == TOP_FLD_STRUCTURE);
            }
            else
            {
                //1st - top , 2nd - bottom
                state->m_LongTermPicNum[0] = 2 * state->m_LongTermFrameIdx + (CurrPicStruct == TOP_FLD_STRUCTURE);
                state->m_LongTermPicNum[1] = 2 * state->m_LongTermFrameIdx + (CurrPicStruct == BOTTOM_FLD_STRUCTURE);
            }
        }
    }
}    // updateLongTermPicNum

#if defined (ALPHA_BLENDING_H264)
void H264ENC_MAKE_NAME(H264EncoderFrame_useAux)(
    H264EncoderFrameType* state)
{
    state->m_pYPlane = (PIXTYPE *)VideoData_GetPlanePointer(&state->m_data, state->alpha_plane);
    state->m_mbinfo = state->m_mbinfo_alpha;
    state->m_isAuxiliary = true;
}

void H264ENC_MAKE_NAME(H264EncoderFrame_usePrimary)(
    H264EncoderFrameType* state)
{
    state->m_pYPlane = (PIXTYPE*)VideoData_GetPlanePointer(&state->m_data, 0);
    state->m_mbinfo = state->m_mbinfo_prim;
    state->m_isAuxiliary = false;
}
#endif

void H264ENC_MAKE_NAME(H264EncoderFrameList_Destroy)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType *pCurr = state->m_pHead;

    while (pCurr)
    {
        H264EncoderFrameType *pNext = pCurr->m_pFutureFrame;
        H264ENC_CALL_DELETE(H264EncoderFrame, pCurr);
        pCurr = pNext;
    }
    state->m_pCurrent = state->m_pHead = state->m_pTail = NULL;
}

void H264ENC_MAKE_NAME(H264EncoderFrameList_clearFrameList)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType *pCurr = state->m_pHead;

    while (pCurr){
        H264EncoderFrameType *pNext = pCurr->m_pFutureFrame;
        H264ENC_CALL_DELETE(H264EncoderFrame, pCurr);
        pCurr = pNext;
    }
    state->m_pCurrent = state->m_pHead = state->m_pTail = 0;
}

H264EncoderFrameType* H264ENC_MAKE_NAME(H264EncoderFrameList_detachHead)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType *pHead = state->m_pHead;
    if (pHead)
    {
        state->m_pHead = state->m_pHead->m_pFutureFrame;
        if (state->m_pHead)
            state->m_pHead->m_pPreviousFrame = 0;
        else
        {
            state->m_pTail = 0;
        }
    }
    state->m_pCurrent = state->m_pHead;
    return pHead;
}

void H264ENC_MAKE_NAME(H264EncoderFrameList_RemoveFrame)(
    H264EncoderFrameListType* state,
    H264EncoderFrameType* pFrm)
{
    H264EncoderFrameType *pPrev = pFrm->m_pPreviousFrame;
    H264EncoderFrameType *pFut = pFrm->m_pFutureFrame;
    if (pFrm == state->m_pHead) //must be equal to pPrev==NULL
    {
        VM_ASSERT(pPrev==NULL);
        H264ENC_MAKE_NAME(H264EncoderFrameList_detachHead)(state);
    }
    else
    {
        if (pFut == NULL)
        {
            state->m_pTail = pPrev;
            pPrev->m_pFutureFrame = 0;
        }
        else
        {
            pPrev->m_pFutureFrame = pFut;
            pFut->m_pPreviousFrame = pPrev;
        }
    }
    state->m_pCurrent = state->m_pHead;
}

//////////////////////////////////////////////////////////////////////////////
// append
//   Appends a new decoded frame buffer to the "end" of the linked list
//////////////////////////////////////////////////////////////////////////////
void H264ENC_MAKE_NAME(H264EncoderFrameList_append)(
    H264EncoderFrameListType* state,
    H264EncoderFrameType *pFrame)
{
    // Error check
    if (!pFrame)
    {
        // Sent in a NULL frame
        return;
    }

    // Has a list been constructed - is their a head?
    if (!state->m_pHead)
    {
        // Must be the first frame appended
        // Set the head to the current
        state->m_pHead = pFrame;
        state->m_pHead->m_pPreviousFrame = 0;
        state->m_pCurrent = state->m_pHead;
    }

    if (state->m_pTail)
    {
        // Set the old tail as the previous for the current
        pFrame->m_pPreviousFrame = state->m_pTail;

        // Set the old tail's future to the current
        state->m_pTail->m_pFutureFrame = pFrame;
    }
    else
    {
        // Must be the first frame appended
        // Set the tail to the current
        state->m_pTail = pFrame;
    }

    // The current is now the new tail
    state->m_pTail = pFrame;
    state->m_pTail->m_pFutureFrame = 0;

    //
}

void H264ENC_MAKE_NAME(H264EncoderFrameList_insertAtCurrent)(
    H264EncoderFrameListType* state,
    H264EncoderFrameType *pFrame)
{
    if (state->m_pCurrent)
    {
        H264EncoderFrameType *pFutureFrame = state->m_pCurrent->m_pFutureFrame;

        pFrame->m_pFutureFrame = pFutureFrame;

        if (pFutureFrame)
        {
            pFutureFrame->m_pPreviousFrame = pFrame;
        }
        else
        {
            // Must be at the tail
            state->m_pTail = pFrame;
        }

        pFrame->m_pPreviousFrame = state->m_pCurrent;
        state->m_pCurrent->m_pFutureFrame = pFrame;
    }
    else
    {
        // Must be the first frame
        H264ENC_MAKE_NAME(H264EncoderFrameList_append)(state, pFrame);
    }
}

H264EncoderFrameType *H264ENC_MAKE_NAME(H264EncoderFrameList_findNextDisposable)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType *pTmp;

    if (!state->m_pCurrent)
    {
        // There are no frames in the list, return
        return NULL;
    }

    // Loop through starting with the next frame after the current one
    for (pTmp = state->m_pCurrent->m_pFutureFrame; pTmp; pTmp = pTmp->m_pFutureFrame)
    {
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isDisposable)(pTmp))
        {
            // We got one

            // Update the current
            state->m_pCurrent = pTmp;

            return pTmp;
        }
    }

    // We got all the way to the tail without finding a free frame
    // Start from the head and go until the current
    for (pTmp = state->m_pHead; pTmp && pTmp->m_pPreviousFrame != state->m_pCurrent; pTmp = pTmp->m_pFutureFrame)
    {
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isDisposable)(pTmp))
        {
            // We got one

            // Update the current
            state->m_pCurrent = pTmp;
            return pTmp;
        }
    }

    // We never found one
    return NULL;
}

H264EncoderFrameType *H264ENC_MAKE_NAME(H264EncoderFrameList_findOldestDisposable)(
    H264EncoderFrameListType* state)
{
    if (!state->m_pCurrent)
    {
        // There are no frames in the list, return
        return NULL;
    }

    state->m_pCurrent = 0;

    Ipp32s SmallestPicOrderCnt = 0x7fffffff;
    Ipp32s LargestRefPicListResetCount = 0;

    // We got all the way to the tail without finding a free frame
    // Start from the head and go until the current
    for (H264EncoderFrameType *pTmp = state->m_pHead; pTmp; pTmp = pTmp->m_pFutureFrame)
    {
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isDisposable)(pTmp))
        {
            if (H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pTmp, 0, 3) > LargestRefPicListResetCount)
            {
                state->m_pCurrent = pTmp;
                SmallestPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pTmp, 0,3);
                LargestRefPicListResetCount = H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pTmp, 0, 3);
            }
            else if ((H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pTmp, 0, 3) < SmallestPicOrderCnt) &&
                (H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pTmp, 0, 3) == LargestRefPicListResetCount ))
            {
                // Update the current
                state->m_pCurrent = pTmp;
                SmallestPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pTmp, 0, 3);
            }
        }
    }

    // We never found one
    return state->m_pCurrent;
}

///////////////////////////////////////////////////////////////////////////////
// findOldestDisplayable
// Search through the list for the oldest displayable frame. It must be
// not disposable, not outputted, and have smallest PicOrderCnt.
///////////////////////////////////////////////////////////////////////////////

H264EncoderFrameType* H264ENC_MAKE_NAME(H264EncoderFrameList_findOldestToEncode)(
    H264EncoderFrameListType* state,
    H264EncoderFrameListType* dpb,
    Ipp32u min_L1_refs,
    Ipp32s use_b_as_refs)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    H264EncoderFrameType *pOldest = NULL;
    H264EncoderFrameType *pLastBref = NULL;
    H264EncoderFrameType *pFirstBref = NULL;
    Ipp32s  SmallestPicOrderCnt = 0x7fffffff;    // very large positive
    Ipp32s  MaxBrefPOC = -1;
    Ipp32s  MinBrefPOC = 0x7fffffff;    // very large positive
    Ipp32s  LargestRefPicListResetCount = 0;
    bool exclude_cur=false;

    while (pCurr)
    {
        if (!pCurr->m_wasEncoded)
        {
            if (pCurr->m_PicCodType==BPREDPIC)
            {
                Ipp32u active_L1_refs;
                H264ENC_MAKE_NAME(H264EncoderFrameList_countL1Refs)(
                    dpb,
                    active_L1_refs,
                    H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 0));

                exclude_cur = active_L1_refs < min_L1_refs;
                //Get B with maximum POC and active_L1_refs<min_L1_refs when B used as refs
                if (use_b_as_refs &&
                    pCurr->m_RefPic &&
                    H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3) < state->curPpoc &&
                    H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pCurr, 0, 3) == LargestRefPicListResetCount)
                {
                    if (H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3) > MaxBrefPOC)
                    {
                         pLastBref = pCurr;
                         MaxBrefPOC = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3);
                    }

                    if (H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3) < MinBrefPOC)
                    {
                         pFirstBref = pCurr;
                         MinBrefPOC = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3);
                    }
                }
            }

            if (!exclude_cur)
            {
                // corresponding frame
                if (H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pCurr, 0, 3) > LargestRefPicListResetCount )
                {
                    pOldest = pCurr;
                    SmallestPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3);
                    LargestRefPicListResetCount = H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pCurr, 0, 3);
                    pLastBref = pCurr;
                    pFirstBref = pCurr;
                    MaxBrefPOC = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3);
                }
                else if ((H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3) < SmallestPicOrderCnt) &&
                    (H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pCurr, 0, 3) == LargestRefPicListResetCount))
                {
                    pOldest = pCurr;
                    SmallestPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3);
                }
            }
        }
        pCurr = pCurr->m_pFutureFrame;
        exclude_cur=false;
    }
    // may be OK if NULL

    if( use_b_as_refs && pOldest ){
        if (pOldest->m_PicCodType == PREDPIC)
        {
            state->test = 0;
            state->curPpoc = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pOldest, 0, 3);
        }
        else if(pOldest->m_PicCodType==BPREDPIC && pLastBref != NULL)
        {
            if( state->test & 1 )
                pOldest = pLastBref;
            else
                pOldest = pFirstBref;
            state->test = !state->test; // for next B
        }
    }

    return pOldest;

}    // findOldestDisplayable

///////////////////////////////////////////////////////////////////////////////
// findNewestToEncode
// Search through the list for the newest displayable frame. It must be
// not disposable, not outputted, and have smallest PicOrderCnt.
///////////////////////////////////////////////////////////////////////////////

H264EncoderFrameType* H264ENC_MAKE_NAME(H264EncoderFrameList_findNewestToEncode)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    H264EncoderFrameType *pNewest = NULL;
    Ipp32s  LargestPicOrderCnt = -1;
    Ipp32s  LargestRefPicListResetCount = 0;
    while (pCurr)
    {
        if (!pCurr->m_wasEncoded)
        {
            // corresponding frame
            if (H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pCurr, 0, 3) > LargestRefPicListResetCount)
            {
                pNewest = pCurr;
                LargestPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3);
                LargestRefPicListResetCount = H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pCurr, 0, 3);
            }
            else if ((H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3) > LargestPicOrderCnt) &&
                (H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(pCurr, 0, 3) == LargestRefPicListResetCount))
            {
                pNewest = pCurr;
                LargestPicOrderCnt = H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3);
            }
        }
        pCurr = pCurr->m_pFutureFrame;
    }
    // may be OK if NULL
    return pNewest;

}    // findNewestToEncode

///////////////////////////////////////////////////////////////////////////////
// countNumDisplayable
//  Return number of displayable frames.
///////////////////////////////////////////////////////////////////////////////

Ipp32s H264ENC_MAKE_NAME(H264EncoderFrameList_countNumToEncode)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    Ipp32u NumToEncode= 0;

    while (pCurr)
    {
        if (pCurr->m_wasEncoded)
            NumToEncode++;
        pCurr = pCurr->m_pFutureFrame;
    }
    return NumToEncode;
}    // countNumDisplayable

///////////////////////////////////////////////////////////////////////////////
// countActiveRefs
//  Return number of active short and long term reference frames.
///////////////////////////////////////////////////////////////////////////////

void H264ENC_MAKE_NAME(H264EncoderFrameList_countActiveRefs)(
    H264EncoderFrameListType* state,
    Ipp32u &NumShortTerm,
    Ipp32u &NumLongTerm)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    NumShortTerm = 0;
    NumLongTerm = 0;

    while (pCurr)
    {
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pCurr))
            NumShortTerm++;
        else if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pCurr))
            NumLongTerm++;
        pCurr = pCurr->m_pFutureFrame;;
    }

}    // countActiveRefs

void H264ENC_MAKE_NAME(H264EncoderFrameList_countL1Refs)(
    H264EncoderFrameListType* state,
    Ipp32u &NumRefs,
    Ipp32s curPOC)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    NumRefs = 0;

    while (pCurr)
    {
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pCurr) &&
            H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(pCurr, 0, 3) > curPOC)
            NumRefs++;
        pCurr = pCurr->m_pFutureFrame;
    }
}    // countActiveRefs

///////////////////////////////////////////////////////////////////////////////
// removeAllRef
// Marks all frames as not used as reference frames.
///////////////////////////////////////////////////////////////////////////////

void H264ENC_MAKE_NAME(H264EncoderFrameList_removeAllRef)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType *pCurr = state->m_pHead;

    while (pCurr)
    {
        if (pCurr->m_wasEncoded)
        {
            H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 0);
            H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 1);
            H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pCurr, 0);
            H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pCurr, 1);
        }
        pCurr = pCurr->m_pFutureFrame;
    }

}    // removeAllRef

void H264ENC_MAKE_NAME(H264EncoderFrameList_IncreaseRefPicListResetCount)(
    H264EncoderFrameListType* state,
    H264EncoderFrameType *ExcludeFrame)
{
    H264EncoderFrameType *pCurr = state->m_pHead;

    while (pCurr)
    {
        if (pCurr!=ExcludeFrame)
        {
            H264ENC_MAKE_NAME(H264EncoderFrame_IncreaseRefPicListResetCount)(pCurr, 0);
            H264ENC_MAKE_NAME(H264EncoderFrame_IncreaseRefPicListResetCount)(pCurr, 1);
        }
        pCurr = pCurr->m_pFutureFrame;
    }

}    // IncreaseRefPicListResetCount

//////////////////////////////////////////////////////////////////////////////
// freeOldestShortTermRef
// Marks the oldest (having smallest FrameNumWrap) short-term reference frame
// as not used as reference frame.
///////////////////////////////////////////////////////////////////////////////

void H264ENC_MAKE_NAME(H264EncoderFrameList_freeOldestShortTermRef)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    H264EncoderFrameType *pOldest = NULL;
    Ipp32s  SmallestFrameNumWrap = 0x0fffffff;    // very large positive

    while (pCurr)
    {
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pCurr) &&
            (pCurr->m_FrameNumWrap < SmallestFrameNumWrap))
        {
            pOldest = pCurr;
            SmallestFrameNumWrap = pCurr->m_FrameNumWrap;
        }
        pCurr = pCurr->m_pFutureFrame;
    }

    VM_ASSERT(pOldest != NULL);    // Should not have been called if no short-term refs

    if (pOldest)
    {
        H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pOldest, 0);
        H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pOldest, 1);
    }

}    // freeOldestShortTermRef

///////////////////////////////////////////////////////////////////////////////
// freeShortTermRef
// Mark the short-term reference frame with specified PicNum as not used
///////////////////////////////////////////////////////////////////////////////

void H264ENC_MAKE_NAME(H264EncoderFrameList_freeShortTermRef)(
    H264EncoderFrameListType* state,
    Ipp32s PicNum)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    bool found = false;
    while (pCurr)
    {
        if (pCurr->m_PictureStructureForRef>=FRM_STRUCTURE)
        {
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pCurr) &&
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pCurr, 0, 0) == PicNum)
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pCurr, 0);
                break;
            }
        }
        else
        {
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(pCurr, 0) &&
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pCurr, 0, 0) == PicNum)
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pCurr, 0);
                found = true;
            }
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(pCurr, 1) &&
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pCurr, 1, 0) == PicNum)
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pCurr, 1);
                found = true;
            }
            if (found) break;

        }

        pCurr = pCurr->m_pFutureFrame;
    }
    VM_ASSERT(pCurr != NULL);    // No match found, should not happen.

}    // freeShortTermRef

///////////////////////////////////////////////////////////////////////////////
// freeLongTermRef
// Mark the long-term reference frame with specified LongTermPicNum as not used
///////////////////////////////////////////////////////////////////////////////

void H264ENC_MAKE_NAME(H264EncoderFrameList_freeLongTermRef)(
    H264EncoderFrameListType* state,
    Ipp32s LongTermPicNum)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    bool found = false;

    while (pCurr)
    {
        if (pCurr->m_PictureStructureForRef>=FRM_STRUCTURE)
        {
            if ((H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pCurr)) &&
                (H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pCurr, 0, 0) == LongTermPicNum))
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 0);
                break;
            }
        }
        else
        {
            if ((H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(pCurr, 0)) &&
                (H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pCurr, 0, 0) == LongTermPicNum))
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 0);
                found = true;
            }
            if ((H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(pCurr, 1)) &&
                (H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(pCurr, 1, 0) == LongTermPicNum))
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 1);
                found = true;
            }
            if (found) break;
        }

        pCurr = pCurr->m_pFutureFrame;
    }
    VM_ASSERT(pCurr != NULL);    // No match found, should not happen.

}    // freeLongTermRef

///////////////////////////////////////////////////////////////////////////////
// freeLongTermRef
// Mark the long-term reference frame with specified LongTermFrameIdx
// as not used
///////////////////////////////////////////////////////////////////////////////

void H264ENC_MAKE_NAME(H264EncoderFrameList_freeLongTermRefIdx)(
    H264EncoderFrameListType* state,
    Ipp32s LongTermFrameIdx)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    bool found = false;

    while (pCurr)
    {
        if (pCurr->m_PictureStructureForRef>=FRM_STRUCTURE)
        {
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(pCurr) &&
                pCurr->m_LongTermFrameIdx == LongTermFrameIdx)
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 0);
                break;
            }
        }
        else
        {
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(pCurr, 0) &&
                pCurr->m_LongTermFrameIdx == LongTermFrameIdx)
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 0);
                found = true;
            }
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(pCurr, 1) &&
                pCurr->m_LongTermFrameIdx == LongTermFrameIdx)
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 1);
                found = true;
            }
            if (found) break;

        }

        pCurr = pCurr->m_pFutureFrame;
    }

    // OK if none found

}    // freeLongTermRefIdx

///////////////////////////////////////////////////////////////////////////////
// freeOldLongTermRef
// Mark any long-term reference frame with LongTermFrameIdx greater
// than MaxLongTermFrameIdx as not used. When MaxLongTermFrameIdx is -1, this
// indicates no long-term frame indices and all long-term reference
// frames should be freed.
///////////////////////////////////////////////////////////////////////////////

void H264ENC_MAKE_NAME(H264EncoderFrameList_freeOldLongTermRef)(
    H264EncoderFrameListType* state,
    Ipp32s MaxLongTermFrameIdx)
{
    H264EncoderFrameType *pCurr = state->m_pHead;

    while (pCurr)
    {
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(pCurr, 0) &&
            pCurr->m_LongTermFrameIdx > MaxLongTermFrameIdx)
        {
            H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 0);
            H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(pCurr, 1);
        }
        pCurr = pCurr->m_pFutureFrame;
    }

}    // freeOldLongTermRef

///////////////////////////////////////////////////////////////////////////////
// changeSTtoLTRef
//    Mark the short-term reference frame with specified PicNum as long-term
//  with specified long term idx.
///////////////////////////////////////////////////////////////////////////////

void H264ENC_MAKE_NAME(H264EncoderFrameList_changeSTtoLTRef)(
    H264EncoderFrameListType* state,
    Ipp32s PicNum,
    Ipp32s LongTermFrameIdx)
{
    H264EncoderFrameType *pCurr = state->m_pHead;
    bool found=false;
    while (pCurr)
    {
        if (pCurr->m_PictureStructureForRef>=FRM_STRUCTURE)
        {
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(pCurr) &&
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pCurr, 0, 0) == PicNum)
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pCurr, 0);
                pCurr->m_LongTermFrameIdx = LongTermFrameIdx;
                H264ENC_MAKE_NAME(H264EncoderFrame_SetisLongTermRef)(pCurr, 0);
                H264ENC_MAKE_NAME(H264EncoderFrame_UpdateLongTermPicNum)(pCurr, 2);
                break;
            }
        }
        else
        {
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(pCurr, 0) &&
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pCurr, 0, 0) == PicNum)
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pCurr, 0);
                pCurr->m_LongTermFrameIdx = LongTermFrameIdx;
                H264ENC_MAKE_NAME(H264EncoderFrame_SetisLongTermRef)(pCurr, 0);
                H264ENC_MAKE_NAME(H264EncoderFrame_UpdateLongTermPicNum)(pCurr, pCurr->m_bottom_field_flag[0]);
                found = true;
            }
            if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(pCurr, 1) &&
                H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(pCurr, 1, 0) == PicNum)
            {
                H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(pCurr, 1);
                pCurr->m_LongTermFrameIdx = LongTermFrameIdx;
                H264ENC_MAKE_NAME(H264EncoderFrame_SetisLongTermRef)(pCurr, 1);
                H264ENC_MAKE_NAME(H264EncoderFrame_UpdateLongTermPicNum)(pCurr, pCurr->m_bottom_field_flag[1]);
                found = true;
            }
            if (found) break;

        }
        pCurr = pCurr->m_pFutureFrame;
    }
    VM_ASSERT(pCurr != NULL);    // No match found, should not happen.

}    // changeSTtoLTRef

H264EncoderFrameType* H264ENC_MAKE_NAME(H264EncoderFrameList_InsertFrame)(
	Ipp32u  m_MaxSliceSize,
    H264EncoderFrameListType* state,
    VideoData* rFrame,
    EnumPicCodType ePictureType,
    Ipp32s isRef,
    Ipp32s num_slices,
    const IppiSize& padded_size
#if defined (ALPHA_BLENDING_H264)
    , Ipp32s alpha
#endif
)
{
    Status ps = UMC_OK;
    H264EncoderFrameType *pFrm;

    pFrm = H264ENC_MAKE_NAME(H264EncoderFrameList_findNextDisposable)(state);

    //if there are no unused frames allocate new frame
    if (!pFrm)
    {
        pFrm = (H264EncoderFrameType *)H264_Malloc(sizeof(H264EncoderFrameType));
        if (!pFrm)
            return NULL;

        H264ENC_MAKE_NAME(H264EncoderFrame_Create)(
			m_MaxSliceSize,
            pFrm,
            rFrame,
            state->memAlloc,
#if defined (ALPHA_BLENDING_H264)
            alpha,
#endif
            0); //Init/allocate from input data

        ps = H264ENC_MAKE_NAME(H264EncoderFrame_allocate)(pFrm, padded_size, num_slices);
        if (ps != UMC_OK)
            return NULL;

        H264ENC_MAKE_NAME(H264EncoderFrameList_insertAtCurrent)(state, pFrm);
    }

    rFrame->GetTime( pFrm->pts_start, pFrm->pts_end );

    //Make copy of input data
    VideoProcessing frame_cnv;
    VideoProcessing_GetFrame(&frame_cnv, rFrame, &pFrm->m_data);

    VideoData_SetFrameType(&pFrm->m_data, VideoData_GetFrameType(rFrame));
    pFrm->m_PicCodType = ePictureType;
    pFrm->m_RefPic = isRef;
    pFrm->m_wasEncoded = false;
    return pFrm;
}


#if defined (ALPHA_BLENDING_H264)
void H264ENC_MAKE_NAME(H264EncoderFrameList_switchToPrimary)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType* frm = state->m_pHead;
    for(; frm; frm = frm->m_pFutureFrame)
        H264ENC_MAKE_NAME(H264EncoderFrame_usePrimary)(frm);
}

void H264ENC_MAKE_NAME(H264EncoderFrameList_switchToAuxiliary)(
    H264EncoderFrameListType* state)
{
    H264EncoderFrameType* frm = state->m_pHead;
    for(; frm ;frm = frm->m_pFutureFrame)
        H264ENC_MAKE_NAME(H264EncoderFrame_useAux)(frm);
}
#endif

void H264ENC_MAKE_NAME(H264EncoderFrame_exchangeFrameYUVPointers)(
    H264EncoderFrameType* frame1,
    H264EncoderFrameType* frame2)
{
    PIXTYPE* tmp1;
    PIXTYPE* tmp2;
    PIXTYPE* tmp3;
    MemID    id_tmp;
    Ipp8u*   tmp;
    Ipp32s   i;

    tmp1 = frame1->m_pYPlane;
    tmp2 = frame1->m_pUPlane;
    tmp3 = frame1->m_pVPlane;
    frame1->m_pYPlane = frame2->m_pYPlane;
    frame1->m_pUPlane = frame2->m_pUPlane;
    frame1->m_pVPlane = frame2->m_pVPlane;
    frame2->m_pYPlane = tmp1;
    frame2->m_pUPlane = tmp2;
    frame2->m_pVPlane = tmp3;

    id_tmp = frame1->frameDataID;
    frame1->frameDataID = frame2->frameDataID;
    frame2->frameDataID = id_tmp;

    tmp = frame1->frameData;
    frame1->frameData = frame2->frameData;
    frame2->frameData = tmp;

    for( i=0; i < VideoData_GetNumPlanes(&frame1->m_data); i++ ){
        void* tmp = VideoData_GetPlanePointer(&frame1->m_data, i);
        VideoData_SetPlanePointer(&frame1->m_data, VideoData_GetPlanePointer(&frame2->m_data, i), i);
        VideoData_SetPlanePointer(&frame2->m_data, tmp, i );
    }

    return;
}


#undef EncoderRefPicListType
#undef H264EncoderFrameType
#undef H264EncoderFrameListType
#undef H264ENC_MAKE_NAME
#undef COEFFSTYPE
#undef PIXTYPE
