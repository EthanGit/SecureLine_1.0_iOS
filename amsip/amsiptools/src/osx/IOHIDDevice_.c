//
//	File:		IOHIDDevice_.c of HID Utilities
// 
//	Created:	9/21/06
//
//	Contains:	convieance functions for IOHIDDeviceGetProperty
//
//	Copyright:  Copyright (c) 2007 Apple Inc., All Rights Reserved
//
//	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by 
//				Apple Inc. ("Apple") in consideration of your agreement to the
//				following terms, and your use, installation, modification or
//				redistribution of this Apple software constitutes acceptance of these
//				terms.  If you do not agree with these terms, please do not use,
//				install, modify or redistribute this Apple software.
//
//				In consideration of your agreement to abide by the following terms, and
//				subject to these terms, Apple grants you a personal, non-exclusive
//				license, under Apple's copyrights in this original Apple software (the
//				"Apple Software"), to use, reproduce, modify and redistribute the Apple
//				Software, with or without modifications, in source and/or binary forms;
//				provided that if you redistribute the Apple Software in its entirety and
//				without modifications, you must retain this notice and the following
//				text and disclaimers in all such redistributions of the Apple Software. 
//				Neither the name, trademarks, service marks or logos of Apple Inc. 
//				may be used to endorse or promote products derived from the Apple
//				Software without specific prior written permission from Apple.  Except
//				as expressly stated in this notice, no other rights or licenses, express
//				or implied, are granted by Apple herein, including but not limited to
//				any patent rights that may be infringed by your derivative works or by
//				other works in which the Apple Software may be incorporated.
//
//				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
//				MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
//				THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
//				FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
//				OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
//
//				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
//				OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//				SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//				INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
//				MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
//				AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
//				STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
//				POSSIBILITY OF SUCH DAMAGE.
//

//*****************************************************
#pragma mark - includes & imports
//-----------------------------------------------------

#include "IOHIDDevice_.h"

//*****************************************************
#pragma mark - typedef's, struct's, enums, defines, etc.
//-----------------------------------------------------

#define kIOHIDDevice_TransactionKey	"DeviceTransaction"

//*****************************************************
#pragma mark - local (static) function prototypes
//-----------------------------------------------------

static Boolean IOHIDDevice_GetLongProperty( IOHIDDeviceRef inIOHIDDeviceRef, CFStringRef inKey, long * outValue );
static void IOHIDDevice_SetLongProperty( IOHIDDeviceRef inIOHIDDeviceRef, CFStringRef inKey, long inValue );

//*****************************************************
#pragma mark - exported globals
//-----------------------------------------------------

//*****************************************************
#pragma mark - local (static) globals
//-----------------------------------------------------

//*****************************************************
#pragma mark - exported function implementations
//-----------------------------------------------------
//*************************************************************************
//
// IOHIDDevice_GetTransport( inIOHIDDeviceRef )
//
// Purpose:	get the Transport CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the Transport CFString for this device
//

CFStringRef IOHIDDevice_GetTransport( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDTransportKey ) );
}

//*************************************************************************
//
// IOHIDDevice_GetVendorID( inIOHIDDeviceRef )
//
// Purpose:	get the vendor ID for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	long - the vendor ID for this device
//

long IOHIDDevice_GetVendorID( IOHIDDeviceRef inIOHIDDeviceRef )
{
	long result = 0;
	( void ) IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDVendorIDKey ), &result );
	return result;
} // IOHIDDevice_GetVendorID


//*************************************************************************
//
// IOHIDDevice_GetVendorIDSource( inIOHIDDeviceRef )
//
// Purpose:	get the VendorIDSource CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the VendorIDSource CFString for this device
//

CFStringRef IOHIDDevice_GetVendorIDSource( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDVendorIDSourceKey ) );
}

//*************************************************************************
//
// IOHIDDevice_GetProductID( inIOHIDDeviceRef )
//
// Purpose:	get the product ID for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	long - the product ID for this device
//

long IOHIDDevice_GetProductID( IOHIDDeviceRef inIOHIDDeviceRef )
{
	long result = 0;
	( void ) IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDProductIDKey ), &result );
	return result;
} // IOHIDDevice_GetProductID

//*************************************************************************
//
// IOHIDDevice_GetVersionNumber( inIOHIDDeviceRef )
//
// Purpose:	get the VersionNumber CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the VersionNumber CFString for this device
//

