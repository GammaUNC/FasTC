//--------------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//--------------------------------------------------------------------------------------

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"

#include "DXTCompressorDLL.h" // DXT compressor DLL.
#include "BC7CompressorDLL.h" // BC7 compressor DLL.

#include "StopWatch.h" // Timer.
#include "TaskMgrTBB.h" // TBB task manager.

#include <tchar.h>
#include <strsafe.h>

#define ALIGN16(x) __declspec(align(16)) x
#define ALIGN32(x) __declspec(align(32)) x

// DXT compressor type.
enum ECompressorType
{
	eCompType_DXT1,
	eCompType_DXT5,
	eCompType_BC7,

	kNumCompressorTypes
};

const TCHAR *kCompressorTypeStr[kNumCompressorTypes] = {
	_T("DXT1/BC1"),
	_T("DXT5/BC3"),
	_T("BC7"),
};

enum EInstructionSet
{
	eInstrSet_Scalar
	, eInstrSet_SSE
	, eInstrSet_AVX2

	, kNumInstructionSets
};

const TCHAR *kInstructionSetStr[kNumInstructionSets] = {
	_T("Scalar"),
	_T("SSE"),
	_T("AVX2"),
};

enum EThreadMode
{
	eThreadMode_None,
	eThreadMode_TBB,
	eThreadMode_Win32,

	kNumThreadModes
};

const TCHAR *kThreadModeStr[kNumThreadModes] = {
	_T("None"),
	_T("TBB"),
	_T("Win32")
};

static BOOL						g_DXT1Available = TRUE;
static BOOL						g_AVX2Available = FALSE;
static BOOL						g_DX11Available = FALSE;

const struct ECompressionScheme {
	const ECompressorType type;
	const EInstructionSet instrSet;
	const EThreadMode threadMode;
	const BOOL &availabilityOverride;
} kCompressionSchemes[] = {
	{ eCompType_DXT1,	eInstrSet_Scalar,		eThreadMode_None,	g_DXT1Available },
	{ eCompType_DXT1,	eInstrSet_Scalar,		eThreadMode_TBB,	g_DXT1Available },
	{ eCompType_DXT1,	eInstrSet_Scalar,		eThreadMode_Win32,	g_DXT1Available },
	{ eCompType_DXT1,	eInstrSet_SSE,			eThreadMode_None,	g_DXT1Available },
	{ eCompType_DXT1,	eInstrSet_SSE,			eThreadMode_TBB,	g_DXT1Available },
	{ eCompType_DXT1,	eInstrSet_SSE,			eThreadMode_Win32,	g_DXT1Available },
	{ eCompType_DXT5,	eInstrSet_Scalar,		eThreadMode_None,	g_DXT1Available },
	{ eCompType_DXT5,	eInstrSet_Scalar,		eThreadMode_TBB,	g_DXT1Available },
	{ eCompType_DXT5,	eInstrSet_Scalar,		eThreadMode_Win32,	g_DXT1Available },
	{ eCompType_DXT5,	eInstrSet_SSE,			eThreadMode_None,	g_DXT1Available },
	{ eCompType_DXT5,	eInstrSet_SSE,			eThreadMode_TBB,	g_DXT1Available },
	{ eCompType_DXT5,	eInstrSet_SSE,			eThreadMode_Win32,	g_DXT1Available },
	{ eCompType_BC7,	eInstrSet_Scalar,		eThreadMode_None,	g_DX11Available },
	{ eCompType_BC7,	eInstrSet_Scalar,		eThreadMode_Win32,	g_DX11Available },
	{ eCompType_BC7,	eInstrSet_SSE,			eThreadMode_None,	g_DX11Available },
	{ eCompType_BC7,	eInstrSet_SSE,			eThreadMode_Win32,	g_DX11Available },
	{ eCompType_DXT1,	eInstrSet_AVX2,			eThreadMode_None,	g_AVX2Available },
	{ eCompType_DXT1,	eInstrSet_AVX2,			eThreadMode_TBB,	g_AVX2Available },
	{ eCompType_DXT1,	eInstrSet_AVX2,			eThreadMode_Win32,	g_AVX2Available },
	{ eCompType_DXT5,	eInstrSet_AVX2,			eThreadMode_None,	g_AVX2Available },
	{ eCompType_DXT5,	eInstrSet_AVX2,			eThreadMode_TBB,	g_AVX2Available },
	{ eCompType_DXT5,	eInstrSet_AVX2,			eThreadMode_Win32,	g_AVX2Available },	
};
const int kNumCompressionSchemes = sizeof(kCompressionSchemes) / sizeof(kCompressionSchemes[0]);
const ECompressionScheme *gCompressionScheme = kCompressionSchemes;

// Textured vertex.
struct Vertex
{
    D3DXVECTOR3 position;
	D3DXVECTOR2 texCoord;
};

// Global variables
CDXUTDialogResourceManager gDialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg gD3DSettingsDlg; // Device settings dialog
CDXUTDialog gHUD; // manages the 3D   
CDXUTDialog gSampleUI; // dialog for sample specific controls
bool gShowHelp = false; // If true, it renders the UI control text
CDXUTTextHelper* gTxtHelper = NULL;
double gCompTime = 0.0;
double gCompRate = 0.0;
int gBlocksPerTask = 256;
int gFrameNum = 0;
int gFrameDelay = 100;
int gTexWidth = 0;
int gTexHeight = 0;
double gError = 0.0;

#ifdef REPORT_RMSE
static const WCHAR *kErrorStr = L"Root Mean Squared Error";
#else
static const WCHAR *kErrorStr = L"Peak Signal/Noise Ratio";
#endif

ID3D11DepthStencilState* gDepthStencilState = NULL;
UINT gStencilReference = 0;
ID3D11InputLayout* gVertexLayout = NULL;
ID3D11Buffer* gVertexBuffer = NULL;
ID3D11Buffer* gQuadVB = NULL;
ID3D11Buffer* gIndexBuffer = NULL;
ID3D11VertexShader* gVertexShader = NULL;
ID3D11PixelShader* gRenderFramePS = NULL;
ID3D11PixelShader* gRenderTexturePS = NULL;
ID3D11SamplerState* gSamPoint = NULL;
ID3D11ShaderResourceView* gUncompressedSRV = NULL; // Shader resource view for the uncompressed texture resource.
ID3D11ShaderResourceView* gCompressedSRV = NULL; // Shader resource view for the compressed texture resource.
ID3D11ShaderResourceView* gErrorSRV = NULL; // Shader resource view for the error texture.

// Win32 thread API
const int kMaxWinThreads = 16;

enum EThreadState {
	eThreadState_WaitForData,
	eThreadState_DataLoaded,
	eThreadState_Running,
	eThreadState_Done
};

typedef void (* CompressionFunc)(const BYTE* inBuf, BYTE* outBuf, int width, int height);

struct WinThreadData {
	EThreadState state;
	int threadIdx;
	const BYTE *inBuf;
	BYTE *outBuf;
	int width;
	int height;
	void (*cmpFunc)(const BYTE* inBuf, BYTE* outBuf, int width, int height);

	// Defaults..
	WinThreadData() :
		state(eThreadState_Done),
		threadIdx(-1),
		inBuf(NULL),
		outBuf(NULL),
		width(-1),
		height(-1),
		cmpFunc(NULL)
	{ }

} gWinThreadData[kMaxWinThreads];

HANDLE gWinThreadWorkEvent[kMaxWinThreads];
HANDLE gWinThreadStartEvent = NULL;
HANDLE gWinThreadDoneEvent = NULL;
int gNumWinThreads = 0;
DWORD gNumProcessors = 1; // We have at least one processor.
DWORD dwThreadIdArray[kMaxWinThreads];
HANDLE hThreadArray[kMaxWinThreads];

// UI control IDs
#define IDC_TOGGLEFULLSCREEN          1
#define IDC_TOGGLEREF                 2
#define IDC_CHANGEDEVICE              3
#define IDC_UNCOMPRESSEDTEXT          4
#define IDC_COMPRESSEDTEXT            5
#define IDC_ERRORTEXT                 6
#define IDC_SIZETEXT                  7
#define IDC_TIMETEXT                  8
#define IDC_RATETEXT                  9
#define IDC_TBB                       10
#define IDC_SIMD                      11
#define IDC_COMPRESSOR                12
#define IDC_BLOCKSPERTASKTEXT         13
#define IDC_BLOCKSPERTASK             14
#define IDC_LOADTEXTURE               15
#define IDC_RECOMPRESS                16
#define IDC_RMSETEXT				  17

// Forward declarations 
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext );

void InitApp();
void RenderText();

void UpdateBlockSlider();
void UpdateCompressionAlgorithms();
void UpdateThreadingMode();
void UpdateCompressionModes();
void UpdateAllowedSettings();

void SetCompressionScheme(EInstructionSet instrSet, ECompressorType compType, EThreadMode threadMode);

HRESULT CreateTextures(LPTSTR file);
void DestroyTextures();
HRESULT LoadTexture(LPTSTR file);
HRESULT PadTexture(ID3D11ShaderResourceView** textureSRV);
HRESULT SaveTexture(ID3D11ShaderResourceView* textureSRV, LPTSTR file);
HRESULT CompressTexture(ID3D11ShaderResourceView* uncompressedSRV, ID3D11ShaderResourceView** compressedSRV);
HRESULT ComputeError(ID3D11ShaderResourceView* uncompressedSRV, ID3D11ShaderResourceView* compressedSRV, ID3D11ShaderResourceView** errorSRV);
HRESULT RecompressTexture();

void ComputeRMSE(const BYTE *errorData, const INT width, const INT height);

void InitWin32Threads();
void DestroyThreads();

void StoreDepthStencilState();
void RestoreDepthStencilState();
HRESULT DisableDepthTest();

