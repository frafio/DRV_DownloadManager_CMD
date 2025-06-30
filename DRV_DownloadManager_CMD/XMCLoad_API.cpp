/*============================================================================
 *
 * $HeadURL: http://serversvn/svn/DYN2P/Coder/trunk/Firmware/CoderB/Src/Adc.c $
 * $Revision: 20571 $
 * $Date: 2019-01-31 09:47:08 +0100 (gio, 31 gen 2019) $
 * $Author: francomg $
 *
 *  Copyright (c) 2016 Autec srl via Pomaroli,65 36030 Caldogno (Vi) Italy
 *
 */
 /** Low level routines for sending data to XMC microcontroller
 *
 * \file
 *
 * CHANGES
 *
 */
#include "XMCLoad_API.h"
#include "Device_Memory.h"

#include "CommPortDM.h"
#include "CommPortDM_Thread.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <string.h>
#include <conio.h>
#include <malloc.h>
#include <assert.h>


#include <iostream>
#include <fstream>
#include <vector>



 /*
 #include <iostream>
 #include <sstream>


 void DBOutXCMLAPI(const char* file, const int line, const CHAR* s)
 {
	 std::wostringstream os_;
	 os_ << file << "(" << line << "): \n";
	 os_ << s << "\n";
	 OutputDebugStringW(os_.str().c_str());
 }
 #define DBOUTXMCLAPI(s)       DBOutXCMLAPI(__FILE__, __LINE__, s)
 */

 // Global variable
DCB    BSLdcb;
static char outputString[1000];  // for message sending to Main form


static void sleep_us(double timeUs);

// print a formatted message ( with one parameter )

/*------------------------------------------------------------------------------
Function Name   : sleep_us()
Description     : High Performance Counter, to put the system to sleep less than 1 ms.
Function Called : None
Input Parameter : timeUs  => Time to sleep in us.
Output Parameter: None
Return Value    : None
LARGE_INTEGER it's a windows type, basically a portable 64 bit integer.
			  It's definition depends on whether the target system supports
			  64 bit integers or not. If the system doesn't support 64 bit
			  ints then it's defined as 2 32 bit ints, a HighPart and a LowPart.
			  If the system does support 64 bit ints then it's a union between
			  the 2 32 bit ints and a 64 bit int called the QuadPart
---------------------------------------------------------------------------------*/
static void sleep_us(double timeUs)
{
	LARGE_INTEGER tick;
	double ticks_per_us;
	double start, end;
	QueryPerformanceFrequency(&tick);   // get the  current performance-counter frequency
	ticks_per_us = (double)tick.QuadPart / 1e6;
	QueryPerformanceCounter(&tick);     // get counter value
	start = (double)tick.QuadPart / ticks_per_us;
	end = start;
	while (end < (start + timeUs)) {
		QueryPerformanceCounter(&tick);
		end = (double)tick.QuadPart / ticks_per_us;
	}
}