long IOHIDDevice_GetVersionNumber( IOHIDDeviceRef inIOHIDDeviceRef )
{
	long result = 0;
	( void ) IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDVersionNumberKey ), &result );
	return result;
}

//*************************************************************************
//
// IOHIDDevice_GetManufacturer( inIOHIDDeviceRef )
//
// Purpose:	get the Manufacturer CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the Manufacturer CFString for this device
//

CFStringRef IOHIDDevice_GetManufacturer( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDManufacturerKey ) );
} // IOHIDDevice_GetManufacturer

//*************************************************************************
//
// IOHIDDevice_GetProduct( inIOHIDDeviceRef )
//
// Purpose:	get the Product CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the Product CFString for this device
//

CFStringRef IOHIDDevice_GetProduct( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDProductKey ) );
} // IOHIDDevice_GetProduct

//*************************************************************************
//
// IOHIDDevice_GetSerialNumber( inIOHIDDeviceRef )
//
// Purpose:	get the SerialNumber CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the SerialNumber CFString for this device
//

CFStringRef IOHIDDevice_GetSerialNumber( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDSerialNumberKey ) );
}

//*************************************************************************
//
// IOHIDDevice_GetCountryCode( inIOHIDDeviceRef )
//
// Purpose:	get the CountryCode CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the CountryCode CFString for this device
//

CFStringRef IOHIDDevice_GetCountryCode( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDCountryCodeKey ) );
}

//*************************************************************************
//
// IOHIDDevice_GetLocationID( inIOHIDDeviceRef )
//
// Purpose:	get the location ID for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	long - the location ID for this device
//

long IOHIDDevice_GetLocationID( IOHIDDeviceRef inIOHIDDeviceRef )
{
	long result = 0;
	( void ) IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDLocationIDKey ), &result );
	return result;
}	// IOHIDDevice_GetLocationID

//*************************************************************************
//
// IOHIDDevice_GetUsage( inIOHIDDeviceRef )
//
// Purpose:	get the usage for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	uint32_t - the usage for this device
//

uint32_t IOHIDDevice_GetUsage( IOHIDDeviceRef inIOHIDDeviceRef )
{
	uint32_t result = 0;
	( void ) IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDDeviceUsageKey ), ( long * ) &result );
	return result;
} // IOHIDDevice_GetUsage

//*************************************************************************
//
// IOHIDDevice_GetUsagePage( inIOHIDDeviceRef )
//
// Purpose:	get the usage page for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	uint32_t - the usage page for this device
//

uint32_t IOHIDDevice_GetUsagePage( IOHIDDeviceRef inIOHIDDeviceRef )
{
	long result = 0;
	( void ) IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDDeviceUsagePageKey ), &result );
	return (uint32_t)result;
}	// IOHIDDevice_GetUsagePage

//*************************************************************************
//
// IOHIDDevice_GetUsagePairs( inIOHIDDeviceRef )
//
// Purpose:	get the UsagePairs CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFArrayRef - the UsagePairs for this device
//

CFArrayRef IOHIDDevice_GetUsagePairs( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDDeviceUsagePairsKey ) );
}

//*************************************************************************
//
// IOHIDDevice_GetPrimaryUsage( inIOHIDDeviceRef )
//
// Purpose:	get the PrimaryUsage CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the PrimaryUsage CFString for this device
//

uint32_t IOHIDDevice_GetPrimaryUsage( IOHIDDeviceRef inIOHIDDeviceRef )
{
	long result = 0;
	( void ) IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDPrimaryUsageKey ), &result );
	return (uint32_t)result;
}

//*************************************************************************
//
// IOHIDDevice_GetPrimaryUsagePage( inIOHIDDeviceRef )
//
// Purpose:	get the PrimaryUsagePage CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the PrimaryUsagePage CFString for this device
//

uint32_t IOHIDDevice_GetPrimaryUsagePage( IOHIDDeviceRef inIOHIDDeviceRef )
{
	long result = 0;
	( void ) IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDPrimaryUsagePageKey ), &result );
	return (uint32_t)result;
}

//*************************************************************************
//
// IOHIDDevice_GetMaxInputReportSize( inIOHIDDeviceRef )
//
// Purpose:	get the MaxInputReportSize CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the MaxInputReportSize CFString for this device
//

CFStringRef IOHIDDevice_GetMaxInputReportSize( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDMaxInputReportSizeKey ) );
}

//*************************************************************************
//
// IOHIDDevice_GetMaxOutputReportSize( inIOHIDDeviceRef )
//
// Purpose:	get the MaxOutputReportSize CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the MaxOutputReportSize CFString for this device
//

