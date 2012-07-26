#define UNICODE

#include <amsiptools/hardware_guid.h>

#ifdef WIN32

#include <mediastreamer2/mscommon.h>

#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>

# pragma comment(lib, "wbemuuid.lib")

#include <stdarg.h>
#define VA_START(a, f)  va_start(a, f)

#if 1

#define  MAX_IDE_DRIVES  1

typedef struct _GETVERSIONOUTPARAMS
{
   BYTE bVersion;      // Binary driver version.
   BYTE bRevision;     // Binary driver revision.
   BYTE bReserved;     // Not used.
   BYTE bIDEDeviceMap; // Bit map of IDE devices.
   DWORD fCapabilities; // Bit mask of driver capabilities.
   DWORD dwReserved[4]; // For future use.
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;

   //  IOCTL commands
#define  DFP_GET_VERSION          0x00074080
#define  DFP_SEND_DRIVE_COMMAND   0x0007c084
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

   //  Valid values for the bCommandReg member of IDEREGS.
#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.


void MyOutputDebugString(char *chfr, ...)
{
	WCHAR wUnicode[1024];
	va_list ap;
	char buffer[512];

	VA_START(ap, chfr);

	memset(buffer, 0, sizeof(buffer));
	_vsnprintf(buffer, sizeof(buffer), chfr, ap);

	MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wUnicode,
						1024);
	OutputDebugString(wUnicode);

	va_end(ap);
}

static BOOL DoIDENTIFY (HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP,
                 PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum,
                 PDWORD lpcbBytesReturned)
{
      // Set up data structures for IDENTIFY command.
   pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
   pSCIP -> irDriveRegs.bFeaturesReg = 0;
   pSCIP -> irDriveRegs.bSectorCountReg = 1;
   pSCIP -> irDriveRegs.bSectorNumberReg = 1;
   pSCIP -> irDriveRegs.bCylLowReg = 0;
   pSCIP -> irDriveRegs.bCylHighReg = 0;

      // Compute the drive number.
   pSCIP -> irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);

      // The command can either be IDE identify or ATAPI identify.
   pSCIP -> irDriveRegs.bCommandReg = bIDCmd;
   pSCIP -> bDriveNumber = bDriveNum;
   pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;

   return ( DeviceIoControl (hPhysicalDriveIOCTL, DFP_RECEIVE_DRIVE_DATA,
               (LPVOID) pSCIP,
               sizeof(SENDCMDINPARAMS) - 1,
               (LPVOID) pSCOP,
               sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
               lpcbBytesReturned, NULL) );
}