namespace DXTC
{
	VOID CompressImageDXT(const BYTE* inBuf, BYTE* outBuf, INT width, INT height);

	VOID CompressImageDXTNoThread(const BYTE* inBuf, BYTE* outBuf, INT width, INT height);
	VOID CompressImageDXTTBB(const BYTE* inBuf, BYTE* outBuf, INT width, INT height);
	VOID CompressImageDXTWIN(const BYTE* inBuf, BYTE* outBuf, INT width, INT height);

	DWORD WINAPI CompressImageDXTWinThread( LPVOID lpParam );
}

#ifdef ENABLE_AVX2
#ifdef _M_X64
/* On x64, we can't have inline assembly in C files, see avxtest.asm */ 
extern "C" int __stdcall supports_AVX2();

#else ifdef WIN32
/* AVX2 instructions require 64 bit mode. */ 
extern "C" int __stdcall supports_AVX2() {
	return 0;
}
#endif // _M_X64
#endif // ENABLE_AVX2

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef ENABLE_AVX2
	g_AVX2Available = supports_AVX2();
#endif

	// Make sure that the event array is set to null...
	memset(gWinThreadWorkEvent, 0, sizeof(gWinThreadWorkEvent));

	// Figure out how many cores there are on this machine
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	gNumProcessors = sysinfo.dwNumberOfProcessors;

	// Make sure all of our threads are empty.
	for(int i = 0; i < kMaxWinThreads; i++) {
		hThreadArray[i] = NULL;
	}

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();

    DXUTInit( true, true, NULL );
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"Fast Texture Compressor" );

	// Try to create a device with DX11 feature set
    DXUTCreateDevice (D3D_FEATURE_LEVEL_11_0, true, 1280, 1024 );

	// If we don't have an adequate driver, then we revert to DX10 feature set...
	DXUTDeviceSettings settings = DXUTGetDeviceSettings();
	if(settings.d3d11.DriverType == D3D_DRIVER_TYPE_UNKNOWN || settings.d3d11.DriverType == D3D_DRIVER_TYPE_NULL) {
		DXUTCreateDevice(D3D_FEATURE_LEVEL_10_1, true, 1280, 1024);

		// !HACK! Force enumeration here in order to relocate hardware with new feature level
		DXUTGetD3D11Enumeration(true);
		DXUTCreateDevice(D3D_FEATURE_LEVEL_10_1, true, 1280, 1024);

		const TCHAR *noDx11msg = _T("Your hardware does not seem to support DX11. BC7 Compression is disabled.");
		MessageBox(NULL, noDx11msg, _T("Error"), MB_OK);
	}
	else {
		g_DX11Available = TRUE;
	}

	// Now that we know what things are allowed, update the available options.
	UpdateAllowedSettings();

    DXUTMainLoop();

	// Destroy all of the threads...
	DestroyThreads();

    return DXUTGetExitCode();
}

// Initialize the app 
void InitApp()
{
	// Initialize dialogs
	gD3DSettingsDlg.Init(&gDialogResourceManager);
	gHUD.Init(&gDialogResourceManager);
	gSampleUI.Init(&gDialogResourceManager);

	gHUD.SetCallback(OnGUIEvent);
	int x = 0;
	int y = 10;
	gHUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", x, y, 170, 23);
	gHUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", x, y += 26, 170, 23, VK_F3);
	gHUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", x, y += 26, 170, 23, VK_F2);

	gSampleUI.SetCallback(OnGUIEvent);
	x = 0;
	y = 0;
    gSampleUI.AddStatic(IDC_UNCOMPRESSEDTEXT, L"Uncompressed", x, y, 125, 22);
    gSampleUI.AddStatic(IDC_COMPRESSEDTEXT, L"Compressed", x, y, 125, 22);
    gSampleUI.AddStatic(IDC_ERRORTEXT, L"Error", x, y, 125, 22);
	WCHAR wstr[MAX_PATH];
	swprintf_s(wstr, MAX_PATH, L"Texture Size: %d x %d", gTexWidth, gTexHeight);
    gSampleUI.AddStatic(IDC_SIZETEXT, wstr, x, y, 125, 22);
	swprintf_s(wstr, MAX_PATH, L"%s: %.2f", kErrorStr, gError);
    gSampleUI.AddStatic(IDC_RMSETEXT, wstr, x, y, 125, 22);
	swprintf_s(wstr, MAX_PATH, L"Compression Time: %0.2f ms", gCompTime);
    gSampleUI.AddStatic(IDC_TIMETEXT, wstr, x, y, 125, 22);
	swprintf_s(wstr, MAX_PATH, L"Compression Rate: %0.2f Mp/s", gCompRate);
    gSampleUI.AddStatic(IDC_RATETEXT, wstr, x, y, 125, 22);
	gSampleUI.AddComboBox(IDC_TBB, x, y, 95, 22);
	gSampleUI.AddComboBox(IDC_SIMD, x, y, 140, 22);
	gSampleUI.AddComboBox(IDC_COMPRESSOR, x, y, 105, 22);
	swprintf_s(wstr, MAX_PATH, L"Blocks Per Task: %d", gBlocksPerTask);
	gSampleUI.AddStatic(IDC_BLOCKSPERTASKTEXT, wstr, x, y, 125, 22);	
	gSampleUI.AddSlider(IDC_BLOCKSPERTASK, x, y, 256, 22, 1, 512, gBlocksPerTask);
	gSampleUI.AddButton(IDC_LOADTEXTURE, L"Load Texture", x, y, 125, 22);
	gSampleUI.AddButton(IDC_RECOMPRESS, L"Recompress", x, y, 125, 22);
}

// Called right before creating a D3D11 device, allowing the app to modify the device settings as needed
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // Uncomment this to get debug information from D3D11
    //pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}

// Handle updates to the scene.
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{

}

// Render the help and statistics text
void RenderText()
{
    UINT nBackBufferHeight = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Height :
            DXUTGetDXGIBackBufferSurfaceDesc()->Height;

    gTxtHelper->Begin();
    gTxtHelper->SetInsertionPos( 2, 0 );
    gTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    gTxtHelper->DrawTextLine( DXUTGetFrameStats( false ) );
    gTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    // Draw help
    if( gShowHelp )
    {
        gTxtHelper->SetInsertionPos( 2, nBackBufferHeight - 20 * 6 );
        gTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        gTxtHelper->DrawTextLine( L"Controls:" );

        gTxtHelper->SetInsertionPos( 20, nBackBufferHeight - 20 * 5 );
        gTxtHelper->DrawTextLine( L"Hide help: F1\n"
                                    L"Quit: ESC\n" );
    }
    else
    {
        gTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        gTxtHelper->DrawTextLine( L"Press F1 for help" );
    }

    gTxtHelper->End();
}

// Handle messages to the application
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = gDialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( gD3DSettingsDlg.IsActive() )
    {
        gD3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = gHUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = gSampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    return 0;
}

// Handle key presses
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_F1:
                gShowHelp = !gShowHelp; break;
        }
    }
}

// Handles the GUI events
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
		{
            DXUTToggleFullScreen();
			break;
		}
        case IDC_TOGGLEREF:
		{
            DXUTToggleREF();
			break;
		}
        case IDC_CHANGEDEVICE:
		{
            gD3DSettingsDlg.SetActive( !gD3DSettingsDlg.IsActive() );
			break;
		}

		case IDC_TIMETEXT:
		{
			WCHAR wstr[MAX_PATH];
			swprintf_s(wstr, MAX_PATH, L"Compression Time: %0.2f ms", gCompTime);
			gSampleUI.GetStatic(IDC_TIMETEXT)->SetText(wstr);
			break;
		}
		case IDC_RATETEXT:
		{
			WCHAR wstr[MAX_PATH];
			swprintf_s(wstr, MAX_PATH, L"Compression Rate: %0.2f Mp/s", gCompRate);
			gSampleUI.GetStatic(IDC_RATETEXT)->SetText(wstr);
			break;
		}

		case IDC_RMSETEXT:
		{
			WCHAR wstr[MAX_PATH];
			swprintf_s(wstr, MAX_PATH, L"%s: %.2f", kErrorStr, gError);
			gSampleUI.GetStatic(IDC_RMSETEXT)->SetText(wstr);
			break;
		}

		case IDC_TBB:
		{
			// Shut down all previous threading abilities.
			DestroyThreads();

			EInstructionSet instrSet = gCompressionScheme->instrSet;
			ECompressorType compType = gCompressionScheme->type;

			EThreadMode newMode = (EThreadMode)(INT_PTR)gSampleUI.GetComboBox(IDC_TBB)->GetSelectedData();

			switch(newMode) {
				case eThreadMode_TBB:

					// Initialize the TBB task manager.
					gTaskMgr.Init();

					break;

				case eThreadMode_Win32:

					InitWin32Threads();

					break;

				case eThreadMode_None:
					// Do nothing, our threads are fine.
					break;
			}

			SetCompressionScheme(instrSet, compType, newMode);
			UpdateAllowedSettings();

			// Recompress the texture.
			RecompressTexture();

			break;
		}

		case IDC_SIMD:
		{
			EThreadMode threadMode = gCompressionScheme->threadMode;
			ECompressorType compType = gCompressionScheme->type;

			EInstructionSet newInstrSet = (EInstructionSet)(INT_PTR)gSampleUI.GetComboBox(IDC_SIMD)->GetSelectedData();

			// If we selected AVX2, then the total number of blocks when using AVX2 changes, so we need
			// to reflect that in the slider.
			UpdateBlockSlider();

			SetCompressionScheme(newInstrSet, compType, threadMode);
			UpdateAllowedSettings();

			// Recompress the texture.
			RecompressTexture();

			break;
		}
		case IDC_COMPRESSOR:
		{
			EThreadMode threadMode = gCompressionScheme->threadMode;
			EInstructionSet instrSet = gCompressionScheme->instrSet;
			ECompressorType newCompType = (ECompressorType)(INT_PTR)gSampleUI.GetComboBox(IDC_COMPRESSOR)->GetSelectedData();
			
			SetCompressionScheme(instrSet, newCompType, threadMode);
			UpdateAllowedSettings();

			// Recompress the texture.
			RecompressTexture();

			break;
		}
		case IDC_BLOCKSPERTASK:
		{
			gBlocksPerTask = gSampleUI.GetSlider(IDC_BLOCKSPERTASK)->GetValue();
			WCHAR wstr[MAX_PATH];
			swprintf_s(wstr, MAX_PATH, L"Blocks Per Task: %d", gBlocksPerTask);
			gSampleUI.GetStatic(IDC_BLOCKSPERTASKTEXT)->SetText(wstr);

			// Recompress the texture.
			RecompressTexture();

			break;
		}
		case IDC_LOADTEXTURE:
		{
			// Store the current working directory.
			TCHAR workingDirectory[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, workingDirectory);

			// Open a file dialog.
			OPENFILENAME openFileName;
			WCHAR file[MAX_PATH];
			file[0] = 0;
			ZeroMemory(&openFileName, sizeof(OPENFILENAME));
			openFileName.lStructSize = sizeof(OPENFILENAME);
			openFileName.lpstrFile = file;
			openFileName.nMaxFile = MAX_PATH;
			openFileName.lpstrFilter = L"DDS\0*.dds\0\0";
			openFileName.nFilterIndex = 1;
			openFileName.lpstrInitialDir = NULL;
			openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if(GetOpenFileName(&openFileName))
			{
				CreateTextures(openFileName.lpstrFile);
			}

			// Restore the working directory. GetOpenFileName changes the current working directory which causes problems with relative paths to assets.
			SetCurrentDirectory(workingDirectory);

			break;
		}
		case IDC_RECOMPRESS:
		{
			// Recompress the texture.
			RecompressTexture();

			break;
		}
    }
}

