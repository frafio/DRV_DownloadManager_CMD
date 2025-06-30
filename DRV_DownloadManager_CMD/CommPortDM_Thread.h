#pragma once

#ifndef COMMPORTDM_THREAD_H
#define COMMPORTDM_THREAD_H


#define _AMD64_

#include <process.h>
#include "CommPortDM.h"
#include <iostream>
#include <sstream>

#include <threads.h>

using namespace std;

#define BUFFER_SIZE  400
#define COM_BAUD_RATE_256000  256000
#define COM_BAUD_RATE_57600   57600

#define CMD_IEC_UPGRADE      (uint8_t)0x01
#define CMD_IEC_ERASE 	     (uint8_t)0x02
#define CMD_FW_UPGRADE       (uint8_t)0x03
#define CMD_HMI_UPGRADE	     (uint8_t)0x04
#define CMD_HMI_UG_DATA       (uint8_t)0x05

#define INFO_IEC_UPGRADE_END_OK		1
#define INFO_IEC_UPGRADE_END_ERR	2
#define INFO_IEC_ERASE_END_OK		3
#define INFO_IEC_ERASE_END_ERR		4
#define INFO_FW_UPGRADE_END_OK		5
#define INFO_FW_UPGRADE_END_ERR		6

class CommPortDM_Thread {

public:
	//HWND mainHandle;	/* UNUSED */
	string comPortName;
	string fileToDownload;	// [BUFFER_SIZE] ;
	//char loaderToDownload[BUFFER_SIZE];
	string loaderToDownload;
	bool IECUpgrade;
	bool IECErase;
	int HMIoperation;
	bool target_reset;
	string outputString;
	unsigned long dwBaudrate;
	//System:: hexArray;	// [0x4000] ;
	CommPortDM* commPortDM_pointer;
	bool stm32L4_micro;
	bool st_micro;

	bool SendCommand(uint8_t cmd);
	bool InitComPort(unsigned long baudrate, bool info);
	bool TargetReset();

public:
	bool isTerminated;
	bool FWUpgrade;
	CommPortDM_Thread();
	CommPortDM_Thread(std::string commport, std::string filepath);
	void StartFWUpgrade(HANDLE handle, string com, string file, string loader, bool reset, bool st_micro);
	void StartIECUpgrade(void* handle, string com, string file);
	void StartHMIUpgrade(void* handle, string com, string file);
	void StartHMIUgData(void* handle, string com, string file);
	void StartIECErase(void* handle, string com);
	bool GetTerminated(void) { return isTerminated; }  // Get termination request
	bool IECAppUpgrade(bool st);
	bool HMIAppUpgrade(bool st);
	bool HMIDataUpgrade(bool st);
	bool IECAppErase(bool st);
	bool FirmwareUpgrade(unsigned char* hex_array);


};
#endif