/*------------------------------------------------------------------------------
Function Name   : init_uart()
Description     : Responsible to initialize the UART Port (COM PORT)
				  Following actions are done:
									-) Initialize the chosen COM PORT with the selected baudrate
									-) Return the hComm handle.
Function Called : None
Input Parameter : *cPortName     => Port Name (e.g: "COM1", "COM2", etc)
				  dwBaudrate     => Baudrate
Output Parameter: *hComm         => Valid Communication Handle.
Return Value    : Error Code
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Init_Uart(CommPortDM* pcom, std::string stdstrPortName, DWORD dwBaudrate)
{
	//char portString[20];

	  //sprintf_s(portString, "\\\\.\\%s", cPortName);
	  //sprintf_s(portString, "%s", wchPortName);
	pcom->SetBaudRate(dwBaudrate);
	pcom->SetCommPort(stdstrPortName);
	pcom->OpenCommPort();

	pcom->FlushCommPort();

	return BSL_NO_ERROR;

}


/*------------------------------------------------------------------------------
Function Name   : bsl_close_interface()
Description     : Responsible to close all of the communication channel (UART or JTAG)
Function Called : None
Input Parameter : *hComm        => Communication Handle.
Output Parameter: None
Return Value    : Error Code
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Close_Interface(CommPortDM* pcom)
{



	if ((pcom != NULL) && (pcom != INVALID_HANDLE_VALUE)) {
		pcom->CloseCommPort();
		pcom = NULL;
	}
	return BSL_NO_ERROR;
}

/*------------------------------------------------------------------------------
Function Name   : init_ASC_BSL()
Description     : Initialize ASC BSL mechanism
Function Called : None
Input Parameter : *hComm        => Communication Handle.
Output Parameter: None
Return Value    : Error Code
Send 0x00 byte for autobaudrate detection (max 115200)
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Init_ASC_BSL(CommPortDM* pcom)
{

	unsigned char     chWrite[16];
	unsigned char     chRead[16];
	unsigned int i;
	DWORD             dwNumOfBytes = 0;

	chRead[0] = 0;

	chWrite[0] = 0x00;

	int num_write = pcom->WriteBuffer(&chWrite[0], 1);

	//Sleep(50);

	for (i = 0; i < 100; i++) {
		if (pcom->ReadBytes(chRead, 1)) {
			if (chRead[0] == 0xD5) break;
		}
	}


	if (chRead[0] == 0xD5)
		return BSL_NO_ERROR;
	else
		return ERROR_BSL_INIT;

}

/*------------------------------------------------------------------------------
Function Name   : send_4_length()
Description     : Send 4 bytes of application length
Function Called : None
Input Parameter : *hComm        => Communication Handle.
Output Parameter: None
Return Value    : Error Code
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Send_4_Length(CommPortDM* pcom, DWORD appLength)
{

	unsigned char     chWrite[16];
	unsigned char     chRead[16];
	DWORD             dwNumOfBytes = 0;
	unsigned int i;

	chRead[0] = 0;

	chWrite[0] = appLength & 0xff; //1. byte
	chWrite[1] = (appLength >> 8) & 0xff; //2. byte
	chWrite[2] = (appLength >> 16) & 0xff; //3. byte
	chWrite[3] = (appLength >> 24) & 0xff; //3. byte

	pcom->WriteBuffer(&chWrite[0], 4);

	//	Sleep(100);

	for (i = 0; i < 1000; i++) {
		if (pcom->ReadBytes(chRead, 1)) {
			if (chRead[0] == 0x01) break;
		}
	}

	if (chRead[0] == 0x01)  //should be 0x01
		return BSL_NO_ERROR;
	else
		return ERROR_BSL_INIT;
}


/*------------------------------------------------------------------------------
Function Name   : send_ASCloader()
Description     : Send Loader 3
Function Called : None
Input Parameter : *hComm        => Communication Handle.
									*hexArray     => Pointer to the array containing the code
									size					=> Code size
Output Parameter: None
Return Value    : Error Code
*  Asc loader is sent in one block
*  At the end a
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Send_ASCloader(CommPortDM* pcom, unsigned char* hexArray, unsigned int size)
{
	unsigned char     chRead[16];
	unsigned int i;

	pcom->WriteBuffer(hexArray, size);


	for (i = 0; i < 1000; i++) {
		if (pcom->ReadBytes(chRead, 1)) {
			if (chRead[0] == 0x01) break;
		}
	}

	if ((chRead[0] != 0x01))
		return ERROR_BSL_BOOTCODE;

	return BSL_NO_ERROR;
}


/*------------------------------------------------------------------------------
Function Name   : bl_send_header()
Description     : Sending the Header Block.
Function Called : None
Input Parameter : *hComm         => Communication Handle.
				  bslHeader      => BSL_HEADER structure.
Output Parameter: None
Return Value    : Error Code
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Bl_Send_Header(CommPortDM* pcom, BSL_HEADER bslHeader)
{
	unsigned char chWrite[16] = { 0 };
	unsigned char chRead[8] = { 0 };
	unsigned char chksum = 0;
	unsigned int  i;
	DWORD dwNumOfBytes = 0;

	chWrite[0] = 0x00;
	chWrite[1] = bslHeader.mode;
	chWrite[2] = (bslHeader.startAddress & 0xFF000000) >> 24;
	chWrite[3] = (bslHeader.startAddress & 0x00FF0000) >> 16;
	chWrite[4] = (bslHeader.startAddress & 0x0000FF00) >> 8;
	chWrite[5] = (bslHeader.startAddress & 0x000000FF);
	chWrite[6] = (bslHeader.sectorSize & 0xFF000000) >> 24;
	chWrite[7] = (bslHeader.sectorSize & 0x00FF0000) >> 16;
	chWrite[8] = (bslHeader.sectorSize & 0x0000FF00) >> 8;
	chWrite[9] = (bslHeader.sectorSize & 0x000000FF);
	chWrite[10] = 0x00;
	chWrite[11] = 0x00;
	chWrite[12] = 0x00;
	chWrite[13] = 0x00;
	chWrite[14] = 0x00;


	if (bslHeader.mode == 4)
		chWrite[10] = bslHeader.flashModule; //flash module

	if (bslHeader.mode == 5) {
		//User Password 1
		chWrite[2] = (bslHeader.userPassword1 & 0xFF000000) >> 24;
		chWrite[3] = (bslHeader.userPassword1 & 0x00FF0000) >> 16;
		chWrite[4] = (bslHeader.userPassword1 & 0x0000FF00) >> 8;
		chWrite[5] = (bslHeader.userPassword1 & 0x000000FF);
		//User Password 2
		chWrite[6] = (bslHeader.userPassword2 & 0xFF000000) >> 24;
		chWrite[7] = (bslHeader.userPassword2 & 0x00FF0000) >> 16;
		chWrite[8] = (bslHeader.userPassword2 & 0x0000FF00) >> 8;
		chWrite[9] = (bslHeader.userPassword2 & 0x000000FF);

		chWrite[10] = bslHeader.flashModule;
		chWrite[11] = (bslHeader.protectionConfig & 0xFF00) >> 8;
		chWrite[12] = (bslHeader.protectionConfig & 0x00FF);
	}

	for (i = 1; i < 15; i++)
		chksum = chksum ^ chWrite[i];

	chWrite[15] = chksum;

	pcom->WriteBuffer(&chWrite[0], 16);


	Sleep(10);
	for (i = 0; i < 1000; i++) {
		if (pcom->ReadBytes(chRead, 1)) {
			if ((chRead[0] == 0x55) ||
				(chRead[0] == ERROR_BSL_MODE) ||
				(chRead[0] == ERROR_BSL_BLOCK_TYPE) ||
				(chRead[0] == ERROR_BSL_CHECKSUM) ||
				(chRead[0] == ERROR_BSL_ADDRESS) ||
				(chRead[0] == ERROR_BSL_PROGRAM) ||
				(chRead[0] == ERROR_BSL_ERASE) ||
				(chRead[0] == ERROR_BSL_PROTECTION))
				break;
		}
	}

	Sleep(20);

	if (chRead[0] == 0x55)
		return BSL_NO_ERROR;
	else if ((chRead[0] == ERROR_BSL_MODE) ||
		(chRead[0] == ERROR_BSL_BLOCK_TYPE) ||
		(chRead[0] == ERROR_BSL_CHECKSUM) ||
		(chRead[0] == ERROR_BSL_ADDRESS) ||
		(chRead[0] == ERROR_BSL_PROGRAM) ||
		(chRead[0] == ERROR_BSL_ERASE) ||
		(chRead[0] == ERROR_BSL_PROTECTION))
		return chRead[0];
	else {
		return ERROR_BSL_UNKNOWN;
	}
}



/*------------------------------------------------------------------------------
Function Name   : bl_send_EOT()
Description     : Sending the EOT Block.
Function Called : None
Input Parameter : *hComm         => Communication Handle.
Output Parameter: None
Return Value    : Error Code
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Bl_Send_EOT(CommPortDM* pcom)
{
	unsigned char chWrite[16];
	unsigned char chRead[8] = { 0 };
	unsigned char chksum = 0;
	unsigned int i;
	DWORD dwNumOfBytes = 0;

	chWrite[0] = 0x02;
	for (i = 1; i < 15; i++) {
		chWrite[i] = 0x00; //not used
		chksum ^= chWrite[i];
	}

	chWrite[15] = chksum;

	pcom->WriteBuffer(&chWrite[0], 16);

	Sleep(10);
	for (i = 0; i < 1000; i++) {
		if (pcom->ReadBytes(chRead, 1)) {
			if ((chRead[0] == 0x55) ||
				(chRead[0] == ERROR_BSL_MODE) ||
				(chRead[0] == ERROR_BSL_BLOCK_TYPE) ||
				(chRead[0] == ERROR_BSL_CHECKSUM) ||
				(chRead[0] == ERROR_BSL_ADDRESS) ||
				(chRead[0] == ERROR_BSL_PROGRAM) ||
				(chRead[0] == ERROR_BSL_ERASE) ||
				(chRead[0] == ERROR_BSL_PROTECTION)) break;
		}
	}

	if (chRead[0] == 0x55)
		return BSL_NO_ERROR;
	else if ((chRead[0] == ERROR_BSL_MODE) ||
		(chRead[0] == ERROR_BSL_BLOCK_TYPE) ||
		(chRead[0] == ERROR_BSL_CHECKSUM) ||
		(chRead[0] == ERROR_BSL_ADDRESS) ||
		(chRead[0] == ERROR_BSL_PROGRAM) ||
		(chRead[0] == ERROR_BSL_ERASE) ||
		(chRead[0] == ERROR_BSL_PROTECTION))
		return chRead[0];
	else {
		return ERROR_BSL_UNKNOWN;
	}
}


/*------------------------------------------------------------------------------
Function Name   : bl_send_data()
Description     : Sending the Data Block.
Function Called : None
Input Parameter : *hComm         => Communication Handle.
				  bslData        => BSL_DATA structure.
Output Parameter: None
Return Value    : Error Code
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Bl_Send_Data(CommPortDM* pcom, BSL_DATA bslData)
{
	unsigned char chWrite[DATA_BYTE_TO_LOAD + 8];
	unsigned char chRead[4] = { 0 };
	unsigned char chksum = 0;
	unsigned int i;
	DWORD dwNumOfBytes = 0;

	static int wcnt;
	static int rcnt1;
	static int rcnt2;

	chWrite[0] = 0x01;
	chWrite[1] = bslData.verification;

	for (i = 0; i < DATA_BYTE_TO_LOAD; i++) {
		chWrite[i + 2] = bslData.cDataArray[i];
	}
	for (i = DATA_BYTE_TO_LOAD + 2; i < DATA_BYTE_TO_LOAD + 7; i++)
		chWrite[i] = 0x00; //not used

	for (i = 1; i < DATA_BYTE_TO_LOAD + 7; i++)
		chksum = chksum ^ chWrite[i];

	chWrite[DATA_BYTE_TO_LOAD + 7] = chksum;
	pcom->WriteBuffer(&chWrite[0], DATA_BYTE_TO_LOAD + 8);


	wcnt++;
	Sleep(10);
	for (i = 0; i < 1000; i++) {
		if (pcom->ReadBytes(chRead, 1)) {
			if ((chRead[0] == 0x55) ||
				(chRead[0] == ERROR_BSL_BLOCK_TYPE) ||
				(chRead[0] == ERROR_BSL_CHECKSUM) ||
				(chRead[0] == ERROR_BSL_ADDRESS) ||
				(chRead[0] == ERROR_BSL_PROGRAM) ||
				(chRead[0] == ERROR_BSL_VERIFICATION))
				break;
		}
	}

	if (chRead[0] == 0x55)
		return BSL_NO_ERROR;
	else if ((chRead[0] == ERROR_BSL_BLOCK_TYPE) ||
		(chRead[0] == ERROR_BSL_CHECKSUM) ||
		(chRead[0] == ERROR_BSL_ADDRESS) ||
		(chRead[0] == ERROR_BSL_PROGRAM) ||
		(chRead[0] == ERROR_BSL_VERIFICATION))
		return chRead[0];
	else
		return ERROR_BSL_UNKNOWN;
}



/////////////////////////////////////////////
// Utility to convert Character to Integer //
/////////////////////////////////////////////
void XMCLAPI_charToInt(unsigned int* intOut, char* charLine, unsigned numOfChar) {
	unsigned i;
	char     tmpChar;

	*intOut = 0;
	for (i = 0; i < numOfChar; i++) {
		if (charLine[i] >= 0x41 && charLine[i] <= 0x46) {
			// Check for 0xA - 0xF
			tmpChar = charLine[i] - 0x37;
		}
		else if (charLine[i] >= 0x61 && charLine[i] <= 0x66) {
			// Check for 0xa - 0xf
			tmpChar = charLine[i] - 0x37;
		}
		else {
			tmpChar = charLine[i] & 0xF;
		}
		*intOut |= (tmpChar << (((numOfChar - 1) - i) * 4));
	}
}

/*------------------------------------------------------------------------------
Function Name   : bl_erase_flash()
Description     : Read in HEX file and erase the flash sector to be programmed.
Function Called : bl_send_header()
Input Parameter : *hComm         => Communication Handle.
				  bslDownload    => BSL_DOWNLOAD structure.
Output Parameter: None
Return Value    : Error Code
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Bl_Erase_Flash(void* handle, CommPortDM* pcom, BSL_DOWNLOAD bslDownload)
{

	//FILE *hexFile; /* Declares a file pointer */
	//char hexLine[80];
	unsigned int result;
	BSL_HEADER bslHeader;
	char outputstr[200];
	int sect;

	/**********************************************************************
	* if we are downloading an IEC application then last two sector of XMC
	* microcontroller have always to be erased even if application file is
	* small; this is because the last four bytes of the last sector holds
	* a "programmed key" that is checked to see if an IEC application
	* is present in flash or not
	**********************************************************************/
	if (bslDownload.IEC) {
		//Erase sector this sector
		bslHeader.mode = 3; //Erase mode
		if (bslDownload.device == STM32L4) {
			if (bslDownload.HMI == 2) {
				bslHeader.startAddress = STM32L4_PFLASH_SectorTable[4].dwStartAddr;
				bslHeader.sectorSize = bslDownload.hexFileSize;
			}
			else if (bslDownload.HMI == 1) {
				bslHeader.startAddress = STM32L4_PFLASH_SectorTable[3].dwStartAddr;
				bslHeader.sectorSize = bslDownload.hexFileSize;
			}
			else {
				bslHeader.startAddress = STM32L4_PFLASH_SectorTable[2].dwStartAddr;
				bslHeader.sectorSize = bslDownload.hexFileSize;
			}
			if (bslDownload.verbose) {
				sprintf_s(outputstr, "\nErasing sector 0x%08X (This may take a few seconds)... ", bslHeader.startAddress);
				//DRVDownloadManager::MainForm::MainForm_ToOutputTbx(outputstr, true);
			}
			result = XMCLAPI_Bl_Send_Header(pcom, bslHeader);

			//if (result != BSL_NO_ERROR)
		}
		else {
			for (sect = 10; sect <= 11; sect++) {
				bslHeader.startAddress = XMC4500_1024_PFLASH_SectorTable[sect].dwStartAddr;
				bslHeader.sectorSize = XMC4500_1024_PFLASH_SectorTable[sect].dwSize;
				if (bslDownload.verbose) {
					sprintf_s(outputstr, "\nErasing sector 0x%08X (This may take a few seconds)... ", bslHeader.startAddress);
					//DRVDownloadManager::MainForm::MainForm_ToOutputTbx(outputstr, true);
				}
				result = XMCLAPI_Bl_Send_Header(pcom, bslHeader);

				if (result != BSL_NO_ERROR)
					break;
			}
		}
	}
	else {

		// if firmware has to be downloaded then all sectors dedicated to firmware are erased;
		// it is not necessary but it simplifies the code.
		bslHeader.mode = 3; //Erase mode
		if (bslDownload.device == STM32L4) {
			/* Runtime Erase */
			bslHeader.startAddress = STM32L4_PFLASH_SectorTable[1].dwStartAddr;
			bslHeader.sectorSize = STM32L4_PFLASH_SectorTable[1].dwSize;
			if (bslDownload.verbose) {
				sprintf_s(outputstr, "\nErasing sector 0x%08X (This may take a few seconds)... ", bslHeader.startAddress);
				//DRVDownloadManager::MainForm::MainForm_ToOutputTbx(outputstr, true);

			}
			result = XMCLAPI_Bl_Send_Header(pcom, bslHeader);

		}
		else {
			for (sect = 4; sect <= 9; sect++) {
				bslHeader.startAddress = XMC4500_1024_PFLASH_SectorTable[sect].dwStartAddr;
				bslHeader.sectorSize = XMC4500_1024_PFLASH_SectorTable[sect].dwSize;
				if (bslDownload.verbose) {
					sprintf_s(outputstr, "\nErasing sector 0x%08X (This may take a few seconds)... ", bslHeader.startAddress);
					//DRVDownloadManager::MainForm::MainForm_ToOutputTbx(outputstr, true);
				}
				result = XMCLAPI_Bl_Send_Header(pcom, bslHeader);

				if (result != BSL_NO_ERROR)
					break;
			}
		}
	}
	return result;

}