// Reject any D3D11 devices that aren't acceptable by returning false
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}

// Find and compile the specified shader
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;
}

// Create any D3D11 resources that aren't dependent on the back buffer
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN(gDialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
    V_RETURN(gD3DSettingsDlg.OnD3D11CreateDevice(pd3dDevice));
    gTxtHelper = new CDXUTTextHelper(pd3dDevice, pd3dImmediateContext, &gDialogResourceManager, 15);

    // Create a vertex shader.
    ID3DBlob* vertexShaderBuffer = NULL;
    V_RETURN(CompileShaderFromFile(L"FastTextureCompressor\\FastTextureCompressor.hlsl", "PassThroughVS", "vs_4_0", &vertexShaderBuffer));
    V_RETURN(pd3dDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &gVertexShader));

	// Create a pixel shader that renders the composite frame.
    ID3DBlob* pixelShaderBuffer = NULL;
    V_RETURN(CompileShaderFromFile(L"FastTextureCompressor\\FastTextureCompressor.hlsl", "RenderFramePS", "ps_4_0", &pixelShaderBuffer));
    V_RETURN(pd3dDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &gRenderFramePS));

	// Create a pixel shader that renders the error texture.
    V_RETURN(CompileShaderFromFile(L"FastTextureCompressor\\FastTextureCompressor.hlsl", "RenderTexturePS", "ps_4_0", &pixelShaderBuffer));
    V_RETURN(pd3dDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &gRenderTexturePS));

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    V_RETURN(pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &gVertexLayout));

    SAFE_RELEASE(vertexShaderBuffer);
    SAFE_RELEASE(pixelShaderBuffer);

	// Create a vertex buffer for three textured quads.
	D3DXVECTOR2 quadSize(0.32f, 0.32f);
	D3DXVECTOR2 quadOrigin(-0.66f, -0.0f);
    Vertex tripleQuadVertices[18];
	ZeroMemory(tripleQuadVertices, sizeof(tripleQuadVertices));
	for(int i = 0; i < 18; i += 6)
	{
		tripleQuadVertices[i].position = D3DXVECTOR3(quadOrigin.x - quadSize.x, quadOrigin.y + quadSize.y, 0.0f);
		tripleQuadVertices[i].texCoord = D3DXVECTOR2(0.0f, 0.0f);

		tripleQuadVertices[i + 1].position = D3DXVECTOR3(quadOrigin.x + quadSize.x, quadOrigin.y + quadSize.y, 0.0f);
		tripleQuadVertices[i + 1].texCoord = D3DXVECTOR2(1.0f, 0.0f);

		tripleQuadVertices[i + 2].position = D3DXVECTOR3(quadOrigin.x + quadSize.x, quadOrigin.y - quadSize.y, 0.0f);
		tripleQuadVertices[i + 2].texCoord = D3DXVECTOR2(1.0f, 1.0f);

		tripleQuadVertices[i + 3].position = D3DXVECTOR3(quadOrigin.x + quadSize.x, quadOrigin.y - quadSize.y, 0.0f);
		tripleQuadVertices[i + 3].texCoord = D3DXVECTOR2(1.0f, 1.0f);

		tripleQuadVertices[i + 4].position = D3DXVECTOR3(quadOrigin.x - quadSize.x, quadOrigin.y - quadSize.y, 0.0f);
		tripleQuadVertices[i + 4].texCoord = D3DXVECTOR2(0.0f, 1.0f);

		tripleQuadVertices[i + 5].position = D3DXVECTOR3(quadOrigin.x - quadSize.x, quadOrigin.y + quadSize.y, 0.0f);
		tripleQuadVertices[i + 5].texCoord = D3DXVECTOR2(0.0f, 0.0f);

		quadOrigin.x += 0.66f;
	}

    D3D11_BUFFER_DESC bufDesc;
	ZeroMemory(&bufDesc, sizeof(bufDesc));
    bufDesc.Usage = D3D11_USAGE_DEFAULT;
    bufDesc.ByteWidth = sizeof(tripleQuadVertices);
    bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
    data.pSysMem = tripleQuadVertices;
    V_RETURN(pd3dDevice->CreateBuffer(&bufDesc, &data, &gVertexBuffer));

	// Create a vertex buffer for a single textured quad.
	quadSize = D3DXVECTOR2(1.0f, 1.0f);
	quadOrigin = D3DXVECTOR2(0.0f, 0.0f);
	Vertex singleQuadVertices[6];
	singleQuadVertices[0].position = D3DXVECTOR3(quadOrigin.x - quadSize.x, quadOrigin.y + quadSize.y, 0.0f);
	singleQuadVertices[0].texCoord = D3DXVECTOR2(0.0f, 0.0f);
	singleQuadVertices[1].position = D3DXVECTOR3(quadOrigin.x + quadSize.x, quadOrigin.y + quadSize.y, 0.0f);
	singleQuadVertices[1].texCoord = D3DXVECTOR2(1.0f, 0.0f);
	singleQuadVertices[2].position = D3DXVECTOR3(quadOrigin.x + quadSize.x, quadOrigin.y - quadSize.y, 0.0f);
	singleQuadVertices[2].texCoord = D3DXVECTOR2(1.0f, 1.0f);
	singleQuadVertices[3].position = D3DXVECTOR3(quadOrigin.x + quadSize.x, quadOrigin.y - quadSize.y, 0.0f);
	singleQuadVertices[3].texCoord = D3DXVECTOR2(1.0f, 1.0f);
	singleQuadVertices[4].position = D3DXVECTOR3(quadOrigin.x - quadSize.x, quadOrigin.y - quadSize.y, 0.0f);
	singleQuadVertices[4].texCoord = D3DXVECTOR2(0.0f, 1.0f);
	singleQuadVertices[5].position = D3DXVECTOR3(quadOrigin.x - quadSize.x, quadOrigin.y + quadSize.y, 0.0f);
	singleQuadVertices[5].texCoord = D3DXVECTOR2(0.0f, 0.0f);

	ZeroMemory(&bufDesc, sizeof(bufDesc));
    bufDesc.Usage = D3D11_USAGE_DEFAULT;
    bufDesc.ByteWidth = sizeof(singleQuadVertices);
    bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufDesc.CPUAccessFlags = 0;
	ZeroMemory(&data, sizeof(data));
    data.pSysMem = singleQuadVertices;
    V_RETURN(pd3dDevice->CreateBuffer(&bufDesc, &data, &gQuadVB));

    // Create a sampler state
    D3D11_SAMPLER_DESC SamDesc;
    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.MipLODBias = 0.0f;
    SamDesc.MaxAnisotropy = 1;
    SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
    SamDesc.MinLOD = 0;
    SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &gSamPoint));

	// Load and initialize the textures.
    WCHAR path[MAX_PATH];
    V_RETURN(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, L"Images\\texture.dds"));
	V_RETURN(CreateTextures(path));

    return S_OK;
}

