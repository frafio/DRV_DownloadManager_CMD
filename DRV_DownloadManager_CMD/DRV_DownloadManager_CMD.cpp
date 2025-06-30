// DRV_DownloadManager_CMD.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <string>
#include "CommPortDM_Thread.h"
#include <chrono>

using namespace std;

#define COM_CHECK_TIMEOUT			10

#define INFO_IEC_UPGRADE_END_OK		1
#define INFO_IEC_UPGRADE_END_ERR	2
#define INFO_IEC_ERASE_END_OK		3
#define INFO_IEC_ERASE_END_ERR		4
#define INFO_FW_UPGRADE_END_OK		5
#define INFO_FW_UPGRADE_END_ERR		6
enum {
	CMD_LINE_ERR_NONE,
	CMD_LINE_ERR_WRONG_PAR,
	CMD_LINE_ERR_ACTION_FAIL,
};

enum {
	CMD_LINE_NONE,
	CMD_LINE_UPDATE_IEC_APP,
	CMD_LINE_ERASE_IEC_APP,
	CMD_LINE_UPDATE_FW,
	CMD_LINE_UPDATE_FW_CRD,
};

typedef enum {
	START_NONE,
	START_FW_UPGRADE,
	START_IEC_ERASE,
	START_IEC_UPGRADE,
	START_HMI_UPGRADE,
	START_HMI_UGDATA,
} Delayed_Process_To_Start_t;

bool getComPorts(string com_port);

int main(int argc, char* argv[]) {

	bool success = false;
	double duration = 10;


	// Simulate some work


	if (argc != 4) {
		std::cerr << "Usage: DRV_DownloadManaager_CMD <COM> <type> <file_path>" << std::endl;
		std::cerr << "Where type : -p :PLC, -h :HMI, -d :DATA_HMI" << std::endl;
		return 1;
	}
	const char* com_port = argv[1];
	const char* type_file = argv[2];
	const char* filePath = argv[3];

	bool port_found = false;

	CommPortDM_Thread thread(com_port, filePath);

	std::cerr << "Selected " << com_port << " file: " << filePath << std::endl;
	auto start_timer = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_timer{};

	std::cerr << "Please connect USB and turn board on in 10 secs" << std::endl;
	
	while (port_found != true && COM_CHECK_TIMEOUT > (elapsed_timer.count())) {
	
		port_found = getComPorts(com_port);

		auto end_timer = std::chrono::high_resolution_clock::now();
		elapsed_timer = end_timer - start_timer;

	}
	if (port_found) {
		switch (type_file[1])
		{

		case 'p':
			if (thread.IECAppUpgrade(true) == 0) success == true;
			break;
		case 'h':
			//thread.StartHMIUpgrade(nullptr, com_port, filePath);
			if (thread.HMIAppUpgrade(true) == 0) success == true;
			break;
		case 'd':
			//thread.StartHMIUgData(nullptr, com_port, filePath);
			if (thread.HMIDataUpgrade(true) == 0) success == true;
			break;

		default:
			success = false;
			std::cerr << "Invalid type file" << std::endl;
			break;
		}
	
	} 
	else if (elapsed_timer.count() >= COM_CHECK_TIMEOUT) {
		std::cerr << "Time is up! Selected " << com_port << " NOT FOUND" << std::endl;
	}
	else {
		std::cerr << "Something gone wrong. " << std::endl;
	}



	if (success == true) {
		std::cout << "Download succeeded." << std::endl;
	}
	else {
		std::cerr << "Download failed." << std::endl;
		return 2;
	}

	return 0;
}


CommPortDM_Thread threadUpdate;
CommPortDM* serialport_com = nullptr;

#pragma comment (lib, "OneCore.lib")


		   bool getComPorts(string com_port) {
			   unsigned long portNumbers[256];
			   unsigned long  portNumbersFound;
			   unsigned long  portNumbersCount = 100;

			   bool found_port = false;

			   unsigned long getCommPortsOutput = _WINBASE_::GetCommPorts(portNumbers, portNumbersCount, &portNumbersFound);
			   // Get the list of COM ports
			   if (getCommPortsOutput == ERROR_SUCCESS) {

				   if (portNumbersFound > 0) {
					   //label1->Text = "";
					   for (unsigned long i = 0; i < portNumbersFound; ++i) {
						   //label1->Text += "COM" + portNumbers[i] + "\n";
						   string tmp_sys_str = "COM";
						   tmp_sys_str += std::to_string(portNumbers[i]);

						   if (com_port.compare(tmp_sys_str) == 0) {

							   std::cerr << "Port " << tmp_sys_str << " found." << std::endl;
							   found_port = true;
							   break;
						   }
					   }
				   }


			   }
			   if (found_port == false) {
				   //std::cerr << "Something wrong. Stopped." << std::endl;
			   }
			   return found_port;
		   }

// Per eseguire il programma: CTRL+F5 oppure Debug > Avvia senza eseguire debug
// Per eseguire il debug del programma: F5 oppure Debug > Avvia debug

// Suggerimenti per iniziare: 
//   1. Usare la finestra Esplora soluzioni per aggiungere/gestire i file
//   2. Usare la finestra Team Explorer per connettersi al controllo del codice sorgente
//   3. Usare la finestra di output per visualizzare l'output di compilazione e altri messaggi
//   4. Usare la finestra Elenco errori per visualizzare gli errori
//   5. Passare a Progetto > Aggiungi nuovo elemento per creare nuovi file di codice oppure a Progetto > Aggiungi elemento esistente per aggiungere file di codice esistenti al progetto
//   6. Per aprire di nuovo questo progetto in futuro, passare a File > Apri > Progetto e selezionare il file con estensione sln