/*------------------------------------------------------------------------------
Function Name   : bl_download_pflash()
Description     : The function to download the hex file to PFlash.
Function Called : bl_erase_flash(), bl_send_header(), bl_send_data(), bl_send_EOT()
Input Parameter : *hComm          => Communication Handle.
									bslDownload			=> BSL_DOWNLOAD
Output Parameter: None
Return Value    : Error code
Programming sequence:
	bl_erase_flash   to erase the flash
	Hex file to download is opened
	bl_send_header with mode =  0 is sent (means start programming)
	Hex file is read line by line and data are sent to the target through bl_send_data
	bl_send_EOT is sent after all data have been sent
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Bl_Download_Pflash(void* handle, CommPortDM_Thread* thread, CommPortDM* pcom, BSL_DOWNLOAD bslDownload)
{

	FILE* hexFile = NULL; /* Declares a file pointer */
	char hexLine[256];
	unsigned char hexArray[DATA_BYTE_TO_LOAD * 2];
	unsigned char* hexArrayPtr;
	unsigned int hexCount, oldhexCount, hexAddress, oldhexAddress, hexType;
	unsigned int intData;
	char* hexData;
	unsigned int temp_addr, old_temp_addr;
	unsigned int page_addr;
	unsigned int offset;
	unsigned int i, j, j2, result;
	bool firstTime;
	unsigned int num_of_bytes;
	unsigned char writeBuffer[DATA_BYTE_TO_LOAD];
	BSL_HEADER bslHeader;
	BSL_DATA bslData;
	bool prev_address_type;
	char outputstr[300];

	bslDownload.pFlash = true;

	//if Hex file is too big, bl_erase_flash will return an error.
	if ((result = XMCLAPI_Bl_Erase_Flash(handle, pcom, bslDownload)) != BSL_NO_ERROR)
		return result;

	cerr << "\nFlash erased " << endl;
	//DRVDownloadManager::MainForm::MainForm_ToOutputTbx("\nFlash erased ", true);
	/* Open the existing file specified for reading */
	/* Note the use of \\ for path separators in text strings */
	size_t open_res = 0;
	//FILE* hexFile;
	Sleep(500);
	unsigned long sizeEndFile;
	if (bslDownload.HMI > 0 && bslDownload.IEC == true) {
		/* Allowed values of the ccs flag are UNICODE, UTF-8, and UTF-16LE. If no value is specified for ccs, fopen_s uses ANSI encoding. */

		//open_res = fopen_s(&hexFile, bslDownload.hexFileName, "rb");// , ccs = UNICODE");
	}
	else {
		open_res = fopen_s(&hexFile, bslDownload.hexFileName, "rt");
		// Determine file size
		if (open_res != 0)
			return  ERROR_HEXFILE;
		if (hexFile == NULL)
			return  ERROR_HEXFILE;
	}

	num_of_bytes = 0;

	hexArrayPtr = hexArray;
	hexAddress = 0;
	hexCount = 0;
	firstTime = true;

	bslData.cDataArray = writeBuffer;
	bslData.verification = 0x01; //enable verification
	prev_address_type = false;

	old_temp_addr = 0;
	temp_addr = 0;

	uint32_t tot_num_bytes = 0;


	if (bslDownload.device == STM32L4 && bslDownload.IEC == true) {

		if (bslDownload.HMI == 2) {
			page_addr = STM32L4_PFLASH_SectorTable[4].dwStartAddr;
		}
		else if (bslDownload.HMI == 1) {
			page_addr = STM32L4_PFLASH_SectorTable[3].dwStartAddr;
		}
		else {
			page_addr = STM32L4_PFLASH_SectorTable[2].dwStartAddr;
		}
		temp_addr = page_addr;

		hexArrayPtr = &hexArray[0];
		num_of_bytes = 0;

		//send program header
		bslHeader.mode = 0;
		bslHeader.startAddress = page_addr;

		if (bslDownload.ser_interface == ASC_INTERFACE)
			result = XMCLAPI_Bl_Send_Header(pcom, bslHeader);
		else
			result = ~BSL_NO_ERROR;

		if (result != BSL_NO_ERROR)
			return result;
		//unsigned long endFile = bslDownload.hexFileSize;
		unsigned long read_per_time = DATA_BYTE_TO_READ;
		sizeEndFile = bslDownload.hexFileSize;

		//while ((hexCount = fread(&hexLine[0], sizeof(hexLine[0]), read_per_time, hexFile)) > 0 || sizeEndFile > read_per_time) {

		std::ifstream hexfilebin((const char*)(bslDownload.hexFileName), std::ios::binary);
		if (!hexfilebin) {
			std::cerr << "Failed to open file.\n";
			return 1;
		}


		while (hexfilebin) {
			hexfilebin.read(&hexLine[0], read_per_time);
			std::streamsize hexCountRead = hexfilebin.gcount();


				if (sizeEndFile <= 0) {
					sprintf_s(outputstr, "readch EOF at 0x%08X ", page_addr);
					std::cerr << outputstr << std::endl;
					break;
			}

			num_of_bytes += hexCountRead;
			sizeEndFile -= hexCountRead;		

			for (i = 0; i < hexCountRead; i++) {
				*hexArrayPtr++ = (unsigned char)hexLine[i];
			}

			tot_num_bytes += hexCountRead;

			//page full?
			if (num_of_bytes >= DATA_BYTE_TO_LOAD) {

				//send data block
				for (j = 0; j < DATA_BYTE_TO_LOAD; j++)
					writeBuffer[j] = hexArray[j];

				if (bslDownload.verbose) {
					sprintf_s(outputstr, "\nProgramming data block to address 0x%08X ", page_addr);
					std::cerr << outputstr << std::endl;
					//DRVDownloadManager::MainForm::MainForm_ToOutputTbx(outputstr, true);
				}

				if (bslDownload.ser_interface == ASC_INTERFACE)
					result = XMCLAPI_Bl_Send_Data(pcom, bslData);
				else
					//result = blCAN_send_data(hComm,bslData);
					result = ~BSL_NO_ERROR;

				if (result != BSL_NO_ERROR)
					return result;

				page_addr += DATA_BYTE_TO_LOAD;

				//reset number of bytes
				num_of_bytes -= DATA_BYTE_TO_LOAD;

				//reset array pointer
				hexArrayPtr = &hexArray[0];

				//fill beginning of array with remaining bytes
				for (i = 0; i < num_of_bytes; i++)
					*hexArrayPtr++ = hexArray[i + DATA_BYTE_TO_LOAD];

			}

			if (sizeEndFile < DATA_BYTE_TO_READ) {
				read_per_time = sizeEndFile;
			}
		} //end of while		



		hexfilebin.close();

	}
	else {
		while (fgets(&hexLine[0], 80, hexFile) != NULL) {

			if (thread->GetTerminated())
				return 1;

			oldhexCount = hexCount;
			oldhexAddress = hexAddress;
			// Hex Count
			XMCLAPI_charToInt(&hexCount, &hexLine[1], 2);
			// Hex Address
			XMCLAPI_charToInt(&hexAddress, &hexLine[3], 4);

			// Hex Type
			XMCLAPI_charToInt(&hexType, &hexLine[7], 2);
			hexData = &hexLine[9];

			offset = 0;


			if (hexType == 4) {
				XMCLAPI_charToInt(&temp_addr, &hexLine[9], 4);
				if (temp_addr == 0x00000800)
					temp_addr = temp_addr + FLASH_OFFS;
				temp_addr = (temp_addr & 0x0000FFFF) << 16;
				prev_address_type = true;
			}

			if (hexType == 0) {
				//Address tag in Hexfile
				temp_addr &= 0xFFFF0000;
				temp_addr |= (hexAddress & 0x0000FFFF);


				//Address discontinious?
				if (temp_addr - old_temp_addr >= 0x200) {

					if (firstTime == false) {

						if (prev_address_type == false) {

							//send remaining data
							for (j = 0; j < num_of_bytes; j++)
								writeBuffer[j] = hexArray[j];

							for (j = num_of_bytes; j < DATA_BYTE_TO_LOAD; j++)
								writeBuffer[j] = 0x00;

							if (bslDownload.verbose) {
								sprintf_s(outputstr, "\nProgramming data block to address 0x%08X ", page_addr);
								std::cerr << outputstr<< std::endl;
								////DRVDownloadManager::MainForm::MainForm_ToOutputTbx(outputstr, true);
							}
							if (bslDownload.ser_interface == ASC_INTERFACE)
								result = XMCLAPI_Bl_Send_Data(pcom, bslData);
							else
								//result = blCAN_send_data(hComm,bslData);
								result = ~BSL_NO_ERROR;

							if (result != BSL_NO_ERROR)
								return result;

							page_addr += DATA_BYTE_TO_LOAD;

						}

						//send EOT frame
						sprintf_s(outputstr, "\nSend EOT");
						std::cerr << outputstr<< std::endl;
						if (bslDownload.ser_interface == ASC_INTERFACE)
							result = XMCLAPI_Bl_Send_EOT(pcom);
						else
							result = ~BSL_NO_ERROR;



						if (result != BSL_NO_ERROR)
							return result;
					}

					//check if address is 256-byte aligned
					offset = temp_addr & 0xFF;

					if (offset) {
						temp_addr -= offset;
					}

					//send program header
					bslHeader.mode = 0;
					bslHeader.startAddress = temp_addr;

					if (bslDownload.ser_interface == ASC_INTERFACE) {
						sprintf_s(outputstr, "\nSend header 0x%08X ", temp_addr);
						std::cerr << outputstr<< std::endl;
						result = XMCLAPI_Bl_Send_Header(pcom, bslHeader);
					}
					else
						result = ~BSL_NO_ERROR;

					if (result != BSL_NO_ERROR)
						return result;

					hexArrayPtr = &hexArray[0];
					num_of_bytes = 0;

					if (offset) {
						num_of_bytes = offset;
						for (j = 0; j < offset; j++)
							*hexArrayPtr++ = 0x00;
					}

					page_addr = temp_addr;
					old_temp_addr = temp_addr;


					oldhexAddress = hexAddress;
					oldhexCount = hexCount;
					firstTime = false;

				}

				if (prev_address_type == true) {
					oldhexCount = hexCount;
					oldhexAddress = hexAddress;
					prev_address_type = false;
				}

				//fill the page
				for (i = oldhexAddress + oldhexCount; i < hexAddress; i++) {
					*hexArrayPtr++ = 0x00;
					num_of_bytes++;
				}

				for (i = 0; i < hexCount; i++) {
					XMCLAPI_charToInt(&intData, &hexData[(2 * i)], 2);
					*hexArrayPtr++ = intData & 0xFF;
				}

				num_of_bytes += hexCount;
				//page full?
				if (num_of_bytes >= DATA_BYTE_TO_LOAD) {

					//send data block
					for (j = 0; j < DATA_BYTE_TO_LOAD - offset; j++)
						writeBuffer[j] = hexArray[j];

					if (bslDownload.verbose) {
						sprintf_s(outputstr, "\nProgramming data block to address 0x%08X ", page_addr);
						std::cerr << outputstr << std::endl;
					}

					if (bslDownload.ser_interface == ASC_INTERFACE)
						result = XMCLAPI_Bl_Send_Data(pcom, bslData);
					else
						result = ~BSL_NO_ERROR;

					if (result != BSL_NO_ERROR)
						return result;


					old_temp_addr = page_addr;
					page_addr += DATA_BYTE_TO_LOAD;


					//reset number of bytes
					num_of_bytes -= DATA_BYTE_TO_LOAD;

					//reset array pointer
					hexArrayPtr = &hexArray[0];

					//fill beginning of array with remaining bytes
					for (i = 0; i < num_of_bytes; i++)
						*hexArrayPtr++ = hexArray[i + DATA_BYTE_TO_LOAD];

				}

			} //end of if hexType == 0

		} //end of while

	}
	Sleep(10);

	if ((num_of_bytes > 0 || hexCount > 0 ) && bslDownload.IEC == true) {

		if (bslDownload.device == STM32L4 && bslDownload.IEC == true) {

			for (j = 0; j < num_of_bytes; j++) {
				writeBuffer[j] = (unsigned char)(hexArray[j]);
			}

			for (j2 = 0; j2 < hexCount; j2++) {
				writeBuffer[j + j2] = (unsigned char)(hexLine[j2]);
			}

		}
		else {
			//send data block
			for (j = 0; j < num_of_bytes; j++)
				writeBuffer[j] = hexArray[j];
		}

		for (j = num_of_bytes + hexCount; j < DATA_BYTE_TO_LOAD; j++)
			writeBuffer[j] = 0x00;

		if (bslDownload.verbose) {
			sprintf_s(outputstr, "\nProgramming data block to address 0x%08X ", page_addr);
			std::cerr << outputstr<<std::endl;
		}
		if (bslDownload.ser_interface == ASC_INTERFACE)
			result = XMCLAPI_Bl_Send_Data(pcom, bslData);
		else
			//result = blCAN_send_data(hComm,bslData);
			result = ~BSL_NO_ERROR;

		if (result != BSL_NO_ERROR)
			return result;

		page_addr += DATA_BYTE_TO_LOAD;
	}

	//send EOT frame
	if (bslDownload.ser_interface == ASC_INTERFACE)
		result = XMCLAPI_Bl_Send_EOT(pcom);
	else
		result = ~BSL_NO_ERROR;

	if (result != BSL_NO_ERROR)
		return result;


	if (bslDownload.IEC == false) {
		fclose(hexFile);
	}

	std::cerr << "\nApp write SUCCESS" << std::endl;

	return BSL_NO_ERROR;

}