// Create any D3D11 resources that depend on the back buffer
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;
    V_RETURN( gDialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( gD3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    gHUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    gHUD.SetSize( 170, 170 );

    gSampleUI.SetLocation( 0, 0 );
    gSampleUI.SetSize( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

	int oneThirdWidth = int(gSampleUI.GetWidth() / 3.0f);
	int oneThirdHeight = int(gSampleUI.GetHeight() / 3.0f);
	int x = 20;
	int y = oneThirdHeight - 20;
    gSampleUI.GetStatic(IDC_UNCOMPRESSEDTEXT)->SetLocation(x, y);
    gSampleUI.GetStatic(IDC_COMPRESSEDTEXT)->SetLocation(x += oneThirdWidth, y);
    gSampleUI.GetStatic(IDC_ERRORTEXT)->SetLocation(x += oneThirdWidth, y);
	x = gSampleUI.GetWidth() - 276;
	y = gSampleUI.GetHeight() - 216;
    gSampleUI.GetStatic(IDC_SIZETEXT)->SetLocation(x, y);
	gSampleUI.GetStatic(IDC_RMSETEXT)->SetLocation(x, y += 26);
    gSampleUI.GetStatic(IDC_TIMETEXT)->SetLocation(x, y += 26);
    gSampleUI.GetStatic(IDC_RATETEXT)->SetLocation(x, y += 26);
	gSampleUI.GetComboBox(IDC_SIMD)->SetLocation(x, y += 26);
	gSampleUI.GetComboBox(IDC_COMPRESSOR)->SetLocation(x + 150, y);
	gSampleUI.GetStatic(IDC_BLOCKSPERTASKTEXT)->SetLocation(x, y += 26);
	gSampleUI.GetComboBox(IDC_TBB)->SetLocation(x + 160, y);
	gSampleUI.GetSlider(IDC_BLOCKSPERTASK)->SetLocation(x, y += 26);
	gSampleUI.GetButton(IDC_LOADTEXTURE)->SetLocation(x, y += 26);
	gSampleUI.GetButton(IDC_RECOMPRESS)->SetLocation(x + 131, y);

    return S_OK;
}

// Render the scene using the D3D11 device
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
	// Recompress the texture gFrameDelay frames after the app has started.  This produces more accurate timing of the
	// compression algorithm.
	if(gFrameNum == gFrameDelay)
	{
		RecompressTexture();
		gFrameNum++;
	}
	else if(gFrameNum < gFrameDelay)
	{
		gFrameNum++;
	}

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( gD3DSettingsDlg.IsActive() )
    {
        gD3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.02f, 0.02f, 0.02f, 1.0f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    // Set the input layout.
    pd3dImmediateContext->IASetInputLayout( gVertexLayout );

    // Set the vertex buffer.
    UINT stride = sizeof( Vertex );
    UINT offset = 0;
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, &gVertexBuffer, &stride, &offset );

    // Set the primitive topology
    pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // Set the shaders
    pd3dImmediateContext->VSSetShader( gVertexShader, NULL, 0 );
    pd3dImmediateContext->PSSetShader( gRenderFramePS, NULL, 0 );
    
	// Set the texture sampler.
    pd3dImmediateContext->PSSetSamplers( 0, 1, &gSamPoint );

	// Render the uncompressed texture.
	pd3dImmediateContext->PSSetShaderResources( 0, 1, &gUncompressedSRV );
    pd3dImmediateContext->Draw( 6, 0 );

	// Render the compressed texture.
	pd3dImmediateContext->PSSetShaderResources( 0, 1, &gCompressedSRV );
    pd3dImmediateContext->Draw( 6, 6 );

	// Render the error texture.
	pd3dImmediateContext->PSSetShaderResources( 0, 1, &gErrorSRV );
    pd3dImmediateContext->Draw( 6, 12 );

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    HRESULT hr;
    V(gHUD.OnRender( fElapsedTime ));
    V(gSampleUI.OnRender( fElapsedTime ));
    RenderText();
    DXUT_EndPerfEvent();
}

// Release D3D11 resources created in OnD3D11ResizedSwapChain 
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    gDialogResourceManager.OnD3D11ReleasingSwapChain();
}

// Release D3D11 resources created in OnD3D11CreateDevice 
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    gDialogResourceManager.OnD3D11DestroyDevice();
    gD3DSettingsDlg.OnD3D11DestroyDevice();
    //CDXUTDirectionWidget::StaticOnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( gTxtHelper );

    SAFE_RELEASE( gVertexLayout );
    SAFE_RELEASE( gVertexBuffer );
    SAFE_RELEASE( gQuadVB );
    SAFE_RELEASE( gIndexBuffer );
    SAFE_RELEASE( gVertexShader );
    SAFE_RELEASE( gRenderFramePS );
    SAFE_RELEASE( gRenderTexturePS );
    SAFE_RELEASE( gSamPoint );

	DestroyTextures();
}

// Free previously allocated texture resources and create new texture resources.
HRESULT CreateTextures(LPTSTR file)
{
	// Destroy any previously created textures.
	DestroyTextures();

	// Load the uncompressed texture.
	HRESULT hr;
	V_RETURN(LoadTexture(file));

	// Compress the texture.
	V_RETURN(CompressTexture(gUncompressedSRV, &gCompressedSRV));

	// Compute the error in the compressed texture.
	V_RETURN(ComputeError(gUncompressedSRV, gCompressedSRV, &gErrorSRV));

	return S_OK;
}

// Destroy texture resources.
void DestroyTextures()
{
	SAFE_RELEASE(gErrorSRV);
	SAFE_RELEASE(gCompressedSRV);
	SAFE_RELEASE(gUncompressedSRV);
}

// This functions loads a texture and prepares it for compression. The compressor only works on texture
// dimensions that are divisible by 4.  Textures that are not divisible by 4 are resized and padded with the edge values.
HRESULT LoadTexture(LPTSTR file)
{
	// Load the uncrompressed texture.
	// The loadInfo structure disables mipmapping by setting MipLevels to 1.
	D3DX11_IMAGE_LOAD_INFO loadInfo;
	ZeroMemory(&loadInfo, sizeof(D3DX11_IMAGE_LOAD_INFO));
	loadInfo.Width = D3DX11_DEFAULT;
	loadInfo.Height = D3DX11_DEFAULT;
	loadInfo.Depth = D3DX11_DEFAULT;
	loadInfo.FirstMipLevel = D3DX11_DEFAULT;
	loadInfo.MipLevels = 1;
	loadInfo.Usage = (D3D11_USAGE) D3DX11_DEFAULT;
	loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	loadInfo.CpuAccessFlags = D3DX11_DEFAULT;
	loadInfo.MiscFlags = D3DX11_DEFAULT;
	loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	loadInfo.Filter = D3DX11_FILTER_POINT | D3DX11_FILTER_SRGB;
	loadInfo.MipFilter = D3DX11_DEFAULT;
	loadInfo.pSrcInfo = NULL;
	HRESULT hr;
	V_RETURN(D3DX11CreateShaderResourceViewFromFile(DXUTGetD3D11Device(), file, &loadInfo, NULL, &gUncompressedSRV, NULL));

	// Pad the texture.
	V_RETURN(PadTexture(&gUncompressedSRV));

	// Query the texture description.
	ID3D11Texture2D* tex;
	gUncompressedSRV->GetResource((ID3D11Resource**)&tex);
	D3D11_TEXTURE2D_DESC texDesc;
	tex->GetDesc(&texDesc);
	SAFE_RELEASE(tex);

	// Update the UI's texture width and height.
	gTexWidth = texDesc.Width;
	gTexHeight = texDesc.Height;
	
	WCHAR wstr[MAX_PATH];
	swprintf_s(wstr, MAX_PATH, L"Texture Size: %d x %d", gTexWidth, gTexHeight);
	gSampleUI.GetStatic(IDC_SIZETEXT)->SetText(wstr);
	// gSampleUI.SendEvent(IDC_SIZETEXT, true, gSampleUI.GetStatic(IDC_SIZETEXT));

	UpdateBlockSlider();

	return S_OK;
}

void SetCompressionScheme(EInstructionSet instrSet, ECompressorType compType, EThreadMode threadMode) {

	bool foundMatch = false;
	for(int i = 0; i < kNumCompressionSchemes; i++) {
		bool match = true;
		match = match && kCompressionSchemes[i].instrSet == instrSet;
		match = match && kCompressionSchemes[i].type == compType;
		match = match && kCompressionSchemes[i].threadMode == threadMode;
		if(match) {
			gCompressionScheme = &(kCompressionSchemes[i]);
			foundMatch = true;
			break;
		}
	}

	if(!foundMatch) {
		OutputDebugString(L"ERROR: Did not find match for compression scheme, not changing.\n");
	}
}

void UpdateCompressionModes() {

	CDXUTComboBox *comboBox = gSampleUI.GetComboBox(IDC_COMPRESSOR);
	comboBox->RemoveAllItems();

	// If we're updating the compression modes, then see 
	// what we currently have selected and keep everything else constant.
	EThreadMode currThreadMode = gCompressionScheme->threadMode;
	EInstructionSet currInstrSet = gCompressionScheme->instrSet;

	bool added[kNumCompressorTypes];
	memset(added, 0, sizeof(added));

	for(int i = 0; i < kNumCompressionSchemes; i++) {

		bool match = kCompressionSchemes[i].instrSet == currInstrSet;
		match = match && kCompressionSchemes[i].threadMode == currThreadMode;
		match = match && kCompressionSchemes[i].availabilityOverride;

		if(match) {
			ECompressorType compType = kCompressionSchemes[i].type;
			if(!added[compType]) {
				comboBox->AddItem(kCompressorTypeStr[compType], (void*)(INT_PTR)compType);
				added[compType] = true;
			}
		}
	}

	comboBox->SetSelectedByData((void *)(INT_PTR)(gCompressionScheme->type));
}

void UpdateCompressionAlgorithms() {
	
	CDXUTComboBox *comboBox = gSampleUI.GetComboBox(IDC_SIMD);
	comboBox->RemoveAllItems();

	// If we're updating the compression algorithms, then see 
	// what we currently have selected and keep everything else constant.
	EThreadMode currThreadMode = gCompressionScheme->threadMode;
	ECompressorType currType = gCompressionScheme->type;

	bool added[kNumInstructionSets];
	memset(added, 0, sizeof(added));

	for(int i = 0; i < kNumCompressionSchemes; i++) {

		bool match = kCompressionSchemes[i].type == currType;
		match = match && kCompressionSchemes[i].threadMode == currThreadMode;
		match = match && kCompressionSchemes[i].availabilityOverride;

		if(match) {
			EInstructionSet instrSet = kCompressionSchemes[i].instrSet;
			if(!added[instrSet]) {
				comboBox->AddItem(kInstructionSetStr[instrSet], (void*)(INT_PTR)instrSet);
				added[instrSet] = true;
			}
		}
	}

	comboBox->SetSelectedByData((void *)(INT_PTR)(gCompressionScheme->instrSet));
}

