/**************************************************************************
** 																          *
**  INFINEON XMCLoad - XMC4000 Bootloader Usage        	                  *
**																          *
**                                                                        *
**  DESCRIPTION :                                                         *
**      - Header file	                  					              *
**      						                                          *  																																	    *
**************************************************************************/

#pragma hdrstop
#include "CommPortDM.h"
#include "CommPortDM_Thread.h"

#define UART_MAX_READ_TIMEOUT          20
#define UART_DEFAULT_READ_TIMEOUT      10
#define UART_MIN_READ_TIMEOUT          5
#define	CAN_DATA_MESSAGE_ID			   0x03

#define DATA_BYTE_TO_LOAD		       0x100
#define DATA_BYTE_TO_READ		       0x80

#define ASC_INTERFACE			       0x01
#define CAN_INTERFACE			       0x02

#define BSL_PROGRAM_FLASH      0x00
#define BSL_RUN_FROM_FLASH     0x01
#define BSL_RUN_FROM_iCache    0x02
#define BSL_ERASE_FLASH        0x03
#define BSL_PROTECT_STATUS     0x04
#define BSL_PROTECT_FLASH      0x05


typedef enum device_type_t {
	XMC4500_1024_DEVICE = 0x1,
	XMC4500_768_DEVICE = 0x2,
	XMC4500_512_DEVICE = 0x3,
	XMC4400_512_DEVICE = 0x4,
	XMC4200_256_DEVICE = 0x5,
	XMC4800_1024_DEVICE = 0x6,
	XMC4800_1536_DEVICE = 0x7,
	XMC4800_2048_DEVICE = 0x8,
	STM32L4 = 0x9,
} device_type_t;


typedef enum bsl_error_codes_t {
	BSL_NO_ERROR = 0x00,
	ERROR_BSL_BLOCK_TYPE = 0xFF,
	ERROR_BSL_MODE = 0xFE,
	ERROR_BSL_CHECKSUM = 0xFD,
	ERROR_BSL_ADDRESS = 0xFC,
	ERROR_BSL_ERASE = 0xFB,
	ERROR_BSL_PROGRAM = 0xFA,
	ERROR_BSL_VERIFICATION = 0xF9,
	ERROR_BSL_PROTECTION = 0xF8,
	ERROR_BSL_BOOTCODE = 0xF7,
	ERROR_BSL_HEXDOWNLOAD = 0xF6,
	ERROR_BSL_UNKNOWN = 0xF0,
	ERROR_BSL_COMINIT = 0xF3,
	ERROR_BSL_INIT = 0xF2,
	ERROR_HEXFILE = 0xF1,
	ERROR_BSL_USB_TIMEOUT = 0xE9
} bsl_error_codes_t;

typedef struct BSL_HEADER {
	unsigned char mode;
	unsigned int startAddress;
	unsigned int sectorSize;
	unsigned int userPassword1;
	unsigned int userPassword2;
	unsigned char flashModule;
	unsigned short protectionConfig;

} BSL_HEADER;

typedef struct BSL_DATA {
	unsigned char* cDataArray;    // Pointer to the data to be loaded.
	unsigned char verification;
} BSL_DATA;

typedef struct BSL_DOWNLOAD {
	bool 			verbose;
	unsigned char 	device;
	unsigned char 	ser_interface;
	bool 			pFlash;
	char* hexFileName;
	unsigned int	hexFileSize;
	bool            IEC;
	unsigned char	HMI;
} BSL_DOWNLOAD;


unsigned int XMCLAPI_Init_Uart(CommPortDM* pcom, std::string stdstrPortName, DWORD dwBaudrate);
unsigned int XMCLAPI_Close_Interface(CommPortDM* pcom);
unsigned int XMCLAPI_Init_ASC_BSL(CommPortDM* com);
unsigned int XMCLAPI_Send_4_Length(CommPortDM* com, DWORD appLength);
unsigned int XMCLAPI_Send_ASCloader(CommPortDM* pcom, unsigned char* hexArray, unsigned int size);
unsigned int XMCLAPI_Bl_Send_Header(CommPortDM* pcom, BSL_HEADER bslHeader);
unsigned int XMCLAPI_Bl_Send_Data(CommPortDM* pcom, BSL_DATA data);
unsigned int XMCLAPI_Bl_Send_EOT(CommPortDM* pcom);
unsigned int XMCLAPI_Bl_Erase_Flash(void* handle, CommPortDM* pcom, BSL_DOWNLOAD bslDownload);
unsigned int XMCLAPI_Bl_Download_Pflash(void* handle, CommPortDM_Thread* thread, CommPortDM* pcom, BSL_DOWNLOAD bslDownload);
unsigned int XMCLAPI_Make_Flash_Image(const char* hexFileName, unsigned char* image, unsigned int max_size, unsigned int* address, unsigned int* num_of_bytes);
const char* XMCLAPI_Error_Message(unsigned int uiError);


