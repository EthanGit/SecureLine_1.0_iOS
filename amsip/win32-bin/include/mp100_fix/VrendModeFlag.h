/////////////////////////////////////////////////////
#ifndef __ITCCVREN_H__
#define __ITCCVREN_H__

#define xCHValue 0x100010 // chromakey value : R:16 G:0 B:16

struct VrenderModeFlag{
	unsigned int mode; 
	// 0: default(no alpha blending & no croma key)
	// 1: chroma key(using xCHValue)
	// 2: alpha blending 25%
	// 3: alpha blending & chroma keying
	unsigned int left;
	unsigned int top;
	unsigned int width;
	unsigned int height;
	unsigned int rotate;
	// CJH ADD 080617 : Alpha Blending [
	unsigned int alpha;	// 0% ~ 100%
	unsigned int ratio;
	unsigned int reserved[2];
	// CJH ADD 080617 : Alpha Blending ]
};

#define DMA2D_NO_OPERATION			0
#define DMA2D_ROTATE_90_DEGREES		1	// Count-clock Rotation 90 degrees
#define DMA2D_ROTATE_180_DEGREES		2	// Count-clock Rotation 180 degrees
#define DMA2D_ROTATE_270_DEGREES		3	// Count-clock Rotation 270 degrees
#define DMA2D_ROTATE_FLIP_VERTICAL	4	// Mirroring(flipping) vertically
#define DMA2D_ROTATE_FLIP_HORIZON	5	// Mirroring(flipping) horizontally
#define DMA2D_ROTATE_FLIP_ALL			6	// Mirroring(flipping) horizontally and vertically

//adf85a65-358f-4fcd-86b1-b8f3d6540324
static const GUID IID_ITccVren = 
{ 0xadf85a65, 0x358f, 0x4fcd, {0x86, 0xb1, 0xb8, 0xf3, 0xd6, 0x54, 0x03, 0x24} };

// {DB1922DC-3E3C-488f-8253-076C34F0FE5D}
static const GUID IID_ITccVren2 = 
{ 0xdb1922dc, 0x3e3c, 0x488f, { 0x82, 0x53, 0x7, 0x6c, 0x34, 0xf0, 0xfe, 0x5d } };

DECLARE_INTERFACE_(ITccVren, IUnknown)
{
	STDMETHOD(GetModeFlag)(THIS_ VrenderModeFlag* Flag) PURE;
	STDMETHOD(SetModeFlag)(THIS_  VrenderModeFlag Flag) PURE;
	STDMETHOD(SetAVSync)(THIS_ BOOL bAVSync) PURE;	
	STDMETHOD(SetM2MScaler)(THIS_ BOOL bM2MScaler) PURE;
	STDMETHOD(SetAlphaBlanding)(THIS_ BOOL AlphaBlanding) PURE;
};

typedef BOOL (CALLBACK *TCCINFORMATIONRECEIVECALLBACK)(DWORD, PVOID, PVOID);

DECLARE_INTERFACE_(ITccVren2, ITccVren)
{
	// blt YUV420 on video LCD layer
	STDMETHOD(BitBltYuv420)(THIS_ BYTE* pBuf, CONST INT imgWidth, CONST INT imgHeight) PURE;
	STDMETHOD(SetPreviewYuv420)(THIS_ CONST BYTE *pBuffer, CONST int imgWidth, CONST int imgHeight, TCCINFORMATIONRECEIVECALLBACK pfn, PVOID pParam) PURE;
};

#endif


