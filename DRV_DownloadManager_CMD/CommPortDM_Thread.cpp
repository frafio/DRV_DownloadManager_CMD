#include "CommPortDM_Thread.h"
#include "XMCLoad_API.h"
#include "XmcLoad.h"
#include <tchar.h>
#include <string>

CommPortDM_Thread::CommPortDM_Thread()
{
	isTerminated = false;
	IECUpgrade = false;
	IECErase = false;
	stm32L4_micro = false;

}

CommPortDM_Thread::CommPortDM_Thread(std::string commport, std::string filepath)
{
	isTerminated = false;
	IECErase = false;
	stm32L4_micro = true;

	if (commPortDM_pointer == nullptr) {
		commPortDM_pointer = CommPortDM::getInstance();
	}

	commPortDM_pointer->SetCommPort(commport);
	comPortName = commport;
	fileToDownload = filepath;
	IECUpgrade = true;
	HMIoperation = 0;
}

	/* -------------------- BEGIN BACKGROUND WORKER PART -------------------- */

	/* Background Worker Main Task Execution */


	/* -------------------- END BACKGROUND WORKER PART -------------------- */

	/* Starts background worker Main task */


	/* Init COM Port */

	bool CommPortDM_Thread::InitComPort(unsigned long baudrate, bool info)
	{
		unsigned int result;

		if (info) {

			std::cerr << "\nStart XMCLoad module " << std::endl;
			/********* show info to main console  **************/
			//sprintf(outputString, "Custom XMCLoad");
			//SendMessage(mainHandle, WM_CONSOLE, (unsigned int)outputString, 0);
			outputString = "\nComm Port = " + comPortName;
			std::cerr << outputString << std::endl;
			//SendMessage(mainHandle, WM_CONSOLE, (unsigned int)outputString, 0);
			dwBaudrate = baudrate;
			//outputString = "\nBaud Rate = " + dwBaudrate;
			//std::cerr << outputString << std::endl;
			//SendMessage(mainHandle, WM_CONSOLE, (unsigned int)outputString, 0);
			//DBOUT("\nInitializing comm port... ");
			std::cerr << "\nInitializing comm port...  " << std::endl;
			/******************************************************/
		}
		/********** initializes the COM port *********************/
		//wstring tmp_portname_wstr = std::wstring(tmp_portname_stdstr.begin(), tmp_portname_stdstr.end()).c_str();
		if ((result = XMCLAPI_Init_Uart(commPortDM_pointer, comPortName, dwBaudrate)) != BSL_NO_ERROR) {
			std::cerr << "\nERROR: COM Port initialization failed - quitting\n " << std::endl;
			//SEND_INFO_TO_MAIN_FORM(INFO_IEC_UPGRADE_END_ERR, 0);
			//SEND_INFO_TO_MAIN_FORM(INFO_IEC_ERASE_END_ERR, 0);
			//SEND_INFO_TO_MAIN_FORM(INFO_FW_UPGRADE_END_ERR, 0);
			return false;
		}
		else if (info) {

			std::cerr << "\nCOM Port initialized " << std::endl;
			return true;
		}
		/******************************************************/
		std::cerr << "\nERROR: COM Port initialization failed: " << result << " BLS error." << std::endl;
		return false;
	}


	bool CommPortDM_Thread::SendCommand(uint8_t cmd)
	{
		uint8_t chRead[2];
		uint8_t chWrite[2];
		unsigned long dwNumOfBytes = 0;
		int j;

		try {
			for (j = 0; j < 25; j++) {
				if (InitComPort(COM_BAUD_RATE_256000, (j == 0))) {   // print info only first cycle
					if (((25 - j) % 5) == 0) {
						//outputString = std::to_string((25 - j) / 5);
						//DBOUT(outputString);
					}
					chWrite[0] = cmd;  // answer with IEC_ERASE command
					chWrite[1] = ~cmd;
					commPortDM_pointer->WriteBuffer(&chWrite[0], 2);
					//commPortDM_pointer->WriteBuffer(&chWrite[1], 1);
					for (uint8_t i_tmp_sleep = 0; i_tmp_sleep < 5; i_tmp_sleep++)
						Sleep(10);
					if (commPortDM_pointer->ReadBytes(chRead, 2) >= 2) {
						if ((chRead[0] == (uint8_t)(~cmd)) && (chRead[1] == cmd)) {
							return true;
						}
					}
				}
				else {
					std::cerr << "\nCom port initialization failed. Init failed. " << std::endl;

					return false;
				}
				/*********** arrive here if init ok but no answer    ***********/
				XMCLAPI_Close_Interface(commPortDM_pointer);
				Sleep(50);
			}
		}
		catch (...) {
			std::cerr << "\nCom port initialization failed. Something wrong. " << std::endl;
			return false;
		}
		/********** cycle end but no answer    *********************/
		//DBOUT("ERROR: NO ANSWER FROM TARGET ");
		return false;
		/**********************************************************************/

	}

	void CommPortDM_Thread::StartIECUpgrade(void* handle, string com, string file) {

		unsigned char hexArray[0x4000];
		int count = 0;

		//mainHandle = handle;
		comPortName = com;
		fileToDownload = file;
		IECUpgrade = true;
		HMIoperation = 0;


		//---- Place thread code here ----
		for (;;) {

			if (isTerminated) {
				//worker->ReportProgress(100, "\nProcess Update Terminated");
				isTerminated = false;
				break;
			}

			if (IECUpgrade) {
				if (HMIoperation == 0) {

					if (IECAppUpgrade(true)) {
						IECUpgrade = false;
					}
				}
				else if (HMIoperation == 1) {
					if (HMIAppUpgrade(true)) {
						IECUpgrade = false;
						HMIoperation = 0;
					}

				}
				else if (HMIoperation == 2) {
					if (HMIDataUpgrade(true)) {
						IECUpgrade = false;
						HMIoperation = 0;
					}
				}
			}
			if (IECErase) {
				if (CommPortDM_Thread::IECAppErase(true)) {
					IECErase = false;
				}
			}

			if (FWUpgrade) {
				//worker->ReportProgress(count, "\nFirmware Upgrade Start...");
				if (FirmwareUpgrade(hexArray)) {
					std::cerr << "\nFirmware Upgrade Done " << std::endl;
					FWUpgrade = false;
					isTerminated = true;
				}
			}
			Sleep(10);
		}
	}

	void CommPortDM_Thread::StartHMIUpgrade(void* handle, string com, string file) {
		//mainHandle = handle;
		comPortName = com;
		fileToDownload = file;
		IECUpgrade = true;
		HMIoperation = 1;
	}

	void CommPortDM_Thread::StartHMIUgData(void* handle, string com, string file) {
		//mainHandle = handle;
		comPortName = com;
		fileToDownload = file;
		IECUpgrade = true;
		HMIoperation = 2;
	}

	void CommPortDM_Thread::StartIECErase(void* handle, string com) {
		//mainHandle = handle;
		comPortName = com;
		IECErase = true;
	}

	void CommPortDM_Thread::StartFWUpgrade(HANDLE handle, string com, string file, string loader, bool reset, bool st_micro)
	{
		//mainHandle = handle;
		comPortName = com;
		fileToDownload = file;
		if (loader != "") {
			loaderToDownload = loader;
		}
		else {
			loaderToDownload = "";
		}
		target_reset = reset;
		FWUpgrade = true;
		IECUpgrade = false;
		HMIoperation = 0;

		commPortDM_pointer = nullptr;
	}

	/**
	*
	* \brief    HMI_UgData
	* \remarks    -
	* Download HMI DATA application to the target.
	*
	*   With this function we want to upgrade IEC application so the command byte
	*   CMD_IEC_UPGRADE and its complement are sent to the target.
	*   Then file.app is sent to the target through XMCLoad(mainHandle,this,hComm,fileToDownload)
	*   XMCLoad start execution checking flash protection status ...
	* -
	*/
	bool CommPortDM_Thread::HMIDataUpgrade(bool st)
	{
		if (SendCommand(CMD_HMI_UG_DATA)) {
			std::cerr << "\nIEC DATA Upgrade Started " << std::endl;
			if (XMCLoad(nullptr, this, commPortDM_pointer, fileToDownload.c_str(),
				/* filesize */
				true, true, 2) == 0)
			{
				std::cerr << "\nIEC UPGRADE COMPLETED SUCCESFULLY " << std::endl;
				//SEND_INFO_TO_MAIN_FORM(INFO_IEC_UPGRADE_END_OK, 0);
			}
			else {
				std::cerr << "\nIEC UPGRADE ERROR " << std::endl;
				//SEND_INFO_TO_MAIN_FORM(INFO_IEC_UPGRADE_END_ERR, 0);
			}
		}
		if (XMCLAPI_Close_Interface(commPortDM_pointer) == BSL_NO_ERROR) {
			return 0;
		}

		return 1;
	}

	/**
	*
	* \brief    HMI_AppUpgrade
	* \remarks    -
	* Download HMI application to the target.
	*
	*   With this function we want to upgrade IEC application so the command byte
	*   CMD_IEC_UPGRADE and its complement are sent to the target.
	*   Then file.app is sent to the target through XMCLoad(mainHandle,this,hComm,fileToDownload)
	*   XMCLoad start execution checking flash protection status ...
	* -
	*/
	bool CommPortDM_Thread::HMIAppUpgrade(bool st)
	{
		if (SendCommand(CMD_HMI_UPGRADE)) {
			std::cerr << "\nIEC APP Upgrade Started " << std::endl;
			if (XMCLoad(nullptr, this, commPortDM_pointer, fileToDownload.c_str(), true, true, 1) == 0) {
				std::cerr << "\nIEC UPGRADE COMPLETED SUCCESFULLY " << std::endl;
				//SEND_INFO_TO_MAIN_FORM(INFO_IEC_UPGRADE_END_OK, 0);
			}
			else {
				std::cerr << "\nIEC UPGRADE ERROR " << std::endl;
				//SEND_INFO_TO_MAIN_FORM(INFO_IEC_UPGRADE_END_ERR, 0);
			}
		}

		if (XMCLAPI_Close_Interface(commPortDM_pointer) == BSL_NO_ERROR) {
			return 0;
		}

		return 1;
	}

	/**
	*
	* \brief    IEC_AppUpgrade
	* \remarks    -
	* Download IEC application to the target.
	*
	*   With this function we want to upgrade IEC application so the command byte
	*   CMD_IEC_UPGRADE and its complement are sent to the target.
	*   Then file.app is sent to the target through XMCLoad(mainHandle,this,hComm,fileToDownload)
	*   XMCLoad start execution checking flash protection status ...
	* -
	*/
	bool CommPortDM_Thread::IECAppUpgrade(bool st)
	{
		if (SendCommand(CMD_IEC_UPGRADE)) {
			std::cerr << "\nIEC APP Upgrade Started " << std::endl;
			if (XMCLoad(nullptr, this, commPortDM_pointer, fileToDownload.c_str(), true, true, 0) == 0) {
				std::cerr << "\nIEC UPGRADE COMPLETED SUCCESFULLY " << std::endl;
				//SEND_INFO_TO_MAIN_FORM(INFO_IEC_UPGRADE_END_OK, 0);
			}
			else {
				std::cerr << "\nIEC UPGRADE ERROR " << std::endl;
				//SEND_INFO_TO_MAIN_FORM(INFO_IEC_UPGRADE_END_ERR, 0);
			}
		}

		if (XMCLAPI_Close_Interface(commPortDM_pointer) == BSL_NO_ERROR) {
			return 0;
		}

		return 1;
	}

	/**
	*
	* \brief    IEC_AppErase
	* \remarks    -
	* Erase IEC App from flash (both data key and XMC flash)
	* -
	*/
	bool CommPortDM_Thread::IECAppErase(bool st)
	{
		BSL_DOWNLOAD bslDownload;
		BSL_HEADER   bslHeader;
		unsigned int result;


		if (SendCommand(CMD_IEC_ERASE)) {
			std::cerr << "\nIEC APP Erase Start " << std::endl;
			if (st == true) {
				bslDownload.device = STM32L4;
			}
			bslDownload.IEC = true;
			bslDownload.verbose = true;
			result = XMCLAPI_Bl_Erase_Flash(nullptr, commPortDM_pointer, bslDownload);
			if (result != BSL_NO_ERROR) {
				std::cerr << "\nIEC APP ERASE ERROR " << std::endl;
				//SEND_INFO_TO_MAIN_FORM(INFO_IEC_ERASE_END_ERR, 0);
			}
			else {
				std::cerr << "\nIEC APP ERASED SUCCESFULLY " << std::endl;
				//SEND_INFO_TO_MAIN_FORM(INFO_IEC_ERASE_END_OK, 0);
				bslHeader.mode = BSL_RUN_FROM_FLASH;
				result = XMCLAPI_Bl_Send_Header(commPortDM_pointer, bslHeader);
				if (result != BSL_NO_ERROR)
				{
					string tmp_str = XMCLAPI_Error_Message(result);
					outputString = "\nERROR: Restarting failed. Reason: " + tmp_str;
					std::cerr << outputString << std::endl;
				}
				std::cerr << "\nRESTARTING..., WAIT GREEN LED " << std::endl;

			}

		}

		if (XMCLAPI_Close_Interface(commPortDM_pointer) == BSL_NO_ERROR) {
			return 0;
		}
		return 1;
	}

	bool CommPortDM_Thread::FirmwareUpgrade(unsigned char* hex_array)
	{
		bool fail = true;
		DWORD appLength;
		unsigned int hex_address, num_of_bytes;
		int init_port_attempt = 0;

		// if no loader to download then wait command request from target

		if (loaderToDownload == "") {
			if (SendCommand(CMD_FW_UPGRADE)) {
				fail = false;
			}
		}
		else {
			while (init_port_attempt < 3) {
				if (InitComPort(COM_BAUD_RATE_57600, true)) {
					fail = false;
					init_port_attempt = 3;
					if (target_reset) {  // if true then target has to be reset through AT commands
						if (TargetReset() == false) {
							std::cerr << "\nFW Upgrade Error " << std::endl;
							//SEND_INFO_TO_MAIN_FORM(INFO_FW_UPGRADE_END_ERR, 0);
							fail = true;
						}
					}
					if (fail == false) {

						std::cerr << "\nInitializing built in loader... " << std::endl;
						if (XMCLAPI_Init_ASC_BSL(commPortDM_pointer) != BSL_NO_ERROR) {
							std::cerr << "\nBuilt in loader initialization fail  " << std::endl;
							fail = true;
						}
						std::cerr << "\nDone  " << std::endl;
					}
					if (fail == false) {
						appLength = 16384;
						std::cerr << "\nSending loader length... " << std::endl;
						if (XMCLAPI_Send_4_Length(commPortDM_pointer, appLength) != BSL_NO_ERROR) {
							std::cerr << "\nSend loader length failed  " << std::endl;
							fail = true;
						}
						std::cerr << "\nDone " << std::endl;
					}
					if (fail == false) {
						if (XMCLAPI_Make_Flash_Image(loaderToDownload.c_str(), hex_array, appLength, &hex_address, &num_of_bytes) != BSL_NO_ERROR) {
							std::cerr << "\nOpening 'ASCLoader.hex' failed " << std::endl;
							fail = true;
						}
					}
					if (fail == false) {
						std::cerr << "\nSending ASCLoader... " << std::endl;

						if (XMCLAPI_Send_ASCloader(commPortDM_pointer, hex_array, appLength) != BSL_NO_ERROR) {
							std::cerr << "\nInstalling ASCLoader failed " << std::endl;
							fail = true;
						}


					}
				}
				else {
					init_port_attempt++;
					/* little delay */
					for (int itmp = 0; itmp < 100000; itmp++);
					std::cerr << "\nTrying again " + init_port_attempt << std::endl;
				}
			}
		}
		/*
		if (SendCommand(CMD_FW_UPGRADE)) {
			fail = false;
		}
		*/
		if (fail == false) {
			std::cerr << "\nFW Upgrade Started " << std::endl;
			if (XMCLoad(nullptr, this, commPortDM_pointer, fileToDownload.c_str(), false, true, 0) == 0) {
				std::cerr << "\nFW Upgrade Success " << std::endl;
			}
			else {
				std::cerr << "\nFW Upgrade Error " << std::endl;
			}
		}


		XMCLAPI_Close_Interface(commPortDM_pointer);

		return true;
	}

	bool CommPortDM_Thread::TargetReset()
	{
		char tx_buffer[30];
		DWORD num_of_bytes = 0;
		unsigned char chRead[11];



		strcpy_s(tx_buffer, "+++AT\r");
		std::cerr << "\nEntering AT commands... " << std::endl;
		_flushall();
		commPortDM_pointer->WriteBuffer((BYTE*)(&tx_buffer[0]), 7);
		Sleep(100);
		commPortDM_pointer->ReadBytes(chRead, 6);
		if ((chRead[0] != 'O') || (chRead[1] != 'K')) {
			std::cerr << "\nUnable to enter AT commands " << std::endl;
			return false;
		}
		strcpy_s(tx_buffer, "AT#BOT\r");
		std::cerr << "\nResetting target " << std::endl;
		commPortDM_pointer->WriteBuffer((BYTE*)(&tx_buffer[0]), 7);
		Sleep(100);
		_flushall();
		commPortDM_pointer->FlushCommPort();
		chRead[0] = 0;
		return true;
	}

