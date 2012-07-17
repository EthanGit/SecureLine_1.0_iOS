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

#define H264EncoderFrameType H264ENC_MAKE_NAME(H264EncoderFrame)
#define H264EncoderFrameListType H264ENC_MAKE_NAME(H264EncoderFrameList)
#define EncoderRefPicListStructType H264ENC_MAKE_NAME(EncoderRefPicListStruct)
#define EncoderRefPicListType H264ENC_MAKE_NAME(EncoderRefPicList)

typedef struct H264ENC_MAKE_NAME(sEncoderRefPicList) EncoderRefPicListType;
typedef struct H264ENC_MAKE_NAME(sH264EncoderFrame) H264EncoderFrameType;
typedef struct H264ENC_MAKE_NAME(sH264EncoderFrameList) H264EncoderFrameListType;

typedef struct H264ENC_MAKE_NAME(sH264EncoderFrame)
{
    // These point to the previous and future reference frames
    // used to decode this frame.
    // These are also used to maintain a doubly-linked list of
    // reference frames in the H264EncoderFrameList class.  So, these
    // may be non-NULL even if the current frame is an I frame,
    // for example.  m_pPreviousFrame will always be valid when
    // decoding a P or B frame, and m_pFutureFrame will always
    // be valid when decoding a B frame.

//public:
    UMC::VideoData m_data;

    Ipp64f pts_start;
    Ipp64f pts_end;

    // L0 and L1 refer to RefList 0 and RefList 1 in JVT spec.
    // In this implementation, L0 is Forward MVs in B slices, and all MVs in P slices
    // L1 is Backward MVs in B slices and unused in P slices.
    H264GlobalMacroblocksDescriptor m_mbinfo;
#if defined (ALPHA_BLENDING_H264)
    H264GlobalMacroblocksDescriptor m_mbinfo_alpha;
    H264GlobalMacroblocksDescriptor m_mbinfo_prim;
#endif

    Ipp32u uWidth;
    Ipp32u uHeight;

    H264EncoderFrameType* m_pPreviousFrame;
    H264EncoderFrameType* m_pFutureFrame;
    bool                  m_wasEncoded;

    //Description of picture, NULL if no picture
    PIXTYPE                *m_pYPlane;
    PIXTYPE                *m_pUPlane;
    PIXTYPE                *m_pVPlane;
    //Frame Data
    IppiSize        m_lumaSize;
    Ipp32u          m_pitchPixels;
    Ipp32u          m_pitchBytes;
#ifdef FRAME_INTERPOLATION
    Ipp32s          m_PlaneSize;
#endif
#ifdef FRAME_TYPE_DETECT_DS
    PIXTYPE         *m_pYPlane_DS;
#endif
    Ipp8u            m_PictureStructureForRef;
    Ipp8u            m_PictureStructureForDec;
    EnumPicCodType   m_PicCodType;
    Ipp32s           m_RefPic;
    Ipp32s           totalMBs;

    // For type 1 calculation of m_PicOrderCnt. m_FrameNum is needed to
    // be used as previous frame num.

    Ipp32s           m_PicNum[2];
    Ipp32s           m_LongTermPicNum[2];
    Ipp32s           m_FrameNum;
    Ipp32s           m_FrameNumWrap;
    Ipp32s           m_LongTermFrameIdx;
    Ipp32s           m_RefPicListResetCount[2];
    Ipp32s           m_PicOrderCnt[2];    // Display order picture count mod MAX_PIC_ORDER_CNT.
    Ipp32u           m_PicOrderCounterAccumulated; // Display order picture counter, but with compensation of IDR POC resets.
    Ipp32s           m_crop_left;
    Ipp32s           m_crop_right;
    Ipp32s           m_crop_top;
    Ipp32s           m_crop_bottom;
    Ipp8s            m_crop_flag;
    bool             m_isShortTermRef[2];
    bool             m_isLongTermRef[2];

    //Ipp32u              m_FrameNum;            // Decode order frame label, from slice header
    Ipp8u            m_PQUANT;            // Picture QP (from first slice header)
    Ipp8u            m_PQUANT_S;            // Picture QS (from first slice header)
    IppiSize         m_dimensions;
    Ipp8u            m_bottom_field_flag[2];

    IppiSize m_paddedParsedFrameDataSize;
#if defined (ALPHA_BLENDING_H264)
    Ipp32s alpha_plane; // Is there alpha plane (>=0) and its number
#endif

#ifdef FRAME_QP_FROM_FILE
    int frame_qp;
#endif

    bool   m_bIsIDRPic;
    Ipp32s numMBs;
    Ipp32s numIntraMBs;
    // Read from slice NAL unit of current picture. True indicates the
    // picture contains only I or SI slice types.

    EncoderRefPicListType *m_pRefPicList;

//private:
    MemoryAllocator* memAlloc;
    MemID            frameDataID;
    MemID            mbsDataID;
    MemID            refListDataID;
    Ipp8u           *frameData;
    Ipp8u           *mbsData;
    Ipp8u           *refListData;

    Ipp32s m_NumSlices;   // Number of slices.
    IppiSize m_macroBlockSize;

#if defined (ALPHA_BLENDING_H264)
    Ipp32s m_isAuxiliary;    //Do we compress auxiliary channel?
#endif

#ifdef SLICE_CHECK_LIMIT
#ifdef AMD_FIX_SLICE_CHECK_LIMIT
	Ipp32u  m_SliceCheckMaxSize;
#endif
#endif
} H264EncoderFrameType;

