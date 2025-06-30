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
 /** Hex file loader into XMC flash.
 *
 * \file
 *
 * CHANGES
 *
 */

#pragma hdrstop
#include <stdio.h>
#include <string>
#include <conio.h>

#include "XMCLoad_API.h"
#include "XMCLoad.h"
#include <fstream>

 /*
 #include <iostream>
 #include <sstream>


 void DBOutXCML(const char* file, const int line, const CHAR* s)
 {
	 std::wostringstream os_;
	 os_ << file << "(" << line << "): \n";
	 os_ << s << "\n";
	 OutputDebugStringW(os_.str().c_str());
 }
 #define DBOUTXMCL(s)       DBOutXCML(__FILE__, __LINE__, s)
 */

#define MAX_BUFFER 20

#define BSL_PROGRAM_FLASH      0x00
#define BSL_RUN_FROM_FLASH     0x01
#define BSL_RUN_FROM_iCache    0x02
#define BSL_ERASE_FLASH        0x03
#define BSL_PROTECT_STATUS     0x04
#define BSL_PROTECT_FLASH      0x05

static BSL_DOWNLOAD bslDownload;  // holds info about download (filename, verbose ... )
static BSL_HEADER   bslHeader;    // header to be transfered to bootloader
static char outputString[1000];   // used for message sending to Main form



#define PRINT_MSG(x)   { sprintf(outputString,x); \
						SendMessage(handle,WM_CONSOLE, (unsigned int)outputString,0 ); }

long XMLoadGetFileSizeFromStream(std::string filePathStr) {
	std::ifstream fs(filePathStr); //(filePathStr);
	long fileSize = fs.tellg();

	if (fileSize > 0) return fileSize;
	else return -1;
}

long XMLoadGetFileSize(const char* filePath)
{
	struct stat stat_buf;
	int rc = stat(filePath, &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

/**
*
* \brief    XMCLoad
* \param[in]   handle     	handle of the Main form: to know where messages have to be sent
* \param[in]   thread     	handle of the thread: to check thread termination request
* \param[in]   com     	  	com port to use
* \param[in]   fileHex      file to download
* \param[in]   IEC          file is IEC app
* \remarks    -
* Load an Hex file into  xmc flash
* - first the xmc flash protection status is checked through header packet  mode 4
* - Then flash is programmed through bl_download_pflash (which is monolitic)
* - header packet with mode =  1 is sent to restart firmware into target board
*
*  fileHex can be:
*	IEC app
*   firmware
*/
int XMCLoad(void* handle, CommPortDM_Thread* thread, CommPortDM* pcom, const char* fileHex, bool IEC, bool st, unsigned char hmi)
{
	unsigned int result;
	/****************** check the Falsh protection status   *****************/
	std::cerr <<"\nCheck Flash protection status... " << std::endl;
	//DRVDownloadManager::MainForm::MainForm_ToOutputTbx("\nCheck Flash protection status... ",true);
	bslHeader.mode = BSL_PROTECT_STATUS;
	bslHeader.flashModule = 0x00;
	result = XMCLAPI_Bl_Send_Header(pcom, bslHeader);
	/************************************************************************/
	int fileSize = -1;
	fileSize = XMLoadGetFileSize(fileHex);

	if (fileSize < 0) {
		fileSize = XMLoadGetFileSizeFromStream(fileHex);

		if (fileSize < 0) result = ERROR_HEXFILE;
	}

	if (result == ERROR_BSL_PROTECTION)
	{
		std::cerr <<"\nERROR: Flash is protected  - quitting\n" << std::endl;
		//DRVDownloadManager::MainForm::MainForm_ToOutputTbx("\nERROR: Flash is protected  - quitting\n",true );
		XMCLAPI_Close_Interface(pcom);
		return 1;
	}
	else if (result == BSL_NO_ERROR) {
		bslDownload.hexFileName = (char*)fileHex;
		bslDownload.hexFileSize = (unsigned int)fileSize;
		bslDownload.verbose = true;

		if (st)
			bslDownload.device = STM32L4;
		else
			bslDownload.device = XMC4500_1024_DEVICE;

		bslDownload.ser_interface = ASC_INTERFACE;
		bslDownload.IEC = IEC;
		bslDownload.HMI = hmi;
		//DRVDownloadManager::MainForm::MainForm_ToOutputTbx("\nDownload Start ",true);
		if ((result = XMCLAPI_Bl_Download_Pflash(handle, thread, pcom, bslDownload)) != BSL_NO_ERROR)
		{
			sprintf_s(outputString, "\nERROR: Hexfile download failed. Reason 1: ");
			strcat_s(outputString, XMCLAPI_Error_Message(result));
			std::cerr <<outputString << std::endl;
			//DRVDownloadManager::MainForm::MainForm_ToOutputTbx(outputString,true);
			//SendMessage(handle,WM_CONSOLE, (unsigned int)outputString,0 );
			return 2;
		}
	}
	else {
		sprintf_s(outputString, "\nERROR: Check protection failed. Reason 2: ");
		strcat_s(outputString, XMCLAPI_Error_Message(result));
		std::cerr <<outputString << std::endl;
		//DRVDownloadManager::MainForm::MainForm_ToOutputTbx(outputString, true);
		//SendMessage(handle,WM_CONSOLE, (unsigned int)outputString,0 );
		return 3;
	}
	bslHeader.mode = BSL_RUN_FROM_FLASH;
	result = XMCLAPI_Bl_Send_Header(pcom, bslHeader);
	if (result != BSL_NO_ERROR)
	{
		sprintf_s(outputString, "\nERROR: Hexfile download failed. Reason: ");
		strcat_s(outputString, XMCLAPI_Error_Message(result));
		std::cerr <<outputString << std::endl;
		//DRVDownloadManager::MainForm::MainForm_ToOutputTbx(outputString, true);
		//SendMessage(handle,WM_CONSOLE, (unsigned int)outputString,0 );
		return 4;
	}
	return 0;
}