void UpdateThreadingMode() {
	
	CDXUTComboBox *comboBox = gSampleUI.GetComboBox(IDC_TBB);
	comboBox->RemoveAllItems();

	// If we're updating the compression algorithms, then see 
	// what we currently have selected and keep everything else constant.
	EInstructionSet currInstrSet = gCompressionScheme->instrSet;
	ECompressorType currType = gCompressionScheme->type;

	bool added[kNumThreadModes];
	memset(added, 0, sizeof(added));

	for(int i = 0; i < kNumCompressionSchemes; i++) {

		bool match = kCompressionSchemes[i].type == currType;
		match = match && kCompressionSchemes[i].instrSet == currInstrSet;
		match = match && kCompressionSchemes[i].availabilityOverride;

		if(match) {
			EThreadMode threadMode = kCompressionSchemes[i].threadMode;
			if(!added[threadMode]) {
				comboBox->AddItem(kThreadModeStr[threadMode], (void*)(INT_PTR)threadMode);
				added[threadMode] = true;
			}
		}
	}

	comboBox->SetSelectedByData((void *)(INT_PTR)(gCompressionScheme->threadMode));
}

void UpdateAllowedSettings() {
	UpdateCompressionModes();
	UpdateCompressionAlgorithms();
	UpdateThreadingMode();
}

void UpdateBlockSlider() {

	int blockRows = gTexHeight / 4;
	int blockCols = gTexWidth / 4;
	if(gCompressionScheme->instrSet == eInstrSet_AVX2) {
		blockCols /= 2;
	}

	int numBlocks = blockRows * blockCols;
	int blksPerProc = numBlocks / gNumProcessors;

	gSampleUI.GetSlider(IDC_BLOCKSPERTASK)->SetRange(1, blksPerProc);
}

// Pad the texture to dimensions that are divisible by 4.
HRESULT PadTexture(ID3D11ShaderResourceView** textureSRV)
{
	// Query the texture description.
	ID3D11Texture2D* tex;
	(*textureSRV)->GetResource((ID3D11Resource**)&tex);
	D3D11_TEXTURE2D_DESC texDesc;
	tex->GetDesc(&texDesc);

	// Exit if the texture dimensions are divisible by 4.
	if((texDesc.Width % 4 == 0) && (texDesc.Height % 4 == 0))
	{
		SAFE_RELEASE(tex);
		return S_OK;
	}

	// Compute the size of the padded texture.
	UINT padWidth = texDesc.Width / 4 * 4 + 4;
	UINT padHeight = texDesc.Height / 4 * 4 + 4;

	// Create a buffer for the padded texels.
	BYTE* padTexels = new BYTE[padWidth * padHeight * 4];

	// Create a staging resource for the texture.
	HRESULT hr;
	ID3D11Device* device = DXUTGetD3D11Device();
	D3D11_TEXTURE2D_DESC stgTexDesc;
	memcpy(&stgTexDesc, &texDesc, sizeof(D3D11_TEXTURE2D_DESC));
	stgTexDesc.Usage = D3D11_USAGE_STAGING;
	stgTexDesc.BindFlags = 0;
	stgTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	ID3D11Texture2D* stgTex;
	V_RETURN(device->CreateTexture2D(&stgTexDesc, NULL, &stgTex));

	// Copy the texture into the staging resource.
    ID3D11DeviceContext* deviceContext = DXUTGetD3D11DeviceContext();
	deviceContext->CopyResource(stgTex, tex);

	// Map the staging resource.
	D3D11_MAPPED_SUBRESOURCE texData;
	V_RETURN(deviceContext->Map(stgTex, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_READ_WRITE, 0, &texData));

	// Copy the beginning of each row.
	BYTE* texels = (BYTE*)texData.pData;
	for(UINT row = 0; row < stgTexDesc.Height; row++)
	{
		UINT rowStart = row * texData.RowPitch;
		UINT padRowStart = row * padWidth * 4;
		memcpy(padTexels + padRowStart, texels + rowStart, stgTexDesc.Width * 4); 

		// Pad the end of each row.
		if(padWidth > stgTexDesc.Width)
		{
			BYTE* padVal = texels + rowStart + (stgTexDesc.Width - 1) * 4;
			for(UINT padCol = stgTexDesc.Width; padCol < padWidth; padCol++)
			{
				UINT padColStart = padCol * 4;
				memcpy(padTexels + padRowStart + padColStart, padVal, 4);
			}
		}
	}

	// Pad the end of each column.
	if(padHeight > stgTexDesc.Height)
	{
		UINT lastRow = (stgTexDesc.Height - 1);
		UINT lastRowStart = lastRow * padWidth * 4;
		BYTE* padVal = padTexels + lastRowStart;
		for(UINT padRow = stgTexDesc.Height; padRow < padHeight; padRow++)
		{
			UINT padRowStart = padRow * padWidth * 4;
			memcpy(padTexels + padRowStart, padVal, padWidth * 4);
		}
	}

	// Unmap the staging resources.
	deviceContext->Unmap(stgTex, D3D11CalcSubresource(0, 0, 1));

	// Create a padded texture.
	D3D11_TEXTURE2D_DESC padTexDesc;
	memcpy(&padTexDesc, &texDesc, sizeof(D3D11_TEXTURE2D_DESC));
	padTexDesc.Width = padWidth;
	padTexDesc.Height = padHeight;
	D3D11_SUBRESOURCE_DATA padTexData;
	ZeroMemory(&padTexData, sizeof(D3D11_SUBRESOURCE_DATA));
	padTexData.pSysMem = padTexels;
	padTexData.SysMemPitch = padWidth * sizeof(BYTE) * 4;
	ID3D11Texture2D* padTex;
	V_RETURN(device->CreateTexture2D(&padTexDesc, &padTexData, &padTex));

	// Delete the padded texel buffer.
	delete [] padTexels;

	// Release the shader resource view for the texture.
	SAFE_RELEASE(*textureSRV);

	// Create a shader resource view for the padded texture.
	D3D11_SHADER_RESOURCE_VIEW_DESC padTexSRVDesc;
	padTexSRVDesc.Format = padTexDesc.Format;
	padTexSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	padTexSRVDesc.Texture2D.MipLevels = padTexDesc.MipLevels;
	padTexSRVDesc.Texture2D.MostDetailedMip = padTexDesc.MipLevels - 1;
	V_RETURN(device->CreateShaderResourceView(padTex, &padTexSRVDesc, textureSRV));

	// Release resources.
	SAFE_RELEASE(padTex);
	SAFE_RELEASE(stgTex);
	SAFE_RELEASE(tex);

	return S_OK;
}

// Save a texture to a file.
HRESULT SaveTexture(ID3D11ShaderResourceView* textureSRV, LPTSTR file)
{
	// Get the texture resource.
	ID3D11Resource* texRes;
	textureSRV->GetResource(&texRes);
	if(texRes == NULL)
	{
		return E_POINTER;
	}

	// Save the texture to a file.
	HRESULT hr;
	V_RETURN(D3DX11SaveTextureToFile(DXUTGetD3D11DeviceContext(), texRes, D3DX11_IFF_DDS, file));

	// Release the texture resources.
	SAFE_RELEASE(texRes);

	return S_OK;
}