// Struct containing list 0 and list 1 reference picture lists for one slice.
// Length is plus 1 to provide for null termination.
typedef struct H264ENC_MAKE_NAME(sEncoderRefPicListStruct)
{
    H264EncoderFrameType *m_RefPicList[MAX_NUM_REF_FRAMES + 1];
    Ipp8s                 m_Prediction[MAX_NUM_REF_FRAMES + 1];
    Ipp32s                m_POC[MAX_NUM_REF_FRAMES+1];

} EncoderRefPicListStructType;

typedef struct H264ENC_MAKE_NAME(sEncoderRefPicList)
{
    EncoderRefPicListStructType m_RefPicListL0;
    EncoderRefPicListStructType m_RefPicListL1;

} EncoderRefPicListType;

// The above variables are used for management of reference frames
// on reference picture lists maintained in m_RefPicList. They are
// updated as reference picture management information is decoded
// from the bitstream. The picture and frame number variables determine
// reference picture ordering on the lists.
void H264ENC_MAKE_NAME(H264EncoderFrame_exchangeFrameYUVPointers)(
    H264EncoderFrameType* frame1,
    H264EncoderFrameType* frame2);

Status H264ENC_MAKE_NAME(H264EncoderFrame_Create)(
	Ipp32u  m_MaxSliceSize,
	H264EncoderFrameType* state,
    VideoData* in,
    MemoryAllocator *pMemAlloc
#if defined (ALPHA_BLENDING_H264)
    , Ipp32s alpha
#endif
    , Ipp32s downScale/* = 0*/);

void H264ENC_MAKE_NAME(H264EncoderFrame_Destroy)(
    H264EncoderFrameType* state);

Status H264ENC_MAKE_NAME(H264EncoderFrame_allocate)(
    H264EncoderFrameType* state,
    const IppiSize& paddedSize,
    Ipp32s num_slices);

// A decoded frame can be "disposed" if it is not an active reference
// and it is not locked by the calling application and it has been
// output for display.
inline
bool H264ENC_MAKE_NAME(H264EncoderFrame_isDisposable)(
    H264EncoderFrameType* state)
{
    return (!state->m_isShortTermRef[0] &&
            !state->m_isShortTermRef[1] &&
            !state->m_isLongTermRef[0] &&
            !state->m_isLongTermRef[1] &&
             state->m_wasEncoded);
}