CFStringRef IOHIDDevice_GetMaxOutputReportSize( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDMaxOutputReportSizeKey ) );
}

//*************************************************************************
//
// IOHIDDevice_GetMaxFeatureReportSize( inIOHIDDeviceRef )
//
// Purpose:	get the MaxFeatureReportSize CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the MaxFeatureReportSize CFString for this device
//

CFStringRef IOHIDDevice_GetMaxFeatureReportSize( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDMaxFeatureReportSizeKey ) );
}

//*************************************************************************
//
// IOHIDDevice_GetReportInterval( inIOHIDDeviceRef )
//
// Purpose:	get the ReportInterval CFString for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	CFStringRef - the ReportInterval CFString for this device
//
#ifndef kIOHIDReportIntervalKey
#define kIOHIDReportIntervalKey "ReportInterval"
#endif
CFStringRef IOHIDDevice_GetReportInterval( IOHIDDeviceRef inIOHIDDeviceRef )
{
	return IOHIDDeviceGetProperty( inIOHIDDeviceRef, CFSTR( kIOHIDReportIntervalKey ) );
}

//*************************************************************************
//
// IOHIDDevice_GetTransaction( inIOHIDDeviceRef )
//
// Purpose:	get the Transaction for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//
// Returns:	IOHIDTransactionRef - the Transaction for this device
//

IOHIDTransactionRef IOHIDDevice_GetTransaction( IOHIDDeviceRef inIOHIDDeviceRef )
{
	IOHIDTransactionRef result = 0;
	( void ) IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDDevice_TransactionKey ), ( long * ) &result );
	return result;
}

//*************************************************************************
//
// IOHIDDevice_SetTransaction( inIOHIDDeviceRef, inTransactionRef )
//
// Purpose:	Set the Transaction for this device
//
// Inputs:  inIOHIDDeviceRef - the IDHIDDeviceRef for this device
//			inTransactionRef - the Transaction reference
//
// Returns:	nothing
//

void IOHIDDevice_SetTransaction( IOHIDDeviceRef inIOHIDDeviceRef, IOHIDTransactionRef inTransactionRef )
{
	IOHIDDevice_SetLongProperty( inIOHIDDeviceRef, CFSTR( kIOHIDDevice_TransactionKey ), ( long ) inTransactionRef );
}

//*****************************************************
#pragma mark - local (static) function implementations
//-----------------------------------------------------

//*************************************************************************
//
// IOHIDDevice_GetLongProperty( inIOHIDDeviceRef, inKey, outValue )
//
// Purpose:	convieance function to return a long property of a device
//
// Inputs:	inIOHIDDeviceRef		- the device
//			inKey			- CFString for the 
//			outValue		- address where to restore the element
// Returns:	the action cookie
//			outValue		- the device
//

static Boolean IOHIDDevice_GetLongProperty( IOHIDDeviceRef inIOHIDDeviceRef, CFStringRef inKey, long * outValue )
{
	Boolean result = FALSE;
	
	if ( inIOHIDDeviceRef ) {
		CFTypeRef tCFTypeRef = IOHIDDeviceGetProperty( inIOHIDDeviceRef, inKey );
		if ( tCFTypeRef ) {
			// if this is a number
			if ( CFNumberGetTypeID() == CFGetTypeID( tCFTypeRef ) ) {
				// get it's value
				result = CFNumberGetValue( ( CFNumberRef ) tCFTypeRef, kCFNumberSInt32Type, outValue );
			}
		}
	}
	return result;
}	// IOHIDDevice_GetLongProperty

//*************************************************************************
//
// IOHIDDevice_SetLongProperty( inIOHIDDeviceRef, inKey, inValue )
//
// Purpose:	convieance function to set a long property of an Device
//
// Inputs:	inIOHIDDeviceRef	- the Device
//			inKey			- CFString for the key
//			inValue			- the value to set it to
// Returns:	nothing
//

static void IOHIDDevice_SetLongProperty( IOHIDDeviceRef inIOHIDDeviceRef, CFStringRef inKey, long inValue )
{
	CFNumberRef tCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type, &inValue );
	if ( tCFNumberRef ) {
		IOHIDDeviceSetProperty( inIOHIDDeviceRef, inKey, tCFNumberRef );
		CFRelease( tCFNumberRef );
	}
}	// IOHIDDevice_SetLongProperty

//*****************************************************