// Compress a texture.
HRESULT CompressTexture(ID3D11ShaderResourceView* uncompressedSRV, ID3D11ShaderResourceView** compressedSRV)
{
	// Query the texture description of the uncompressed texture.
	ID3D11Resource* uncompRes;
	gUncompressedSRV->GetResource(&uncompRes);
	D3D11_TEXTURE2D_DESC uncompTexDesc;
	((ID3D11Texture2D*)uncompRes)->GetDesc(&uncompTexDesc);

	// Create a 2D texture for the compressed texture.
	HRESULT hr;
	ID3D11Texture2D* compTex;
	D3D11_TEXTURE2D_DESC compTexDesc;
	memcpy(&compTexDesc, &uncompTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	
	switch(gCompressionScheme->type) {
	default:
	case eCompType_DXT1:
		compTexDesc.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
		break;
	case eCompType_DXT5:
		compTexDesc.Format = DXGI_FORMAT_BC3_UNORM_SRGB;
		break;
	case eCompType_BC7:
		compTexDesc.Format = DXGI_FORMAT_BC7_UNORM_SRGB;
		break;
	}

	ID3D11Device* device = DXUTGetD3D11Device();
	V_RETURN(device->CreateTexture2D(&compTexDesc, NULL, &compTex));

	// Create a shader resource view for the compressed texture.
	SAFE_RELEASE(*compressedSRV);
	D3D11_SHADER_RESOURCE_VIEW_DESC compSRVDesc;
	compSRVDesc.Format = compTexDesc.Format;
	compSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	compSRVDesc.Texture2D.MipLevels = compTexDesc.MipLevels;
	compSRVDesc.Texture2D.MostDetailedMip = compTexDesc.MipLevels - 1;
	V_RETURN(device->CreateShaderResourceView(compTex, &compSRVDesc, compressedSRV));

	// Create a staging resource for the compressed texture.
	compTexDesc.Usage = D3D11_USAGE_STAGING;
	compTexDesc.BindFlags = 0;
	compTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	ID3D11Texture2D* compStgTex;
	V_RETURN(device->CreateTexture2D(&compTexDesc, NULL, &compStgTex));

	// Create a staging resource for the uncompressed texture.
	uncompTexDesc.Usage = D3D11_USAGE_STAGING;
	uncompTexDesc.BindFlags = 0;
	uncompTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	ID3D11Texture2D* uncompStgTex;
	V_RETURN(device->CreateTexture2D(&uncompTexDesc, NULL, &uncompStgTex));

	// Copy the uncompressed texture into the staging resource.
    ID3D11DeviceContext* deviceContext = DXUTGetD3D11DeviceContext();
	deviceContext->CopyResource(uncompStgTex, uncompRes);

	// Map the staging resources.
	D3D11_MAPPED_SUBRESOURCE uncompData;
	V_RETURN(deviceContext->Map(uncompStgTex, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_READ_WRITE, 0, &uncompData));
	D3D11_MAPPED_SUBRESOURCE compData;
	V_RETURN(deviceContext->Map(compStgTex, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_READ_WRITE, 0, &compData));

	// Time the compression.
	StopWatch stopWatch;
	stopWatch.Start();

	const int kNumCompressions = 1;
	for(int cmpNum = 0; cmpNum < kNumCompressions; cmpNum++) {

		// Compress the uncompressed texels.
		DXTC::CompressImageDXT((BYTE*)uncompData.pData, (BYTE*)compData.pData, uncompTexDesc.Width, uncompTexDesc.Height);
	}

	// Update the compression time.
	stopWatch.Stop();
	gCompTime = stopWatch.TimeInMilliseconds();
	gSampleUI.SendEvent(IDC_TIMETEXT, true, gSampleUI.GetStatic(IDC_TIMETEXT));

	// Compute the compression rate.
	INT numPixels = compTexDesc.Width * compTexDesc.Height * kNumCompressions;
	gCompRate = (double)numPixels / stopWatch.TimeInSeconds() / 1000000.0;
	gSampleUI.SendEvent(IDC_RATETEXT, true, gSampleUI.GetStatic(IDC_RATETEXT));
	stopWatch.Reset();

	// Unmap the staging resources.
	deviceContext->Unmap(compStgTex, D3D11CalcSubresource(0, 0, 1));
	deviceContext->Unmap(uncompStgTex, D3D11CalcSubresource(0, 0, 1));

	// Copy the staging resourse into the compressed texture.
	deviceContext->CopyResource(compTex, compStgTex);

	// Release resources.
	SAFE_RELEASE(uncompStgTex);
	SAFE_RELEASE(compStgTex);
	SAFE_RELEASE(compTex);
	SAFE_RELEASE(uncompRes);

	return S_OK;
}

#define CHECK_WIN_THREAD_FUNC(x) \
	do { \
		if(NULL == (x)) { \
			wchar_t wstr[256]; \
			swprintf_s(wstr, L"Error detected from call %s at line %d of main.cpp", _T(#x), __LINE__); \
			ReportWinThreadError(wstr); \
		} \
	} \
	while(0)

void ReportWinThreadError(const wchar_t *str) {

	// Retrieve the system error message for the last-error code.
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	// Display the error message.

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR) lpMsgBuf) + lstrlen((LPCTSTR)str) + 40) * sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		str, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR) lpDisplayBuf, TEXT("Error"), MB_OK); 

	// Free error-handling buffer allocations.

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

void InitWin32Threads() {

	// Already initialized?
	if(gNumWinThreads > 0) {
		return;
	}
	
	SetLastError(0);

	gNumWinThreads = gNumProcessors;
	if(gNumWinThreads >= MAXIMUM_WAIT_OBJECTS)
		gNumWinThreads = MAXIMUM_WAIT_OBJECTS;

	// Create the synchronization events.
	for(int i = 0; i < gNumWinThreads; i++) {
		CHECK_WIN_THREAD_FUNC(gWinThreadWorkEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL));
	}

	CHECK_WIN_THREAD_FUNC(gWinThreadStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL));
	CHECK_WIN_THREAD_FUNC(gWinThreadDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL));

	// Create threads
	for(int threadIdx = 0; threadIdx < gNumWinThreads; threadIdx++) {
		gWinThreadData[threadIdx].state = eThreadState_WaitForData;
		CHECK_WIN_THREAD_FUNC(hThreadArray[threadIdx] = CreateThread(NULL, 0, DXTC::CompressImageDXTWinThread, &gWinThreadData[threadIdx], 0, &dwThreadIdArray[threadIdx]));
	}
}

void DestroyThreads() {

	switch(gCompressionScheme->threadMode) {
		case eThreadMode_TBB:
		{
			// Shutdown the TBB task manager.
			gTaskMgr.Shutdown();
		}
		break;

		case eThreadMode_Win32:
		{
			// Release all windows threads that may be active...
			for(int i=0; i < gNumWinThreads; i++) {
				gWinThreadData[i].state = eThreadState_Done;
			}

			// Send the event for the threads to start.
			CHECK_WIN_THREAD_FUNC(ResetEvent(gWinThreadDoneEvent));
			CHECK_WIN_THREAD_FUNC(SetEvent(gWinThreadStartEvent));

			// Wait for all the threads to finish....
			DWORD dwWaitRet = WaitForMultipleObjects(gNumWinThreads, hThreadArray, TRUE, INFINITE);
			if(WAIT_FAILED == dwWaitRet)
				ReportWinThreadError(L"DestroyThreads() -- WaitForMultipleObjects");

			// !HACK! This doesn't actually do anything. There is either a bug in the 
			// Intel compiler or the windows run-time that causes the threads to not
			// be cleaned up properly if the following two lines of code are not present.
			// Since we're passing INFINITE to WaitForMultipleObjects, that function will
			// never time out and per-microsoft spec, should never give this return value...
			// Even with these lines, the bug does not consistently disappear unless you
			// clean and rebuild. Heigenbug?
			//
			// If we compile with MSVC, then the following two lines are not necessary.
			else if(WAIT_TIMEOUT == dwWaitRet)
				OutputDebugString(L"DestroyThreads() -- WaitForMultipleObjects -- TIMEOUT");

			// Reset the start event
			CHECK_WIN_THREAD_FUNC(ResetEvent(gWinThreadStartEvent));
			CHECK_WIN_THREAD_FUNC(SetEvent(gWinThreadDoneEvent));

			// Close all thread handles.
			for(int i=0; i < gNumWinThreads; i++) {
				CHECK_WIN_THREAD_FUNC(CloseHandle(hThreadArray[i]));
			}

			for(int i =0; i < kMaxWinThreads; i++ ){
				hThreadArray[i] = NULL;
			}

			// Close all event handles...
			CHECK_WIN_THREAD_FUNC(CloseHandle(gWinThreadDoneEvent)); 
			gWinThreadDoneEvent = NULL;
			
			CHECK_WIN_THREAD_FUNC(CloseHandle(gWinThreadStartEvent)); 
			gWinThreadStartEvent = NULL;

			for(int i = 0; i < gNumWinThreads; i++) {
				CHECK_WIN_THREAD_FUNC(CloseHandle(gWinThreadWorkEvent[i]));
			}

			for(int i = 0; i < kMaxWinThreads; i++) {
				gWinThreadWorkEvent[i] = NULL;
			}

			gNumWinThreads = 0;
		}
		break;

		case eThreadMode_None:
			// Do nothing.
			break;
	}
}