inline
bool H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(
    H264EncoderFrameType* state,
    Ipp32s WhichField)
{
    if (state->m_PictureStructureForRef>=FRM_STRUCTURE )
        return state->m_isShortTermRef[0] && state->m_isShortTermRef[1];
    else
        return state->m_isShortTermRef[WhichField];
}

inline
Ipp8u H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef0)(
    H264EncoderFrameType* state)
{
    return state->m_isShortTermRef[0] + state->m_isShortTermRef[1]*2;
}

inline
Ipp32s H264ENC_MAKE_NAME(H264EncoderFrame_PicNum)(
    H264EncoderFrameType* state,
    Ipp32s f,
    Ipp32s force/* = 0*/)
{
    if ((state->m_PictureStructureForRef>=FRM_STRUCTURE && force==0) || force==3)
    {
        return MIN(state->m_PicNum[0],state->m_PicNum[1]);
    }
    else if (force==2)
    {
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 0) &&
            H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 1))
            return MIN(state->m_PicNum[0],state->m_PicNum[1]);
        else if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 0))
            return state->m_PicNum[0];
        else
            return state->m_PicNum[0];
    }

    return state->m_PicNum[f];
}

inline
void H264ENC_MAKE_NAME(H264EncoderFrame_setPicNum)(
    H264EncoderFrameType* state,
    Ipp32s PicNum,
    Ipp8u f)
{
    if (state->m_PictureStructureForRef>=FRM_STRUCTURE)
    {
        state->m_PicNum[0] = state->m_PicNum[1] = PicNum;
    }
    else
        state->m_PicNum[f] = PicNum;
}

// Updates m_FrameNumWrap and m_PicNum if the frame is a short-term
// reference and a frame number wrap has occurred.
void H264ENC_MAKE_NAME(H264EncoderFrame_UpdateFrameNumWrap)(
    H264EncoderFrameType* state,
    Ipp32s CurrFrameNum,
    Ipp32s MaxFrameNum,
    Ipp32s CurrPicStruct);

inline
void H264ENC_MAKE_NAME(H264EncoderFrame_SetisShortTermRef)(
    H264EncoderFrameType* state,
    Ipp32s WhichField)
{
    if (state->m_PictureStructureForRef>=FRM_STRUCTURE)
        state->m_isShortTermRef[0] = state->m_isShortTermRef[1] = true;
    else
        state->m_isShortTermRef[WhichField] = true;
}

inline
Ipp32s H264ENC_MAKE_NAME(H264EncoderFrame_PicOrderCnt)(
    H264EncoderFrameType* state,
    Ipp32s index,
    Ipp32s force/* = 0*/)
{
    if ((state->m_PictureStructureForRef>=FRM_STRUCTURE && force==0) || force==3)
    {
        return MIN(state->m_PicOrderCnt[0], state->m_PicOrderCnt[1]);
    }
    else if (force==2)
    {
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 0) &&
            H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 1))
            return MIN(state->m_PicOrderCnt[0], state->m_PicOrderCnt[1]);
        else if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 0))
            return state->m_PicOrderCnt[0];
        else
            return state->m_PicOrderCnt[1];
    }
    return state->m_PicOrderCnt[index];
}

inline
Ipp32s H264ENC_MAKE_NAME(H264EncoderFrame_DeblockPicID)(
    H264EncoderFrameType* state,
    Ipp32s index)
{
#if 0
    //the constants are subject to change
    return PicOrderCnt(index,force)*2+FrameNumWrap()*534+FrameNum()*878+PicNum(index,force)*14
        +RefPicListResetCount(index,force);
#else
    size_t ret = (size_t)state;
    return (Ipp32s)(ret + index);

#endif
}

inline
bool H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(
    H264EncoderFrameType* state,
    Ipp32s WhichField)
{
    if (state->m_PictureStructureForRef>=FRM_STRUCTURE)
        return state->m_isLongTermRef[0] && state->m_isLongTermRef[1];
    else
        return state->m_isLongTermRef[WhichField];
}

inline
Ipp8u H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef0)(
    H264EncoderFrameType* state)
{
    return state->m_isLongTermRef[0] + state->m_isLongTermRef[1]*2;
}