/*------------------------------------------------------------------------------
Function Name   : make_flash_image
Description     : Reads a file in the Intel HEX format and creates a flash image.
Function Called : None
Input Parameter : *hexFile
Name    => HEX file name to open the source file
									max_size			  => Size limit of the flash image
Output Parameter: *image		  => Pointer to array containing the flash image
				  *address		  => Code link address indicated in the HEX file
				  *num_of_bytes	  => Actual size of the flash image
Return Value    : Error code
---------------------------------------------------------------------------------*/
unsigned int XMCLAPI_Make_Flash_Image(const char* hexFileName, unsigned char* image, unsigned int max_size, unsigned int* address, unsigned int* num_of_bytes) {

	FILE* hexFile; /* Declares a file pointer */
	char hexLine[80];
	unsigned int hexCount, oldhexCount, hexAddress, oldhexAddress, hexType;
	unsigned int intData;

	char* hexData;
	unsigned int i;
	unsigned int temp_addr;
	bool address_set = false;
	bool prev_addressType = false;
	bool LinearBaseAddr = false;

	if (max_size > 0xFFFF)
		return  ERROR_HEXFILE;


	/* Open the existing file specified for reading */
	/* Note the use of \\ for path separators in text strings */
	fopen_s(&hexFile, hexFileName, "rt");
	if (hexFile == NULL)
		return  ERROR_HEXFILE;

	*num_of_bytes = 0;

	hexAddress = 0;
	hexCount = 0;

	while (fgets(hexLine, 80, hexFile) != NULL) {
		oldhexCount = hexCount;
		oldhexAddress = hexAddress;

		// Hex Count
		XMCLAPI_charToInt(&hexCount, &hexLine[1], 2);

		// Hex Address
		XMCLAPI_charToInt(&hexAddress, &hexLine[3], 4);

		if (LinearBaseAddr == false) {
			for (i = oldhexAddress + oldhexCount; i < hexAddress; i++)
				*image++ = 0x00;
		}
		else LinearBaseAddr = false;

		// Hex Type
		XMCLAPI_charToInt(&hexType, &hexLine[7], 2);
		hexData = &hexLine[9];

		if (hexType == 0) {
			if (prev_addressType == true) {
				temp_addr |= (hexAddress & 0x0000FFFF);
				prev_addressType = false;
			}
			for (i = 0; i < hexCount; i++) {
				XMCLAPI_charToInt(&intData, &hexData[(2 * i)], 2);
				*image++ = intData & 0xFF;
			}
			*num_of_bytes += hexCount;
		}
		if ((hexType == 4) && (prev_addressType == false) && (address_set == false)) {
			XMCLAPI_charToInt(&temp_addr, &hexLine[9], 4);
			temp_addr = (temp_addr & 0x0000FFFF) << 16;
			address_set = true;
			prev_addressType = true;
			LinearBaseAddr = true;
		}
	}
	if ((*num_of_bytes > max_size) || (*num_of_bytes == 0))
		return ERROR_HEXFILE;

	while (*num_of_bytes < 7168) {        //fill the minimun to 8kbyte
		*image = 0x00;
		image = image++;
		(*num_of_bytes)++;
	}

	*address = temp_addr;
	fclose(hexFile);


	return BSL_NO_ERROR;
}