static inline DXGI_FORMAT GetNonSRGBFormat(DXGI_FORMAT f) {
	switch(f) {
		case DXGI_FORMAT_BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM;
		case DXGI_FORMAT_BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM;
		case DXGI_FORMAT_BC7_UNORM_SRGB: return DXGI_FORMAT_BC7_UNORM; 
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
		default: assert(!"Unknown format!");
	}
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

// Compute an "error" texture that represents the absolute difference in color between an
// uncompressed texture and a compressed texture.
HRESULT ComputeError(ID3D11ShaderResourceView* uncompressedSRV, ID3D11ShaderResourceView* compressedSRV, ID3D11ShaderResourceView** errorSRV)
{
	HRESULT hr;

	// Query the texture description of the uncompressed texture.
	ID3D11Resource* uncompRes;
	gUncompressedSRV->GetResource(&uncompRes);
	D3D11_TEXTURE2D_DESC uncompTexDesc;
	((ID3D11Texture2D*)uncompRes)->GetDesc(&uncompTexDesc);

	// Query the texture description of the uncompressed texture.
	ID3D11Resource* compRes;
	gCompressedSRV->GetResource(&compRes);
	D3D11_TEXTURE2D_DESC compTexDesc;
	((ID3D11Texture2D*)compRes)->GetDesc(&compTexDesc);

	// Create a 2D resource without gamma correction for the two textures.
	compTexDesc.Format = GetNonSRGBFormat(compTexDesc.Format);
	uncompTexDesc.Format = GetNonSRGBFormat(uncompTexDesc.Format);

	ID3D11Device* device = DXUTGetD3D11Device();

	ID3D11Texture2D* uncompTex;
	device->CreateTexture2D(&uncompTexDesc, NULL, &uncompTex);

	ID3D11Texture2D* compTex;
	device->CreateTexture2D(&compTexDesc, NULL, &compTex);

	// Create a shader resource view for the two textures.
	D3D11_SHADER_RESOURCE_VIEW_DESC compSRVDesc;
	compSRVDesc.Format = compTexDesc.Format;
	compSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	compSRVDesc.Texture2D.MipLevels = compTexDesc.MipLevels;
	compSRVDesc.Texture2D.MostDetailedMip = compTexDesc.MipLevels - 1;
	ID3D11ShaderResourceView *compSRV;
	V_RETURN(device->CreateShaderResourceView(compTex, &compSRVDesc, &compSRV));

	D3D11_SHADER_RESOURCE_VIEW_DESC uncompSRVDesc;
	uncompSRVDesc.Format = uncompTexDesc.Format;
	uncompSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	uncompSRVDesc.Texture2D.MipLevels = uncompTexDesc.MipLevels;
	uncompSRVDesc.Texture2D.MostDetailedMip = uncompTexDesc.MipLevels - 1;
	ID3D11ShaderResourceView *uncompSRV;
	V_RETURN(device->CreateShaderResourceView(uncompTex, &uncompSRVDesc, &uncompSRV));

	// Create a 2D texture for the error texture.
	ID3D11Texture2D* errorTex;
	D3D11_TEXTURE2D_DESC errorTexDesc;
	memcpy(&errorTexDesc, &uncompTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	errorTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	V_RETURN(device->CreateTexture2D(&errorTexDesc, NULL, &errorTex));

	// Create a render target view for the error texture.
	D3D11_RENDER_TARGET_VIEW_DESC errorRTVDesc;
	errorRTVDesc.Format = errorTexDesc.Format;
	errorRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	errorRTVDesc.Texture2D.MipSlice = 0;
	ID3D11RenderTargetView* errorRTV;
	V_RETURN(device->CreateRenderTargetView(errorTex, &errorRTVDesc, &errorRTV));

	// Create a shader resource view for the error texture.
	D3D11_SHADER_RESOURCE_VIEW_DESC errorSRVDesc;
	errorSRVDesc.Format = errorTexDesc.Format;
	errorSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	errorSRVDesc.Texture2D.MipLevels = errorTexDesc.MipLevels;
	errorSRVDesc.Texture2D.MostDetailedMip = errorTexDesc.MipLevels - 1;
	V_RETURN(device->CreateShaderResourceView(errorTex, &errorSRVDesc, errorSRV));

	// Create a query for the GPU operations...
	D3D11_QUERY_DESC GPUQueryDesc;
	GPUQueryDesc.Query = D3D11_QUERY_EVENT;
	GPUQueryDesc.MiscFlags = 0;

#ifdef _DEBUG
	D3D11_QUERY_DESC OcclusionQueryDesc;
	OcclusionQueryDesc.Query = D3D11_QUERY_OCCLUSION;
	OcclusionQueryDesc.MiscFlags = 0;

	D3D11_QUERY_DESC StatsQueryDesc;
	StatsQueryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
	StatsQueryDesc.MiscFlags = 0;
#endif

	ID3D11Query *GPUQuery;
	V_RETURN(device->CreateQuery(&GPUQueryDesc, &GPUQuery));

	ID3D11DeviceContext* deviceContext = DXUTGetD3D11DeviceContext();

	deviceContext->CopyResource(compTex, compRes);
	deviceContext->CopyResource(uncompTex, uncompRes);

#ifdef _DEBUG
	ID3D11Query *OcclusionQuery, *StatsQuery;
	V_RETURN(device->CreateQuery(&OcclusionQueryDesc, &OcclusionQuery));
	V_RETURN(device->CreateQuery(&StatsQueryDesc, &StatsQuery));

	deviceContext->Begin(OcclusionQuery);
	deviceContext->Begin(StatsQuery);
#endif	

	// Set the viewport to a 1:1 mapping of pixels to texels.
	D3D11_VIEWPORT viewport;
	viewport.Width = (FLOAT)errorTexDesc.Width;
	viewport.Height = (FLOAT)errorTexDesc.Height;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	deviceContext->RSSetViewports(1, &viewport);

	// Bind the render target view of the error texture.
	ID3D11RenderTargetView* RTV[1] = { errorRTV };
	deviceContext->OMSetRenderTargets(1, RTV, NULL);

	// Clear the render target.
	FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	deviceContext->ClearRenderTargetView(errorRTV, color);

	// Set the input layout.
	deviceContext->IASetInputLayout(gVertexLayout);

	// Set vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &gQuadVB, &stride, &offset);

	// Set the primitive topology
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set the shaders
	deviceContext->VSSetShader(gVertexShader, NULL, 0);
	deviceContext->PSSetShader(gRenderTexturePS, NULL, 0);

	// Set the texture sampler.
	deviceContext->PSSetSamplers(0, 1, &gSamPoint);

	// Bind the textures.
	ID3D11ShaderResourceView* SRV[2] = { compSRV, uncompSRV};
	deviceContext->PSSetShaderResources(0, 2, SRV);

	// Store the depth/stencil state.
	StoreDepthStencilState();

	// Disable depth testing.
	V_RETURN(DisableDepthTest());

	// Render a quad.
	deviceContext->Draw(6, 0);

	// Restore the depth/stencil state.
	RestoreDepthStencilState();

	// Reset the render target.
	RTV[0] = DXUTGetD3D11RenderTargetView();
    deviceContext->OMSetRenderTargets(1, RTV, DXUTGetD3D11DepthStencilView());

	// Reset the viewport.
	viewport.Width = (FLOAT)DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	viewport.Height = (FLOAT)DXUTGetDXGIBackBufferSurfaceDesc()->Height;
	deviceContext->RSSetViewports(1, &viewport);

	deviceContext->End(GPUQuery);
#ifdef _DEBUG
	deviceContext->End(OcclusionQuery);
	deviceContext->End(StatsQuery);
#endif

	BOOL finishedGPU = false;

	// If we do not have a d3d 11 context, we will still hit this line and try to
	// finish using the GPU. If this happens this enters an infinite loop.
	int infLoopPrevention = 0;
	while(!finishedGPU && ++infLoopPrevention < 10000) {
		HRESULT ret;
		V_RETURN(ret = deviceContext->GetData(GPUQuery, &finishedGPU, sizeof(BOOL), 0));
		if(ret != S_OK)
			Sleep(1);
	}

#ifdef _DEBUG
	UINT64 nPixelsWritten = 0;
	deviceContext->GetData(OcclusionQuery, (void *)&nPixelsWritten, sizeof(UINT64), 0);

	D3D11_QUERY_DATA_PIPELINE_STATISTICS stats;
	deviceContext->GetData(StatsQuery, (void *)&stats, sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS), 0);

	TCHAR nPixelsWrittenMsg[256];
	_stprintf(nPixelsWrittenMsg, _T("Pixels rendered during error computation: %d\n"), nPixelsWritten);
	OutputDebugString(nPixelsWrittenMsg);
#endif

	// Create a copy of the error texture that is accessible by the CPU
	ID3D11Texture2D* errorTexCopy;
	D3D11_TEXTURE2D_DESC errorTexCopyDesc;
	memcpy(&errorTexCopyDesc, &uncompTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	errorTexCopyDesc.Usage = D3D11_USAGE_STAGING;
	errorTexCopyDesc.BindFlags = 0;
	errorTexCopyDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	V_RETURN(device->CreateTexture2D(&errorTexCopyDesc, NULL, &errorTexCopy));

	// Copy the error texture into the copy....
	deviceContext->CopyResource(errorTexCopy, errorTex);

	// Map the staging resource.
	D3D11_MAPPED_SUBRESOURCE errorData;
	V_RETURN(deviceContext->Map(errorTexCopy, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_READ, 0, &errorData));

	// Calculate PSNR
	ComputeRMSE((const BYTE *)(errorData.pData), errorTexCopyDesc.Width, errorTexCopyDesc.Height);
	gSampleUI.SendEvent(IDC_RMSETEXT, true, gSampleUI.GetStatic(IDC_RMSETEXT));

	// Unmap the staging resources.
	deviceContext->Unmap(errorTexCopy, D3D11CalcSubresource(0, 0, 1));

	// Release resources.
	SAFE_RELEASE(errorRTV);
	SAFE_RELEASE(errorTex);
	SAFE_RELEASE(errorTexCopy);
	SAFE_RELEASE(uncompRes);
	SAFE_RELEASE(compRes);
	SAFE_RELEASE(GPUQuery);

#ifdef _DEBUG
	SAFE_RELEASE(OcclusionQuery);
	SAFE_RELEASE(StatsQuery);
#endif

	SAFE_RELEASE(compSRV);
	SAFE_RELEASE(uncompSRV);
	SAFE_RELEASE(compTex);
	SAFE_RELEASE(uncompTex);

	return S_OK;
}

// Recompresses the already loaded texture and recomputes the error.
HRESULT RecompressTexture()
{
	// Destroy any previously created textures.
	SAFE_RELEASE(gErrorSRV);
	SAFE_RELEASE(gCompressedSRV);

	// Compress the texture.
	HRESULT hr;
	V_RETURN(CompressTexture(gUncompressedSRV, &gCompressedSRV));

	// Compute the error in the compressed texture.
	V_RETURN(ComputeError(gUncompressedSRV, gCompressedSRV, &gErrorSRV));

	return S_OK;
}

// Store the depth-stencil state.
void StoreDepthStencilState()
{
	DXUTGetD3D11DeviceContext()->OMGetDepthStencilState(&gDepthStencilState, &gStencilReference);
}

// Restore the depth-stencil state.
void RestoreDepthStencilState()
{
	DXUTGetD3D11DeviceContext()->OMSetDepthStencilState(gDepthStencilState, gStencilReference);
}

// Disable depth testing.
HRESULT DisableDepthTest()
{
	D3D11_DEPTH_STENCIL_DESC depStenDesc;
	ZeroMemory(&depStenDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depStenDesc.DepthEnable = FALSE;
	depStenDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depStenDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depStenDesc.StencilEnable = FALSE;
	depStenDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depStenDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	depStenDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depStenDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depStenDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depStenDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depStenDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depStenDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depStenDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depStenDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	ID3D11DepthStencilState* depStenState;
	HRESULT hr;
	V_RETURN(DXUTGetD3D11Device()->CreateDepthStencilState(&depStenDesc, &depStenState));

    DXUTGetD3D11DeviceContext()->OMSetDepthStencilState(depStenState, 0);

	SAFE_RELEASE(depStenState);

	return S_OK;
}

void ComputeRMSE(const BYTE *errorData, const INT width, const INT height) {
	
	const float *w = BC7C::GetErrorMetric();

	const double wr = w[0];
	const double wg = w[1];
	const double wb = w[2];
	
	double MSE = 0.0;
	for(int i = 0; i < width; i++) {
		for(int j = 0; j < height; j++) {
			const INT pixel = ((const INT *)errorData)[j * width + i];

			double dr = double(pixel & 0xFF) * wr;
			double dg = double((pixel >> 8) & 0xFF) * wg;
			double db = double((pixel >> 16) & 0xFF) * wb;

			const double pixelMSE = (double(dr) * double(dr)) + (double(dg) * double(dg)) + (double(db) * double(db));
			MSE += pixelMSE;
		}
	}

	MSE /= (double(width) * double(height));
#ifdef REPORT_RMSE
	gError = sqrt(MSE);
#else
	double MAXI = (255.0 * wr) * (255.0 * wr) + (255.0 * wg) * (255.0 * wg) + (255.0 * wb) * (255.0 * wb);
	gError= 10 * log10(MAXI/MSE);
#endif
}

namespace DXTC
{
	VOID CompressImageDXT(const BYTE* inBuf, BYTE* outBuf, INT width, INT height) {

		// If we aren't multi-cored, then just run everything serially.
		if(gNumProcessors <= 1) {
			CompressImageDXTNoThread(inBuf, outBuf, width, height);
			return;
		}

		switch(gCompressionScheme->threadMode) {
			case eThreadMode_None:
				CompressImageDXTNoThread(inBuf, outBuf, width, height);
				break;
			case eThreadMode_TBB:
				CompressImageDXTTBB(inBuf, outBuf, width, height);
				break;
			case eThreadMode_Win32:
				CompressImageDXTWIN(inBuf, outBuf, width, height);
				break;
		}
	}

	CompressionFunc GetCompressionFunc() {
		switch(gCompressionScheme->instrSet)
		{
			case eInstrSet_SSE: 
			{
				switch(gCompressionScheme->type) {
					case eCompType_DXT1: return DXTC::CompressImageDXT1SSE2;
					case eCompType_DXT5: return DXTC::CompressImageDXT5SSE2;
					case eCompType_BC7: return BC7C::CompressImageBC7SIMD;
				}
			}
			break;

			case eInstrSet_Scalar:
			{
				switch(gCompressionScheme->type) {
					case eCompType_DXT1: return DXTC::CompressImageDXT1;
					case eCompType_DXT5: return DXTC::CompressImageDXT5;
					case eCompType_BC7: return BC7C::CompressImageBC7;
				}
			}
			break;

#ifdef ENABLE_AVX2
			case eInstrSet_AVX2:
			{
				switch(gCompressionScheme->type) {
					case eCompType_DXT1: return DXTC::CompressImageDXT1AVX2;
					case eCompType_DXT5: return DXTC::CompressImageDXT5AVX2;
				}
			}
#endif
		}
		return NULL;
	}

	void CompressImageDXTNoThread(const BYTE* inBuf, BYTE* outBuf, INT width, INT height) {

		CompressionFunc cmpFunc = GetCompressionFunc();

		if(cmpFunc == NULL) {
			OutputDebugString(L"DXTC::CompressImageDXTNoThread -- Compression Scheme not implemented!\n");
			return;
		}

		// Do the compression.
		(*cmpFunc)(inBuf, outBuf, width, height);
	}

	// Use the TBB task manager to compress an image with DXT compression.
	VOID CompressImageDXTTBB(const BYTE* inBuf, BYTE* outBuf, INT width, INT height)
	{
		// Initialize the data.
		DXTTaskData data;
		data.inBuf = inBuf;
		data.outBuf = outBuf;
		data.width = width;
		data.height = height;
		data.numBlocks = width * height / 16;
		if(gCompressionScheme->instrSet == eInstrSet_AVX2) {
			data.numBlocks = width * height / 32;
		}
		data.kBlocksPerTask = gBlocksPerTask;

		// Compute the task count.
		UINT taskCount = (UINT)ceil((float)data.numBlocks / gBlocksPerTask);

		// Create the task set.
		TASKSETFUNC taskFunc = NULL;
		switch(gCompressionScheme->instrSet)
		{
			case eInstrSet_SSE:
			{
				switch(gCompressionScheme->type) {
					case eCompType_DXT1: taskFunc = DXTC::CompressImageDXT1SSE2Task; break;
					case eCompType_DXT5: taskFunc = DXTC::CompressImageDXT5SSE2Task; break;
				}
			}
			break;

			case eInstrSet_Scalar:
			{
				switch(gCompressionScheme->type) {
					case eCompType_DXT1: taskFunc = DXTC::CompressImageDXT1Task; break;
					case eCompType_DXT5: taskFunc = DXTC::CompressImageDXT5Task; break;
				}
			}
			break;

#ifdef ENABLE_AVX2
			case eInstrSet_AVX2:
			{
				switch(gCompressionScheme->type) {
					case eCompType_DXT1: taskFunc = DXTC::CompressImageDXT1AVX2Task; break;
					case eCompType_DXT5: taskFunc = DXTC::CompressImageDXT5AVX2Task; break;
				}
			}
			break;
#endif
		}

		TASKSETHANDLE taskSet;
		gTaskMgr.CreateTaskSet(taskFunc, &data, taskCount, NULL, 0, "Fast Texture Compression", &taskSet);
		if(taskSet == TASKSETHANDLE_INVALID)
		{
			return;
		}

		// Wait for the task set.
		gTaskMgr.WaitForSet(taskSet);

		// Release the task set.
		gTaskMgr.ReleaseHandle(taskSet);
		taskSet = TASKSETHANDLE_INVALID;
	}

	int GetBlocksPerLoop() {
		if(gCompressionScheme->instrSet == eInstrSet_AVX2)
			return 2;
		return 1;
	}

	int GetBytesPerBlock() {
		switch(gCompressionScheme->type) {
			default:
			case eCompType_DXT1:
				return 8;
				
			case eCompType_DXT5:
			case eCompType_BC7:
				return 16;
		}
	}

	VOID CompressImageDXTWIN(const BYTE* inBuf, BYTE* outBuf, INT width, INT height) {

		const int numThreads = gNumWinThreads;
		const int blocksPerLoop = GetBlocksPerLoop();
		const int bytesPerBlock = GetBytesPerBlock();

		// We want to split the data evenly among all threads.
		const int kNumPixels = width * height;
		const int kNumBlocks = kNumPixels >> (3 + blocksPerLoop);
		const int kBlocksPerRow = width >> (1 + blocksPerLoop);

		const int kBlocksPerThread = kNumBlocks / numThreads;
		const int kBlocksPerColumn = height >> 2;
		const int kBlockRowsPerThread = kBlocksPerThread / kBlocksPerRow;
		const int kBlockColsPerThread = kBlocksPerThread % kBlocksPerRow;
		const int kOffsetPerThread = kBlockRowsPerThread * width * 4 * 4 + kBlockColsPerThread * 4 * 4 * (blocksPerLoop);
		const int kHeightPerThread = (blocksPerLoop * 16 * kBlocksPerThread) / width;

		CompressionFunc cmpFunc = GetCompressionFunc();
		if(cmpFunc == NULL) {
			OutputDebugString(L"DXTC::CompressImageDXTNoThread -- Compression Scheme not implemented!\n");
			return;
		}

		// Load the threads.
		for(int threadIdx = 0; threadIdx < numThreads; threadIdx++) {

			WinThreadData *data = &gWinThreadData[threadIdx];
			data->inBuf = inBuf + (threadIdx * kOffsetPerThread);
			data->outBuf = outBuf + (threadIdx * kBlocksPerThread * blocksPerLoop * bytesPerBlock);
			data->width = width;
			data->height = kHeightPerThread;
			data->cmpFunc = cmpFunc;
			data->state = eThreadState_DataLoaded;
			data->threadIdx = threadIdx;
		}

		// Send the event for the threads to start.
		CHECK_WIN_THREAD_FUNC(ResetEvent(gWinThreadDoneEvent));
		CHECK_WIN_THREAD_FUNC(SetEvent(gWinThreadStartEvent));

		// Wait for all the threads to finish
		if(WAIT_FAILED == WaitForMultipleObjects(numThreads, gWinThreadWorkEvent, TRUE, INFINITE))
				ReportWinThreadError(TEXT("CompressImageDXTWIN -- WaitForMultipleObjects"));

		// Reset the start event
		CHECK_WIN_THREAD_FUNC(ResetEvent(gWinThreadStartEvent));
		CHECK_WIN_THREAD_FUNC(SetEvent(gWinThreadDoneEvent));
	}

	DWORD WINAPI CompressImageDXTWinThread( LPVOID lpParam ) {
		WinThreadData *data = (WinThreadData *)lpParam;

		while(data->state != eThreadState_Done) {

			if(WAIT_FAILED == WaitForSingleObject(gWinThreadStartEvent, INFINITE))
				ReportWinThreadError(TEXT("CompressImageDXTWinThread -- WaitForSingleObject"));

			if(data->state == eThreadState_Done)
				break;

			data->state = eThreadState_Running;
			(*(data->cmpFunc))(data->inBuf, data->outBuf, data->width, data->height);

			data->state = eThreadState_WaitForData;

			HANDLE workEvent = gWinThreadWorkEvent[data->threadIdx];
			if(WAIT_FAILED == SignalObjectAndWait(workEvent, gWinThreadDoneEvent, INFINITE, FALSE))
				ReportWinThreadError(TEXT("CompressImageDXTWinThread -- SignalObjectAndWait"));
		}

		return 0;
	}
}