inline
void H264ENC_MAKE_NAME(H264EncoderFrame_SetisLongTermRef)(
    H264EncoderFrameType* state,
    Ipp32s WhichField)
{
    if (state->m_PictureStructureForRef>=FRM_STRUCTURE)
        state->m_isLongTermRef[0] = state->m_isLongTermRef[1] = true;
    else
        state->m_isLongTermRef[WhichField] = true;
}

inline
void H264ENC_MAKE_NAME(H264EncoderFrame_unSetisShortTermRef)(
    H264EncoderFrameType* state,
    Ipp32s WhichField)
{
    if (state->m_PictureStructureForRef>=FRM_STRUCTURE){
        state->m_isShortTermRef[0] = state->m_isShortTermRef[1] = false;
    }else
        state->m_isShortTermRef[WhichField] = false;
}

inline
void H264ENC_MAKE_NAME(H264EncoderFrame_unSetisLongTermRef)(
    H264EncoderFrameType* state,
    Ipp32s WhichField)
{
    if (state->m_PictureStructureForRef>=FRM_STRUCTURE){
        state->m_isLongTermRef[0] = state->m_isLongTermRef[1] = false;
    }else
        state->m_isLongTermRef[WhichField] = false;
}

inline
Ipp32s H264ENC_MAKE_NAME(H264EncoderFrame_LongTermPicNum)(
    H264EncoderFrameType* state,
    Ipp32s f,
    Ipp32s force/* = 0*/)
{
    if ((state->m_PictureStructureForRef>=FRM_STRUCTURE && force==0) || force==3){
        return MIN(state->m_LongTermPicNum[0], state->m_LongTermPicNum[1]);
    }else if (force==2){
        if (H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(state, 0) &&
            H264ENC_MAKE_NAME(H264EncoderFrame_isLongTermRef1)(state, 1))
            return MIN(state->m_LongTermPicNum[0], state->m_LongTermPicNum[1]);
        else if (H264ENC_MAKE_NAME(H264EncoderFrame_isShortTermRef1)(state, 0))
            return state->m_LongTermPicNum[0];
        else
            return state->m_LongTermPicNum[0];
    }
    return state->m_LongTermPicNum[f];
}

void H264ENC_MAKE_NAME(H264EncoderFrame_UpdateLongTermPicNum)(
    H264EncoderFrameType* state,
    Ipp32s CurrPicStruct);

inline
void H264ENC_MAKE_NAME(H264EncoderFrame_IncreaseRefPicListResetCount)(
    H264EncoderFrameType* state,
    Ipp32s f)
{
    /*if (state->m_PictureStructureForRef>=FRM_STRUCTURE)
    {
    state->m_RefPicListResetCount[0]++;
    state->m_RefPicListResetCount[1]++;
    }
    else*/
    state->m_RefPicListResetCount[f]++;
}

inline
void H264ENC_MAKE_NAME(H264EncoderFrame_InitRefPicListResetCount)(
    H264EncoderFrameType* state,
    Ipp32s f)
{
    if (state->m_PictureStructureForRef >= FRM_STRUCTURE) {
        // Should reconsider keeping this branch.
        state->m_RefPicListResetCount[0] = state->m_RefPicListResetCount[1] = 0;
    }
    else
        state->m_RefPicListResetCount[f] = 0;
}

inline
Ipp32s H264ENC_MAKE_NAME(H264EncoderFrame_RefPicListResetCount)(
    H264EncoderFrameType* state,
    Ipp32s f,
    Ipp32s force/* = 0*/)
{
    if ((state->m_PictureStructureForRef>=FRM_STRUCTURE && force==0)|| force==3)
        return MAX(state->m_RefPicListResetCount[0], state->m_RefPicListResetCount[1]);
    else
        return state->m_RefPicListResetCount[f];
}