////////////////////////////////////////////////
// Utility to convert error message to string //
////////////////////////////////////////////////
const char* XMCLAPI_Error_Message(unsigned int uiError) {
	string message = new char[40];
	switch (uiError) {
	case BSL_NO_ERROR:
		message = "No error";
		break;
	case ERROR_BSL_BLOCK_TYPE:
		message = "Block type error";
		break;
	case ERROR_BSL_MODE:
		message = "Invalid mode";
		break;
	case ERROR_BSL_CHECKSUM:
		message = "Checksum error";
		break;
	case ERROR_BSL_ADDRESS:
		message = "Invalid address";
		break;
	case ERROR_BSL_ERASE:
		message = "Erase error";
		break;
	case ERROR_BSL_PROGRAM:
		message = "Program error";
		break;
	case ERROR_BSL_VERIFICATION:
		message = "Verification error";
		break;
	case ERROR_BSL_PROTECTION:
		message = "Protection error";
		break;
	case ERROR_BSL_BOOTCODE:
		message = "Bootcode error";
		break;
	case ERROR_BSL_HEXDOWNLOAD:
		message = "hexfile download error";
		break;
	case ERROR_BSL_UNKNOWN:
		message = "Unknown error";
		break;
	case ERROR_BSL_COMINIT:
		message = "COM-Init error";
		break;
	case ERROR_BSL_INIT:
		message = "BSL-Init error";
		break;
	case ERROR_HEXFILE:
		message = "Invalid hexfile";
		break;
	case ERROR_CANBUS:
		message = "CAN Bus error";
		break;
	default:
		message = "Unknown error";
		break;
	};
	return message.c_str();
}