static int ReadPhysicalDriveInNTWithAdminRights (hdd_guid_t *hg)
{
   int done = FALSE;
   int drive = 0;
   BYTE IdOutCmd [sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
   for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
   {
      HANDLE hPhysicalDriveIOCTL = 0;

         //  Try to get a handle to PhysicalDrive IOCTL, report failure
         //  and exit if can't.
      TCHAR driveName [256];

      swprintf (driveName, sizeof(driveName)-1, L"\\\\.\\PhysicalDrive%d", drive);

         //  Windows NT, Windows 2000, must have admin rights
      hPhysicalDriveIOCTL = CreateFile (driveName,
                               GENERIC_READ | GENERIC_WRITE, 
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, 0, NULL);

      if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
      {
         GETVERSIONOUTPARAMS VersionParams;
         DWORD               cbBytesReturned = 0;

            // Get the version, etc of PhysicalDrive IOCTL
         memset ((void*) &VersionParams, 0, sizeof(VersionParams));

         if (  DeviceIoControl (hPhysicalDriveIOCTL, DFP_GET_VERSION,
                   NULL, 
                   0,
                   &VersionParams,
                   sizeof(VersionParams),
                   &cbBytesReturned, NULL) )
         {         


				// If there is a IDE device at number "i" issue commands
				// to the device
			 if (VersionParams.bIDEDeviceMap > 0)
			 {
				BYTE             bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
				SENDCMDINPARAMS  scip;

				// Now, get the ID sector for all IDE devices in the system.
				   // If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
				   // otherwise use the IDE_ATA_IDENTIFY command
				bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? \
						  IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;

				memset (&scip, 0, sizeof(scip));
				memset (IdOutCmd, 0, sizeof(IdOutCmd));

				if ( DoIDENTIFY (hPhysicalDriveIOCTL, 
						   &scip, 
						   (PSENDCMDOUTPARAMS)&IdOutCmd, 
						   (BYTE) bIDCmd,
						   (BYTE) drive,
						   &cbBytesReturned))
				{
	              
		
				   USHORT *pIdSector = (USHORT *)
								 ((PSENDCMDOUTPARAMS) IdOutCmd) -> bBuffer;
				   //AddIfNew(pIdSector);	  
				   done = TRUE;
				}
			}
	    }

         CloseHandle (hPhysicalDriveIOCTL);
      }
   }

   return done;
}

typedef struct _SRB_IO_CONTROL
{
   ULONG HeaderLength;
   UCHAR Signature[8];
   ULONG Timeout;
   ULONG ControlCode;
   ULONG ReturnCode;
   ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

#define  SENDIDLENGTH  sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE

#define  FILE_DEVICE_SCSI              0x0000001b
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define  IOCTL_SCSI_MINIPORT 0x0004D008  //  see NTDDSCSI.H for definition

   // The following struct defines the interesting part of the IDENTIFY
   // buffer:
typedef struct _IDSECTOR
{
   USHORT  wGenConfig;
   USHORT  wNumCyls;
   USHORT  wReserved;
   USHORT  wNumHeads;
   USHORT  wBytesPerTrack;
   USHORT  wBytesPerSector;
   USHORT  wSectorsPerTrack;
   USHORT  wVendorUnique[3];
   CHAR    sSerialNumber[20];
   USHORT  wBufferType;
   USHORT  wBufferSize;
   USHORT  wECCSize;
   CHAR    sFirmwareRev[8];
   CHAR    sModelNumber[40];
   USHORT  wMoreVendorUnique;
   USHORT  wDoubleWordIO;
   USHORT  wCapabilities;
   USHORT  wReserved1;
   USHORT  wPIOTiming;
   USHORT  wDMATiming;
   USHORT  wBS;
   USHORT  wNumCurrentCyls;
   USHORT  wNumCurrentHeads;
   USHORT  wNumCurrentSectorsPerTrack;
   ULONG   ulCurrentSectorCapacity;
   USHORT  wMultSectorStuff;
   ULONG   ulTotalAddressableSectors;
   USHORT  wSingleWordDMA;
   USHORT  wMultiWordDMA;
   BYTE    bReserved[128];
} IDSECTOR, *PIDSECTOR;

static int ReadIdeDriveAsScsiDriveInNT (hdd_guid_t *hg)
{
   int done = FALSE;
   int controller = 0;

   for (controller = 0; controller < 16; controller++)
   {
      HANDLE hScsiDriveIOCTL = 0;
      TCHAR   driveName [256];

         //  Try to get a handle to PhysicalDrive IOCTL, report failure
         //  and exit if can't.
      swprintf (driveName, sizeof(driveName)-1, L"\\\\.\\Scsi%d:", controller);

         //  Windows NT, Windows 2000, any rights should do
      hScsiDriveIOCTL = CreateFile (driveName,
                               GENERIC_READ | GENERIC_WRITE, 
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, 0, NULL);


      if (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
      {
         int drive = 0;

         for (drive = 0; drive < 2; drive++)
         {
            char buffer [sizeof (SRB_IO_CONTROL) + SENDIDLENGTH];
            SRB_IO_CONTROL *p = (SRB_IO_CONTROL *) buffer;
            SENDCMDINPARAMS *pin =
                   (SENDCMDINPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
            DWORD dummy;
   
            memset (buffer, 0, sizeof (buffer));
            p -> HeaderLength = sizeof (SRB_IO_CONTROL);
            p -> Timeout = 10000;
            p -> Length = SENDIDLENGTH;
            p -> ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
            strncpy ((char *) p -> Signature, "SCSIDISK", 8);
  
            pin -> irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
            pin -> bDriveNumber = drive;

            if (DeviceIoControl (hScsiDriveIOCTL, IOCTL_SCSI_MINIPORT, 
                                 buffer,
                                 sizeof (SRB_IO_CONTROL) +
                                         sizeof (SENDCMDINPARAMS) - 1,
                                 buffer,
                                 sizeof (SRB_IO_CONTROL) + SENDIDLENGTH,
                                 &dummy, NULL))
            {
               SENDCMDOUTPARAMS *pOut =
                    (SENDCMDOUTPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
               IDSECTOR *pId = (IDSECTOR *) (pOut -> bBuffer);
               if (pId -> sModelNumber [0])
               {

                  USHORT *pIdSector = (USHORT *) pId;

				  snprintf(hg->volume_physicalname, sizeof(hg->volume_physicalname), "\\\\.\\Scsi%d:", controller);
				  snprintf(hg->volume_name, sizeof(pId->sModelNumber), "%s", pId->sModelNumber);
				  int i=0;
				  int j=0;
				  while (i<sizeof(pId->sModelNumber))
				  {
					  char c = hg->volume_name[i+1];
					  hg->volume_name[i+1]=hg->volume_name[i];
					  hg->volume_name[i]=c;
					  i++;
					  i++;
					  if (hg->volume_name[i]=='\0')
						  break;
					  if (hg->volume_name[i+1]=='\0')
						  break;
				  }
				  i=0;
				  j=0;
				  while (i<20)
				  {
					  if (hg->volume_name[i]!=' ')
						  hg->volume_name[j++]=hg->volume_name[i];
					  if (hg->volume_name[i+1]!=' ')
						  hg->volume_name[j++]=hg->volume_name[i+1];

					  if (hg->volume_name[i]=='\0')
						  break;
					  if (hg->volume_name[i+1]=='\0')
						  break;
					  i++;
					  i++;
				  }
				  hg->volume_name[j]='\0';

				  snprintf(hg->volume_serial, sizeof(pId->sSerialNumber), "%s", pId->sSerialNumber);
				  i=0;
				  j=0;
				  while (i<sizeof(pId->sSerialNumber))
				  {
					  char c = hg->volume_serial[i+1];
					  hg->volume_serial[i+1]=hg->volume_serial[i];
					  hg->volume_serial[i]=c;
					  i++;
					  i++;
					  if (hg->volume_serial[i]=='\0')
						  break;
					  if (hg->volume_serial[i+1]=='\0')
						  break;
				  }
				  i=0;
				  j=0;
				  while (i<20)
				  {
					  if (hg->volume_serial[i]!=' ')
						  hg->volume_serial[j++]=hg->volume_serial[i];
					  if (hg->volume_serial[i+1]!=' ')
						  hg->volume_serial[j++]=hg->volume_serial[i+1];

					  if (hg->volume_serial[i]=='\0')
						  break;
					  if (hg->volume_serial[i+1]=='\0')
						  break;
					  i++;
					  i++;
				  }
				  hg->volume_serial[j]='\0';

				  GUID guid;
				  memset(&guid, 0, sizeof(guid));
				  memcpy(&guid, hg->volume_serial, sizeof(GUID));
				  wchar_t szGuid[40];
				  HRESULT hr = StringFromGUID2(guid, szGuid, 39);
				  if (SUCCEEDED(hr))
				  {
					  WideCharToMultiByte(CP_UTF8,0,szGuid,-1,hg->volume_guid,sizeof(szGuid),0,0);
				  }


				  //MyOutputDebugString("method 4 %s\n", hg->volume_physicalname);
				  //MyOutputDebugString("HDD name: %s\n", hg->volume_name);
				  //MyOutputDebugString("Hardware serial: %s\n", hg->volume_serial);
				  //MyOutputDebugString("GUID: %s\n",hg->volume_guid);
                  done = TRUE;
               }
            }
         }
         CloseHandle (hScsiDriveIOCTL);
      }
   }

   return done;
}


typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    USHORT FirmwareRevision[4];             // 2E  23-26
    USHORT ModelNumber[20];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A  53
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       //     59
    ULONG  UserAddressableSectors;          //     60-61
    USHORT SingleWordDMASupport : 8;        //     62
    USHORT SingleWordDMAActive : 8;
    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;
    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[2];                    //     69-70
    USHORT ReleaseTimeOverlapped;           //     71
    USHORT ReleaseTimeServiceCommand;       //     72
    USHORT MajorRevision;                   //     73
    USHORT MinorRevision;                   //     74
    USHORT Reserved6[50];                   //     75-126
    USHORT SpecialFunctionsEnabled;         //     127
    USHORT Reserved7[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

static int ReadPhysicalDriveInNTUsingSmart (hdd_guid_t *hg)
{
   int done = FALSE;
   int drive = 0;

   for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
   {
      HANDLE hPhysicalDriveIOCTL = 0;

         //  Try to get a handle to PhysicalDrive IOCTL, report failure
         //  and exit if can't.

	  TCHAR driveName [256];
      swprintf  (driveName, sizeof(driveName)-1, L"\\\\.\\PhysicalDrive%d", drive);

         //  Windows NT, Windows 2000, Windows Server 2003, Vista
      hPhysicalDriveIOCTL = CreateFile (driveName,
                               GENERIC_READ | GENERIC_WRITE, 
                               FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 
							   NULL, OPEN_EXISTING, 0, NULL);

      if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
      {

      }
      else
      {
         GETVERSIONINPARAMS GetVersionParams;
         DWORD cbBytesReturned = 0;

            // Get the version, etc of PhysicalDrive IOCTL
         memset ((void*) & GetVersionParams, 0, sizeof(GetVersionParams));

         if (  DeviceIoControl (hPhysicalDriveIOCTL, SMART_GET_VERSION,
                   NULL, 
                   0,
     			   &GetVersionParams, sizeof (GETVERSIONINPARAMS),
				   &cbBytesReturned, NULL) )
         {         
	           // Allocate the command buffer
			ULONG CommandSize = sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE;
        	PSENDCMDINPARAMS Command = (PSENDCMDINPARAMS) malloc (CommandSize);
	           // Retrieve the IDENTIFY data
	           // Prepare the command
#define ID_CMD          0xEC            // Returns ID sector for ATA
			Command -> irDriveRegs.bCommandReg = ID_CMD;
			DWORD BytesReturned = 0;
	        if ( DeviceIoControl (hPhysicalDriveIOCTL, 
				                    SMART_RCV_DRIVE_DATA, Command, sizeof(SENDCMDINPARAMS),
									Command, CommandSize,
									&BytesReturned, NULL) )

			{
                PIDENTIFY_DATA pIdSector = (PIDENTIFY_DATA) ((PSENDCMDOUTPARAMS) Command) -> bBuffer;

				snprintf(hg->volume_physicalname, sizeof(hg->volume_physicalname), "%s", "\\\\.\\PHYSICALDRIVE0");
				snprintf(hg->volume_name, sizeof(pIdSector->ModelNumber), "%s", pIdSector->ModelNumber);
				int i=0;
				int j=0;
				while (i<sizeof(pIdSector->ModelNumber))
				{
					char c = hg->volume_name[i+1];
					hg->volume_name[i+1]=hg->volume_name[i];
					hg->volume_name[i]=c;
					i++;
					i++;
					if (hg->volume_name[i]=='\0')
						break;
					if (hg->volume_name[i+1]=='\0')
						break;
				}
				i=0;
				j=0;
				while (i<20)
				{
					if (hg->volume_name[i]!=' ')
						hg->volume_name[j++]=hg->volume_name[i];
					if (hg->volume_name[i+1]!=' ')
						hg->volume_name[j++]=hg->volume_name[i+1];

					if (hg->volume_name[i]=='\0')
						break;
					if (hg->volume_name[i+1]=='\0')
						break;
					i++;
					i++;
				}
				hg->volume_name[j]='\0';

				snprintf(hg->volume_serial, sizeof(pIdSector->SerialNumber), "%s", pIdSector->SerialNumber);
				i=0;
				j=0;
				while (i<20)
				{
					char c = hg->volume_serial[i+1];
					hg->volume_serial[i+1]=hg->volume_serial[i];
					hg->volume_serial[i]=c;
					i++;
					i++;
					if (hg->volume_serial[i]=='\0')
						break;
					if (hg->volume_serial[i+1]=='\0')
						break;
				}
				i=0;
				j=0;
				while (i<20)
				{
					if (hg->volume_serial[i]!=' ')
						hg->volume_serial[j++]=hg->volume_serial[i];
					if (hg->volume_serial[i+1]!=' ')
						hg->volume_serial[j++]=hg->volume_serial[i+1];

					if (hg->volume_serial[i]=='\0')
						break;
					if (hg->volume_serial[i+1]=='\0')
						break;
					i++;
					i++;
				}
				hg->volume_serial[j]='\0';

				GUID guid;
				memset(&guid, 0, sizeof(guid));
				memcpy(&guid, hg->volume_serial, sizeof(GUID));
				wchar_t szGuid[40];
				HRESULT hr = StringFromGUID2(guid, szGuid, 39);
				if (SUCCEEDED(hr))
				{
					WideCharToMultiByte(CP_UTF8,0,szGuid,-1,hg->volume_guid,sizeof(szGuid),0,0);
				}
  
                done = TRUE;
			}
	           // Done
            CloseHandle (hPhysicalDriveIOCTL);
			free (Command);
		 }
      }
   }

   return done;
}

typedef struct _MEDIA_SERAL_NUMBER_DATA {
  ULONG  SerialNumberLength; 
  ULONG  Result;
  ULONG  Reserved[2];
  UCHAR  SerialNumberData[1];
} MEDIA_SERIAL_NUMBER_DATA, *PMEDIA_SERIAL_NUMBER_DATA;

	//  function to decode the serial numbers of IDE hard drives
	//  using the IOCTL_STORAGE_QUERY_PROPERTY command 
static char * flipAndCodeBytes (char * str)
{
	static char flipped [1000];
	int num = strlen (str);
	strcpy (flipped, "");
	for (int i = 0; i < num; i += 4)
	{
		for (int j = 1; j >= 0; j--)
		{
			int sum = 0;
			for (int k = 0; k < 2; k++)
			{
				sum *= 16;
				switch (str [i + j * 2 + k])
				{
				case '0': sum += 0; break;
				case '1': sum += 1; break;
				case '2': sum += 2; break;
				case '3': sum += 3; break;
				case '4': sum += 4; break;
				case '5': sum += 5; break;
				case '6': sum += 6; break;
				case '7': sum += 7; break;
				case '8': sum += 8; break;
				case '9': sum += 9; break;
				case 'a': sum += 10; break;
				case 'b': sum += 11; break;
				case 'c': sum += 12; break;
				case 'd': sum += 13; break;
				case 'e': sum += 14; break;
				case 'f': sum += 15; break;
				}
			}
			if (sum > 0) 
			{
				char sub [2];
				sub [0] = (char) sum;
				sub [1] = 0;
				strcat (flipped, sub);
			}
		}
	}

	return flipped;
}

static void dump_buffer (const char* title,
			const unsigned char* buffer,
			int len)
{
   int i = 0;
   int j;

   printf ("\n-- %s --\n", title);
   if (len > 0)
   {
      printf ("%8.8s ", " ");
      for (j = 0; j < 16; ++j)
      {
	    printf (" %2X", j);
      }
      printf ("  ");
      for (j = 0; j < 16; ++j)
      {
	    printf ("%1X", j);
      }
      printf ("\n");
   }
   while (i < len)
   {
      printf("%08x ", i);
      for (j = 0; j < 16; ++j)
      {
	 if ((i + j) < len)
	    printf (" %02x", (int) buffer[i +j]);
	 else
	    printf ("   ");
      }
      printf ("  ");
      for (j = 0; j < 16; ++j)
      {
	 if ((i + j) < len)
	    printf ("%c", isprint (buffer[i + j]) ? buffer [i + j] : '.');
	 else
	    printf (" ");
      }
      printf ("\n");
      i += 16;
   }
   printf ("-- DONE --\n");
}

int ReadPhysicalDriveInNTWithZeroRights (hdd_guid_t *hg)
{
   int done = FALSE;
   int drive = 0;

   for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
   {
      HANDLE hPhysicalDriveIOCTL = 0;

         //  Try to get a handle to PhysicalDrive IOCTL, report failure
         //  and exit if can't.
      TCHAR driveName [256];

      swprintf (driveName, sizeof(driveName)-1, L"\\\\.\\PhysicalDrive%d", drive);

         //  Windows NT, Windows 2000, Windows XP - admin rights not required
      hPhysicalDriveIOCTL = CreateFile (driveName, 0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, 0, NULL);


      if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
	  {
             MyOutputDebugString ("\n%d ReadPhysicalDriveInNTWithZeroRights ERROR"
		             "\nCreateFile(%s) returned INVALID_HANDLE_VALUE\n",
		             __LINE__, driveName);
	  }
      else if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
      {
		 STORAGE_PROPERTY_QUERY query;
         DWORD cbBytesReturned = 0;
		 char buffer [10000];

         memset ((void *) & query, 0, sizeof (query));
		 query.PropertyId = StorageDeviceProperty;
		 query.QueryType = PropertyStandardQuery;

		 memset (buffer, 0, sizeof (buffer));

         if ( DeviceIoControl (hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY,
                   & query,
                   sizeof (query),
				   & buffer,
				   sizeof (buffer),
                   & cbBytesReturned, NULL) )
         {         
			 STORAGE_DEVICE_DESCRIPTOR * descrip = (STORAGE_DEVICE_DESCRIPTOR *) & buffer;
			 char tmp [1000];

			 //MyOutputDebugString ("\n%d STORAGE_DEVICE_DESCRIPTOR contents for drive %d\n"
			 //               "                Version: %ld\n"
			 //               "                   Size: %ld\n"
			 //               "             DeviceType: %02x\n"
			 //               "     DeviceTypeModifier: %02x\n"
			 //               "         RemovableMedia: %d\n"
			 //               "        CommandQueueing: %d\n"
			 //               "         VendorIdOffset: %4ld (0x%02lx)\n"
			 //               "        ProductIdOffset: %4ld (0x%02lx)\n"
			 //               "  ProductRevisionOffset: %4ld (0x%02lx)\n"
			 //               "     SerialNumberOffset: %4ld (0x%02lx)\n"
			 //               "                BusType: %d\n"
			 //               "    RawPropertiesLength: %ld\n",
			 //               __LINE__, drive,
			 //               (unsigned long) descrip->Version,
			 //               (unsigned long) descrip->Size,
			 //               (int) descrip->DeviceType,
			 //               (int) descrip->DeviceTypeModifier,
			 //               (int) descrip->RemovableMedia,
			 //               (int) descrip->CommandQueueing,
			 //               (unsigned long) descrip->VendorIdOffset,
			 //               (unsigned long) descrip->VendorIdOffset,
			 //               (unsigned long) descrip->ProductIdOffset,
			 //               (unsigned long) descrip->ProductIdOffset,
			 //               (unsigned long) descrip->ProductRevisionOffset,
			 //               (unsigned long) descrip->ProductRevisionOffset,
			 //               (unsigned long) descrip->SerialNumberOffset,
			 //               (unsigned long) descrip->SerialNumberOffset,
			 //               (int) descrip->BusType,
			 //               (unsigned long) descrip->RawPropertiesLength);

			 //dump_buffer ("Contents of RawDeviceProperties",
			 //             (unsigned char*) descrip->RawDeviceProperties,
			 //             descrip->RawPropertiesLength);

			 //dump_buffer ("Contents of first 256 bytes in buffer",
			 //             (unsigned char*) buffer, 256);

			snprintf(hg->volume_physicalname, sizeof(hg->volume_physicalname), "%s", "\\\\.\\PHYSICALDRIVE0");

			strcpy (tmp, & buffer [descrip -> ProductIdOffset ]);
			snprintf(hg->volume_name, sizeof(hg->volume_name), "%s", tmp);

			strcpy (tmp, flipAndCodeBytes ( & buffer [descrip -> SerialNumberOffset]));
			snprintf(hg->volume_serial, sizeof(hg->volume_serial), "%s", tmp);

			int i=0;
			int j=0;
			while (i<20)
			{
				if (hg->volume_serial[i]!=' ')
					hg->volume_serial[j++]=hg->volume_serial[i];
				if (hg->volume_serial[i+1]!=' ')
					hg->volume_serial[j++]=hg->volume_serial[i+1];

				if (hg->volume_serial[i]=='\0')
					break;
				if (hg->volume_serial[i+1]=='\0')
					break;
				i++;
				i++;
			}
			hg->volume_serial[j]='\0';

			GUID guid;
			memset(&guid, 0, sizeof(guid));
			memcpy(&guid, hg->volume_serial, sizeof(GUID));
			wchar_t szGuid[40];
			HRESULT hr = StringFromGUID2(guid, szGuid, 39);
			if (SUCCEEDED(hr))
			{
				WideCharToMultiByte(CP_UTF8,0,szGuid,-1,hg->volume_guid,sizeof(szGuid),0,0);
			}
		 }
		 else
		 {
			 DWORD err = GetLastError ();
			 MyOutputDebugString ("\nDeviceIOControl IOCTL_STORAGE_QUERY_PROPERTY error = %d\n", err);
		 }

		if (hg->volume_name[0]!='\0' && hg->volume_serial[0]!='\0'
			&& hg->volume_physicalname[0]!='\0' && hg->volume_guid[0]!='\0')
		{
			MyOutputDebugString("-- used method 5-1 to retreive HDD\n");
			return 1;
		}

		 memset (buffer, 0, sizeof (buffer));

         if ( DeviceIoControl (hPhysicalDriveIOCTL, IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER,
                   NULL,
                   0,
				   & buffer,
				   sizeof (buffer),
                   & cbBytesReturned, NULL) )
         {         
			 MEDIA_SERIAL_NUMBER_DATA * mediaSerialNumber = 
							(MEDIA_SERIAL_NUMBER_DATA *) & buffer;
			 char serialNumber [1000];

			 strcpy (serialNumber, (char *) mediaSerialNumber -> SerialNumberData);
			snprintf(hg->volume_serial, sizeof(hg->volume_serial), "%s", serialNumber);

			int i=0;
			int j=0;
			while (i<20)
			{
				if (hg->volume_serial[i]!=' ')
					hg->volume_serial[j++]=hg->volume_serial[i];
				if (hg->volume_serial[i+1]!=' ')
					hg->volume_serial[j++]=hg->volume_serial[i+1];

				if (hg->volume_serial[i]=='\0')
					break;
				if (hg->volume_serial[i+1]=='\0')
					break;
				i++;
				i++;
			}
			hg->volume_serial[j]='\0';

			GUID guid;
			memset(&guid, 0, sizeof(guid));
			memcpy(&guid, hg->volume_serial, sizeof(GUID));
			wchar_t szGuid[40];
			HRESULT hr = StringFromGUID2(guid, szGuid, 39);
			if (SUCCEEDED(hr))
			{
				WideCharToMultiByte(CP_UTF8,0,szGuid,-1,hg->volume_guid,sizeof(szGuid),0,0);
			}
			MyOutputDebugString("-- used method 5-2 to retreive SERIAL %s\n", hg->volume_serial);
		 }
		 else
		 {
			 DWORD err = GetLastError ();
			 MyOutputDebugString ("\nDeviceIOControl IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER error = %d\n", err);
		 }

         CloseHandle (hPhysicalDriveIOCTL);
      }
   }

   return done;
}

static int LoadDiskInfo (hdd_guid_t *hg)
{
	int done = FALSE;
	__int64 id = 0;
	OSVERSIONINFO version;
	memset (&version, 0, sizeof (version));
	version.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	GetVersionEx (&version);
	//for(UINT i = 0; i< m_list.size(); i++)
	//	delete m_list[i];
	//m_list.clear();
	if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		//  this works under WinNT4 or Win2K if you have admin rights
		done = ReadPhysicalDriveInNTWithAdminRights (hg);

		if (hg->volume_name[0]!='\0' && hg->volume_serial[0]!='\0'
			&& hg->volume_physicalname[0]!='\0' && hg->volume_guid[0]!='\0')
		{
			MyOutputDebugString("-- used method 3 to retreive HDD\n");
			return 3;
		}

		//  this should work in WinNT or Win2K if previous did not work
		//  this is kind of a backdoor via the SCSI mini port driver into
		//     the IDE drives
		done = ReadIdeDriveAsScsiDriveInNT (hg);

		if (hg->volume_name[0]!='\0' && hg->volume_serial[0]!='\0'
			&& hg->volume_physicalname[0]!='\0' && hg->volume_guid[0]!='\0')
		{
			MyOutputDebugString("-- used method 4 to retreive HDD\n");
			return 4;
		}

		//this works under WinNT4 or Win2K or WinXP if you have any rights
		done = ReadPhysicalDriveInNTWithZeroRights (hg);
		if (hg->volume_name[0]!='\0' && hg->volume_serial[0]!='\0'
			&& hg->volume_physicalname[0]!='\0' && hg->volume_guid[0]!='\0')
		{
			MyOutputDebugString("-- used method 5 to retreive HDD\n");
			return 5;
		}

		done = ReadPhysicalDriveInNTUsingSmart(hg);

		if (hg->volume_name[0]!='\0' && hg->volume_serial[0]!='\0'
			&& hg->volume_physicalname[0]!='\0' && hg->volume_guid[0]!='\0')
		{
			MyOutputDebugString("-- used method 6 to retreive HDD\n");
			return 6;
		}
	}
	else
	{
		//  this works under Win9X and calls a VXD
		int attempt = 0;

		//  try this up to 10 times to get a hard drive serial number
		for (attempt = 0;
			attempt < 10 && !done ;
			attempt++)
		{
			//done = ReadDrivePortsInWin9X ();
		}
	}
	return -1; //(long) m_list.size();
}

#endif

static unsigned int htoi(char s[])
{
    unsigned int val = 0;
    int x = 0;
    
    if(s[x] == '0' && (s[x+1]=='x' || s[x+1]=='X')) x+=2;
    
    while(s[x]!='\0')
    {
       if(val > UINT_MAX) return 0;
       else if(s[x] >= '0' && s[x] <='9')
       {
          val = val * 16 + s[x] - '0';
       }
       else if(s[x]>='A' && s[x] <='F')
       {
          val = val * 16 + s[x] - 'A' + 10;
       }
       else if(s[x]>='a' && s[x] <='f')
       {
          val = val * 16 + s[x] - 'a' + 10;
       }
       else return 0;
       
       x++;
    }
    return val;
}

int hdd_guid_get(hdd_guid_t *hg)
{
    HRESULT hres;
	int i;

	memset(hg, 0, sizeof(hdd_guid_t));

    //hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    hres =  CoInitialize(NULL); 
    if (FAILED(hres))
    {
		MyOutputDebugString("Failed to CoInitialize. Error code = 0x%x", hres);
        //return -1;
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------
    // Note: If you are using Windows 2000, you need to specify -
    // the default authentication credentials for a user by using
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
    // parameter of CoInitializeSecurity ------------------------

    hres =  CoInitializeSecurity(
        NULL, 
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
        );

                      
    if (FAILED(hres))
    {
		MyOutputDebugString("Failed to CoInitializeSecurity. Error code = 0x%x", hres);
        //CoUninitialize();
        //return -1;
    }

    IWbemLocator *pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (FAILED(hres))
    {
        //cout << "Failed to create IWbemLocator object."
        //    << " Err code = 0x"
        //    << hex << hres << endl;
        CoUninitialize();
        return -3;
    }

    IWbemServices *pSvc = NULL;
	
    hres = pLoc->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (e.g. Kerberos)
         0,                       // Context object 
         &pSvc                    // pointer to IWbemServices proxy
         );
    
    if (FAILED(hres))
    {
        //cout << "Could not connect. Error code = 0x" 
        //     << hex << hres << endl;
        pLoc->Release();     
        CoUninitialize();
        return -4;
    }


    hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        //cout << "Could not set proxy blanket. Error code = 0x" 
        //    << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return -5;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject *pclsObj;
    ULONG uReturn = 0;
   
    hres = pSvc->ExecQuery(
        bstr_t("WQL"), 
        bstr_t("SELECT * FROM Win32_DiskDrive"), // DeviceID=\"\\\\.\\PHYSICALDRIVE0\""), //Win32_PhysicalMedia"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);
    
    if (FAILED(hres))
    {
        //cout << "Query for operating system name failed."
        //    << " Error code = 0x" 
        //    << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return -6;
    }
   
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
            &pclsObj, &uReturn);

        if(0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        hr = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
		{
			char szVal[256];
			WideCharToMultiByte(CP_UTF8,0,vtProp.bstrVal,-1,szVal,256,0,0);
			VariantClear(&vtProp);

			if (strcasecmp(szVal, "\\\\.\\PHYSICALDRIVE0")==0)
			{
				snprintf(hg->volume_physicalname, sizeof(hg->volume_physicalname), "%s", szVal);

				// Get the value of the Name property
				hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
				if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
				{
					WideCharToMultiByte(CP_UTF8,0,vtProp.bstrVal,-1,szVal,256,0,0);
					VariantClear(&vtProp);
					snprintf(hg->volume_name, sizeof(hg->volume_name), "%s", szVal);
				}

				//Windows Server 2003, Windows XP, Windows 2000, and Windows NT 4.0:  This property is not available.
				hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
				if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
				{
					/* convert from HEX TO ASCII */
					//BSTR = 0x001e22f4 "2020202057202d445857374c3830313134383832"
					int i=0;
					int j=0;
					char *val = (char *)vtProp.bstrVal;
					while (i<50)
					{
						char l[3];
						char l2[3];
						l[0]=(char)vtProp.bstrVal[i*2];
						l[1]=(char)vtProp.bstrVal[i*2+1];
						l[2]='\0';
						l2[0]=(char)vtProp.bstrVal[i*2+2];
						l2[1]=(char)vtProp.bstrVal[i*2+3];
						l2[2]='\0';

						char c = htoi(l2);
						if (c!=0x20)
							szVal[j++]=c;
						if (c==0x00)
							break;
						
						c = htoi(l);
						if (c!=0x20)
							szVal[j++]=c;
						if (c==0x00)
							break;
						i++;
						i++;
					}
					szVal[i]='\0';
					VariantClear(&vtProp);
					snprintf(hg->volume_serial, sizeof(hg->volume_serial), "%s", szVal);

					GUID guid;
					memset(&guid, 0, sizeof(guid));
					memcpy(&guid, hg->volume_serial, sizeof(GUID));
					wchar_t szGuid[40];
					hr = StringFromGUID2(guid, szGuid, 39);
					if (SUCCEEDED(hr))
					{
						WideCharToMultiByte(CP_UTF8,0,szGuid,-1,hg->volume_guid,sizeof(szGuid),0,0);
					}
				}

				pclsObj->Release();
				break;
			}
		}
		
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pEnumerator->Release();

	if (hg->volume_name[0]!='\0' && hg->volume_serial[0]!='\0'
		&& hg->volume_physicalname[0]!='\0' && hg->volume_guid[0]!='\0')
	{
		MyOutputDebugString("-- used method 1 to retreive HDD\n");
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;
	}

    hres = pSvc->ExecQuery(
        bstr_t("WQL"), 
        bstr_t(L"SELECT * FROM Win32_PhysicalMedia"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);
    
    if (FAILED(hres))
    {
 /*       cout << "Query for operating system name failed."
            << " Error code = 0x" 
            << hex << hres << endl;*/
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return -7;
    }
   
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
            &pclsObj, &uReturn);

        if(0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

		hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
		{
		}
        hr = pclsObj->Get(L"Tag", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
		{
			char szVal[256];
			WideCharToMultiByte(CP_UTF8,0,vtProp.bstrVal,-1,szVal,256,0,0);
			VariantClear(&vtProp);

			if (strcasecmp(szVal, "\\\\.\\PHYSICALDRIVE0")==0)
			{
				snprintf(hg->volume_physicalname, sizeof(hg->volume_physicalname), "%s", szVal);

				// Get the value of the Name property
				hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
				if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
				{
					WideCharToMultiByte(CP_UTF8,0,vtProp.bstrVal,-1,szVal,256,0,0);
					VariantClear(&vtProp);
					snprintf(hg->volume_name, sizeof(hg->volume_name), "%s", szVal);
				}
				hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
				if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
				{
					WideCharToMultiByte(CP_UTF8,0,vtProp.bstrVal,-1,szVal,256,0,0);
					VariantClear(&vtProp);
					snprintf(hg->volume_serial, sizeof(hg->volume_serial), "%s", szVal);

					GUID guid;
					memset(&guid, 0, sizeof(guid));
					memcpy(&guid, hg->volume_serial, sizeof(GUID));
					wchar_t szGuid[40];
					hr = StringFromGUID2(guid, szGuid, 39);
					if (SUCCEEDED(hr))
					{
						WideCharToMultiByte(CP_UTF8,0,szGuid,-1,hg->volume_guid,sizeof(szGuid),0,0);
					}						
				}

				//pclsObj->Release();
				//break;
			}
		}
		
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

	if (hg->volume_name[0]!='\0' && hg->volume_serial[0]!='\0'
		&& hg->volume_physicalname[0]!='\0' && hg->volume_guid[0]!='\0')
	{
		MyOutputDebugString("-- used method 2 to retreive HDD\n");
		return 2;
	}

	memset(hg, 0, sizeof(hdd_guid_t));

	i = LoadDiskInfo (hg);
	if (hg->volume_name[0]!='\0' && hg->volume_serial[0]!='\0'
		&& hg->volume_physicalname[0]!='\0' && hg->volume_guid[0]!='\0')
	{
		return i;
	}

	memset(hg, 0, sizeof(hdd_guid_t));
	return -8;	
}

int proc_guid_get(proc_guid_t *hg)
{
    HRESULT hres;

	memset(hg, 0, sizeof(proc_guid_t));

    //hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    hres =  CoInitialize(NULL); 
    if (FAILED(hres))
    {
        return -1;
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------
    // Note: If you are using Windows 2000, you need to specify -
    // the default authentication credentials for a user by using
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
    // parameter of CoInitializeSecurity ------------------------

    hres =  CoInitializeSecurity(
        NULL, 
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
        );

                      
    if (FAILED(hres))
    {
        CoUninitialize();
        return -1;
    }

    IWbemLocator *pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (FAILED(hres))
    {
        CoUninitialize();
        return -1;
    }

    IWbemServices *pSvc = NULL;
	
    hres = pLoc->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (e.g. Kerberos)
         0,                       // Context object 
         &pSvc                    // pointer to IWbemServices proxy
         );
    
    if (FAILED(hres))
    {
        pLoc->Release();     
        CoUninitialize();
        return -1;
    }


    hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return -1;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject *pclsObj;
    ULONG uReturn = 0;
   
    hres = pSvc->ExecQuery(
        bstr_t("WQL"), 
        bstr_t("SELECT * FROM Win32_Processor"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);
    
    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return -1;
    }
   
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
            &pclsObj, &uReturn);

        if(0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        hr = pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
		{
			char szVal[256];
			WideCharToMultiByte(CP_UTF8,0,vtProp.bstrVal,-1,szVal,256,0,0);
			VariantClear(&vtProp);

			if (szVal[0]!='\0')
			{
				snprintf(hg->proc_serial, sizeof(hg->proc_serial), "%s", szVal);

				// Get the value of the Name property
				hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
				if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
				{
					WideCharToMultiByte(CP_UTF8,0,vtProp.bstrVal,-1,szVal,256,0,0);
					VariantClear(&vtProp);
					snprintf(hg->proc_name, sizeof(hg->proc_name), "%s", szVal);

					GUID guid;
					memset(&guid, 0, sizeof(guid));
					memcpy(&guid, hg->proc_serial, sizeof(GUID));
					wchar_t szGuid[40];
					hr = StringFromGUID2(guid, szGuid, 39);
					if (SUCCEEDED(hr))
					{
						WideCharToMultiByte(CP_UTF8,0,szGuid,-1,hg->proc_guid,sizeof(szGuid),0,0);
					}
				}

				pclsObj->Release();
				break;
			}
		}
		
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

	if (hg->proc_name[0]!='\0' && hg->proc_serial[0]!='\0'
		&& hg->proc_guid[0]!='\0')
	{
		MyOutputDebugString("-- used method 1 to retreive PROC\n");
		return 1;
	}

	memset(hg, 0, sizeof(proc_guid_t));
	return -1;	
}

int mb_guid_get(mb_guid_t *hg)
{
    HRESULT hres;

	memset(hg, 0, sizeof(mb_guid_t));

    //hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    hres =  CoInitialize(NULL); 
    if (FAILED(hres))
    {
        return -1;
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------
    // Note: If you are using Windows 2000, you need to specify -
    // the default authentication credentials for a user by using
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
    // parameter of CoInitializeSecurity ------------------------

    hres =  CoInitializeSecurity(
        NULL, 
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
        );

                      
    if (FAILED(hres))
    {
        CoUninitialize();
        return -1;
    }

    IWbemLocator *pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (FAILED(hres))
    {
        CoUninitialize();
        return -1;
    }

    IWbemServices *pSvc = NULL;
	
    hres = pLoc->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (e.g. Kerberos)
         0,                       // Context object 
         &pSvc                    // pointer to IWbemServices proxy
         );
    
    if (FAILED(hres))
    {
        pLoc->Release();     
        CoUninitialize();
        return -1;
    }


    hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return -1;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject *pclsObj;
    ULONG uReturn = 0;
   
    hres = pSvc->ExecQuery(
        bstr_t("WQL"), 
        bstr_t("SELECT * FROM Win32_baseBoard"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);
    
    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return -1;
    }
   
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
            &pclsObj, &uReturn);

        if(0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
		{
			char szVal[256];
			WideCharToMultiByte(CP_UTF8,0,vtProp.bstrVal,-1,szVal,256,0,0);
			VariantClear(&vtProp);

			if (szVal[0]!='\0')
			{
				snprintf(hg->mb_serial, sizeof(hg->mb_serial), "%s", szVal);

				// Get the value of the Name property
				hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
				if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
				{
					WideCharToMultiByte(CP_UTF8,0,vtProp.bstrVal,-1,szVal,256,0,0);
					VariantClear(&vtProp);
					snprintf(hg->mb_name, sizeof(hg->mb_name), "%s", szVal);

					GUID guid;
					memset(&guid, 0, sizeof(guid));
					memcpy(&guid, hg->mb_serial, sizeof(GUID));
					wchar_t szGuid[40];
					hr = StringFromGUID2(guid, szGuid, 39);
					if (SUCCEEDED(hr))
					{
						WideCharToMultiByte(CP_UTF8,0,szGuid,-1,hg->mb_guid,sizeof(szGuid),0,0);
					}
				}

				pclsObj->Release();
				break;
			}
		}
		
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

	if (hg->mb_name[0]!='\0' && hg->mb_serial[0]!='\0'
		&& hg->mb_guid[0]!='\0')
	{
		MyOutputDebugString("-- used method 1 to retreive MB\n");
		return 1;
	}

	memset(hg, 0, sizeof(mb_guid_t));
	return -1;	
}

#else

int hdd_guid_get(hdd_guid_t *hg)
{
	memset(hg, 0, sizeof(hdd_guid_t));
	return -1;
}

int proc_guid_get(proc_guid_t *hg)
{
	memset(hg, 0, sizeof(proc_guid_t));
	return -1;
}

int mb_guid_get(mb_guid_t *hg)
{
	memset(hg, 0, sizeof(mb_guid_t));
	return -1;
}

#endif