inline
Ipp32s H264ENC_MAKE_NAME(H264EncoderFrame_GetNumberByParity)(
    H264EncoderFrameType* state,
    Ipp32s parity)
{
    if (parity == -1)
        return -1;
    if (state->m_bottom_field_flag[0] == parity)
        return 0;
    if (state->m_bottom_field_flag[1] == parity)
        return 1;

    VM_ASSERT(state->m_PictureStructureForRef >= FRM_STRUCTURE);
    return 0;
}

// Returns pointer to start of specified ref pic list.
inline
EncoderRefPicListStructType* H264ENC_MAKE_NAME(H264EncoderFrame_GetRefPicList)(
    H264EncoderFrameType* state,
    Ipp32s SliceNum,
    Ipp32s List)
{
    EncoderRefPicListStructType *pList;
#ifdef SLICE_CHECK_LIMIT
#ifdef AMD_FIX_SLICE_CHECK_LIMIT
	if (state->m_SliceCheckMaxSize)
		SliceNum = 0;
#else
	SliceNum = 0;
#endif
#endif
    if (List == LIST_0)
        pList = &state->m_pRefPicList[SliceNum].m_RefPicListL0;
    else
        pList = &state->m_pRefPicList[SliceNum].m_RefPicListL1;

    return pList;

}

Status H264ENC_MAKE_NAME(H264EncoderFrame_allocateParsedFrameData)(
    H264EncoderFrameType* state,
    const IppiSize&,
    Ipp32s);

void H264ENC_MAKE_NAME(H264EncoderFrame_deallocateParsedFrameData)(
    H264EncoderFrameType* state);

#if defined (ALPHA_BLENDING_H264)

void H264ENC_MAKE_NAME(H264EncoderFrame_useAux)(
    H264EncoderFrameType* state);

void H264ENC_MAKE_NAME(H264EncoderFrame_usePrimary)(
    H264EncoderFrameType* state);

#endif


typedef struct H264ENC_MAKE_NAME(sH264EncoderFrameList)
{
//private:
    // m_pHead points to the first element in the list, and m_pTail
    // points to the last.  m_pHead->previous() and m_pTail->future()
    // are both NULL.
    H264EncoderFrameType *m_pHead;
    H264EncoderFrameType *m_pTail;
    H264EncoderFrameType *m_pCurrent;
    MemoryAllocator* memAlloc;

    unsigned int test;
    Ipp32s curPpoc;
} H264EncoderFrameListType;

Status H264ENC_MAKE_NAME(H264EncoderFrameList_Create)(
    H264EncoderFrameListType* state);

Status H264ENC_MAKE_NAME(H264EncoderFrameList_Create)(
    H264EncoderFrameListType* state,
    MemoryAllocator *pMemAlloc);

void H264ENC_MAKE_NAME(H264EncoderFrameList_Destroy)(
    H264EncoderFrameListType* state);

void H264ENC_MAKE_NAME(H264EncoderFrameList_clearFrameList)(
    H264EncoderFrameListType* state);

// Detach the first frame and return a pointer to it
H264EncoderFrameType* H264ENC_MAKE_NAME(H264EncoderFrameList_detachHead)(
    H264EncoderFrameListType* state);

// Removes selected frame from list
void H264ENC_MAKE_NAME(H264EncoderFrameList_RemoveFrame)(
    H264EncoderFrameListType* state,
    H264EncoderFrameType*);

// Append the given frame to our tail
void H264ENC_MAKE_NAME(H264EncoderFrameList_append)(
    H264EncoderFrameListType* state,
    H264EncoderFrameType *pFrame);

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
);

// Move the given list to the beginning of our list.
void H264ENC_MAKE_NAME(H264EncoderFrameList_insertList)(
    H264EncoderFrameListType* state,
    H264EncoderFrameListType& src);

// Search through the list for the next disposable frame to decode into
H264EncoderFrameType *H264ENC_MAKE_NAME(H264EncoderFrameList_findNextDisposable)(
    H264EncoderFrameListType* state);

// Search through the list for the oldest disposable frame to decode into
H264EncoderFrameType *H264ENC_MAKE_NAME(H264EncoderFrameList_findOldestDisposable)(
    H264EncoderFrameListType* state);

