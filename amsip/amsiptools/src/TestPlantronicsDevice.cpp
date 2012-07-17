// TestDevice.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "DeviceEventHandler.h"

[module(name="TestDevice")];

void DoTest();
unsigned int WINAPI InputProcessingThread(void* pArgument);
void PrintHelp();
void ProcessInput(int c, IDevicePtr spDevice);

typedef CComGITPtr<IDevice> IDeviceGITPtr;

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef _DEBUG
	::AtlTraceLoadSettings(NULL);
#endif

	// Initialize COM runtime
	::CoInitialize(NULL);

    try
    {
        DoTest();
    }
    catch(const _com_error& ex)
    {
        std::cerr << "Exception: " << ex.Description() << std::endl;
		ATLASSERT(false);
    }
    catch(...)
    {
        std::cerr << "Unknown Exception" << std::endl;
		ATLASSERT(false);
    }
}

void DoTest()
{
	IDevicePtr spDevice;
	HRESULT hr = spDevice.CreateInstance(__uuidof(CDevice));
	assert( SUCCEEDED(hr) );

	DeviceEventHandler eventHander;
	eventHander.HookEvents((IUnknown*)spDevice);

	spDevice->StartupPNP();

	// We need the message pump to marshal USB HID input
	// events from the read thread into the main thread.
	// Because this test is a console application, we need
	// to implement the message pump. At the same time, we 
	// have to accept input from the keyboard.
	// The keyboard input is handled in a separate thread,
	// the message pump is implemented in the main thread.

	// Pointers to COM interfaces has to be marshalled
	// to another thread using GIT
	IDeviceGITPtr spgitDevice(spDevice);

	// Start the input thread
	unsigned int threadID;
	// create the thread
	HANDLE hThread = (HANDLE) _beginthreadex(
							NULL /*security*/,
							0 /*stack_size*/,
							&InputProcessingThread/*start_address*/,
							reinterpret_cast<void*>((long long)spgitDevice.GetCookie()) /*arglist*/,
							0 /*initflag - running*/,
							&threadID /*thrdaddr*/ );
	ATLASSERT(NULL != hThread);

	// Message loop continues until the input thread terminates
	while(::MsgWaitForMultipleObjects( 1, &hThread, false,
		INFINITE, QS_ALLINPUT ) != WAIT_OBJECT_0)
	{
		MSG msg;
		while( ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	::CloseHandle(hThread);

	// Unsubscribe the events
	eventHander.UnhookEvents();
	// Shutdown the PNP listener and detach from the device
	spDevice->ShutdownPNP();
}

unsigned int WINAPI InputProcessingThread(void* pArgument)
{
	::CoInitialize(NULL);

	DWORD cookie = (DWORD) reinterpret_cast<long long>(pArgument);
	IDeviceGITPtr spgitDevice(cookie);

    IDevicePtr spDevice; 
	spgitDevice.CopyTo(&spDevice);

	std::cout << std::endl << "Listening for input reports" << std::endl;
	PrintHelp();

	while(true)
	{
		try
		{
			int c = getch();
			if( c == 27 /*ESC*/ )
			{
				break;
			}
			ProcessInput(c, spDevice);
		}
		catch(const _com_error& ex)
		{
			std::cerr << "Exception: " << ex.Description() << std::endl;
		}
		catch(...)
		{
			std::cerr << "Unknown Exception" << std::endl;
		}
	}
	std::cout << "Done listening!" << std::endl;

	return 0;
}

void ProcessInput(int c, IDevicePtr spDevice)
{
	switch(c)
	{
	case 'R':
		std::cout << std::endl << "Setting Ringer=true" << std::endl;
		spDevice->Ringer = true;
		break;
	case 'r':
		std::cout << std::endl << "Setting Ringer=false" << std::endl;
		spDevice->Ringer = false;
		break;
	case 'A':
		std::cout << std::endl << "Setting AudioEnabled=true" << std::endl;
		try
		{
			spDevice->AudioEnabled = true;
		}
		catch(_com_error e)
		{
			std::cout << "Setting AudioEnabled=true failed" << std::endl;
		}
		break;
	case 'a':
		std::cout << std::endl << "Setting AudioEnabled=false" << std::endl;
		spDevice->AudioEnabled = false;
		break;
	case 's':
	case 'S':
		{
			bool bAttached = (spDevice->IsAttached != VARIANT_FALSE);
			std::cout << std::endl << "Attached=" << (bAttached ? "true" : "false") << std::endl;
			if( bAttached )
			{
				bool bAudioEnabled = (spDevice->AudioEnabled != VARIANT_FALSE);
				std::cout << "AudioEnabled=" << (bAudioEnabled ? "true" : "false") << std::endl;

				bool bMute = (spDevice->Mute != VARIANT_FALSE);
				std::cout << "Mute=" << (bMute ? "on" : "off") << std::endl;
			}
		}
		break;
	default:
		PrintHelp();
	}
}
	
void PrintHelp()
{
	std::cout << std::endl << "R - ringer on" << std::endl
		<< "r   - ringer off" << std::endl
		<< "A   - audio link on" << std::endl
		<< "s   - print status" << std::endl
		<< "ESC - exit" << std::endl;
}