// Inserts a frame immediately after the position pointed to by m_pCurrent
void H264ENC_MAKE_NAME(H264EncoderFrameList_insertAtCurrent)(
    H264EncoderFrameListType* state,
    H264EncoderFrameType *pFrame);

// Mark all frames as not used as reference frames.
void H264ENC_MAKE_NAME(H264EncoderFrameList_removeAllRef)(
    H264EncoderFrameListType* state);

// Mark all frames as not used as reference frames.
void H264ENC_MAKE_NAME(H264EncoderFrameList_IncreaseRefPicListResetCount)(
    H264EncoderFrameListType* state,
    H264EncoderFrameType *ExcludeFrame);

// Mark the oldest short-term reference frame as not used.
void H264ENC_MAKE_NAME(H264EncoderFrameList_freeOldestShortTermRef)(
    H264EncoderFrameListType* state);

// Mark the short-term reference frame with specified PicNum as not used
void H264ENC_MAKE_NAME(H264EncoderFrameList_freeShortTermRef)(
    H264EncoderFrameListType* state,
    Ipp32s PicNum);

// Mark the long-term reference frame with specified LongTermPicNum  as not used
void H264ENC_MAKE_NAME(H264EncoderFrameList_freeLongTermRef)(
    H264EncoderFrameListType* state,
    Ipp32s LongTermPicNum);

// Mark the long-term reference frame with specified LongTermFrameIdx as not used
void H264ENC_MAKE_NAME(H264EncoderFrameList_freeLongTermRefIdx)(
    H264EncoderFrameListType* state,
    Ipp32s LongTermFrameIdx);

// Mark any long-term reference frame with LongTermFrameIdx greater than MaxLongTermFrameIdx as not used.
void H264ENC_MAKE_NAME(H264EncoderFrameList_freeOldLongTermRef)(
    H264EncoderFrameListType* state,
    Ipp32s MaxLongTermFrameIdx);

// Mark the short-term reference frame with specified PicNum as long-term with specified long term idx.
void H264ENC_MAKE_NAME(H264EncoderFrameList_changeSTtoLTRef)(
    H264EncoderFrameListType* state,
    Ipp32s PicNum,
    Ipp32s LongTermFrameIdx);

void H264ENC_MAKE_NAME(H264EncoderFrameList_countActiveRefs)(
    H264EncoderFrameListType* state,
    Ipp32u &NumShortTerm,
    Ipp32u &NumLongTerm);

// Return number of active short and long term reference frames.
void H264ENC_MAKE_NAME(H264EncoderFrameList_countL1Refs)(
    H264EncoderFrameListType* state,
    Ipp32u &NumRefs,
    Ipp32s curPOC);

// Search through the list for the oldest displayable frame.
H264EncoderFrameType* H264ENC_MAKE_NAME(H264EncoderFrameList_findOldestToEncode)(
    H264EncoderFrameListType* state,
    H264EncoderFrameListType* dpb,
    Ipp32u min_L1_refs,
    Ipp32s use_B_as_refs);

// Search through the list for the newest displayable frame.
H264EncoderFrameType* H264ENC_MAKE_NAME(H264EncoderFrameList_findNewestToEncode)(
    H264EncoderFrameListType* state);

Ipp32s H264ENC_MAKE_NAME(H264EncoderFrameList_countNumToEncode)(
    H264EncoderFrameListType* state);

#if defined (ALPHA_BLENDING_H264)
void H264ENC_MAKE_NAME(H264EncoderFrameList_switchToPrimary)(
    H264EncoderFrameListType* state);

void H264ENC_MAKE_NAME(H264EncoderFrameList_switchToAuxiliary)(
    H264EncoderFrameListType* state);
#endif

#undef EncoderRefPicListStructType
#undef EncoderRefPicListType
#undef H264EncoderFrameType
#undef H264EncoderFrameListType
#undef H264ENC_MAKE_NAME
#undef COEFFSTYPE
#undef PIXTYPE
