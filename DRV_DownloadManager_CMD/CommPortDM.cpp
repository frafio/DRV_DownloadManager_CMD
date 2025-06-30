#include "CommPortDM.h"
#include "CommPortDMError.h"

	CommPortDM::CommPortDM() :m_CommPortDM_Open(false), m_CommPortDM_COM(""), m_CommPortDM_Handle(0) {

		m_CommPortDM_dcb.DCBlength = sizeof(DCB);
		m_CommPortDM_dcb.BaudRate = 57600;
		//m_CommPortDM_dcb.ByteSize = 8;
		m_CommPortDM_dcb.ByteSize = DATABITS_8;
		m_CommPortDM_dcb.Parity = NOPARITY;    //NOPARITY and friends are #defined in windows.h
		m_CommPortDM_dcb.StopBits = ONESTOPBIT;  //ONESTOPBIT is also from windows.h
		m_CommPortDM_dcb.fDtrControl = 0;
		m_CommPortDM_dcb.fRtsControl = 0;

	}

	/*
	CommPortDM::CommPortDM(std::string com_str, DWORD baud ) {
		m_CommPortDM_Open = false;
		m_CommPortDM_COM = com_str;
		m_CommPortDM_dcb.DCBlength = sizeof(DCB);
		m_CommPortDM_dcb.BaudRate = baud;
		m_CommPortDM_dcb.StopBits = ONESTOPBIT;
		m_CommPortDM_dcb.Parity = PARITY_NONE;
		m_CommPortDM_dcb.ByteSize = DATABITS_8;
		m_CommPortDM_dcb.fDtrControl = 0;
		m_CommPortDM_dcb.fRtsControl = 0;
	}
	*/
	CommPortDM::~CommPortDM() {
		if (m_CommPortDM_Open)
			CloseCommPort();
	}



	void CommPortDM::OpenCommPort() {

		if (m_CommPortDM_Open) {
			return;
		}

		DCB tempDCB;
		tempDCB.BaudRate = m_CommPortDM_dcb.BaudRate;
		tempDCB.ByteSize = m_CommPortDM_dcb.ByteSize;
		tempDCB.Parity = m_CommPortDM_dcb.Parity;
		tempDCB.StopBits = m_CommPortDM_dcb.StopBits;

		std::string tmp_prefix_COM = "\\\\.\\" + m_CommPortDM_COM;

		std::wstring wstr(tmp_prefix_COM.begin(), tmp_prefix_COM.end()); // Convert to wide string
		LPCWSTR lpwstr = wstr.c_str();
		m_CommPortDM_Handle = CreateFile(lpwstr,
			GENERIC_READ | GENERIC_WRITE,
			0,    /* comm devices must be opened w/exclusive-access */
			NULL, /* no security attrs */
			OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
			0,    /* not overlapped I/O */
			NULL  /* hTemplate must be NULL for comm devices */
		);
		//serialHandle = CreateFile(com_port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
		//	FILE_ATTRIBUTE_NORMAL, 0);

		if (m_CommPortDM_Handle == INVALID_HANDLE_VALUE) {

			throw CommPortDMError::CommPortDMError(CommPortDMError::OPEN_ERROR);
		}

		// Do some basic settings

		if (!GetCommState(m_CommPortDM_Handle, &m_CommPortDM_dcb))
		{
			// something is hay wire, close the port and return
			CloseHandle(m_CommPortDM_Handle);
			throw CommPortDMError::CommPortDMError(CommPortDMError::GETCOMMSTATE);
		}
		m_CommPortDM_dcb.BaudRate = tempDCB.BaudRate;
		m_CommPortDM_dcb.ByteSize = tempDCB.ByteSize;
		m_CommPortDM_dcb.Parity = tempDCB.Parity;
		m_CommPortDM_dcb.StopBits = tempDCB.StopBits;
		m_CommPortDM_dcb.fOutX = false;
		m_CommPortDM_dcb.fInX = false;
		m_CommPortDM_dcb.fDtrControl = 0;
		m_CommPortDM_dcb.fRtsControl = 0;

		/*
		// dcb contains the actual port properties.  Now copy our settings into this dcb
		*/
		if (!SetCommState(m_CommPortDM_Handle, &m_CommPortDM_dcb))
		{
			// something is hay wire, close the port and return
			CloseHandle(m_CommPortDM_Handle);
			throw CommPortDMError::CommPortDMError(CommPortDMError::SETCOMMSTATE);
		}

		// set the intial size of the transmit and receive queues. These are
		// not exposed to outside clients of the class either. Perhaps they should be?
		// I set the receive buffer to 32k, and the transmit buffer to 9k (a default).
		if (!SetupComm(m_CommPortDM_Handle, 1024 * 32, 1024 * 9))
		{
			// something is hay wire, close the port and return
			CloseHandle(m_CommPortDM_Handle);
			throw CommPortDMError::CommPortDMError(CommPortDMError::SETUPCOMM);
		}
		m_CommPortDM_TimeOuts.ReadIntervalTimeout = 15;
		m_CommPortDM_TimeOuts.ReadTotalTimeoutMultiplier = 1;
		m_CommPortDM_TimeOuts.ReadTotalTimeoutConstant = 250;
		m_CommPortDM_TimeOuts.WriteTotalTimeoutMultiplier = 1;
		m_CommPortDM_TimeOuts.WriteTotalTimeoutConstant = 250;
		if (!SetCommTimeouts(m_CommPortDM_Handle, &(m_CommPortDM_TimeOuts)))
		{
			// something is hay wire, close the port and return
			CloseHandle(m_CommPortDM_Handle);
			throw CommPortDMError::CommPortDMError(CommPortDMError::SETCOMMTIMEOUTS);
		}

		// if we made it to here then success
		m_CommPortDM_Open = true;
	}


	void CommPortDM::CloseCommPort(void) {
		if (!m_CommPortDM_Open)       // if already closed, return
			return;

		if (CloseHandle(m_CommPortDM_Handle) != 0) // CloseHandle is non-zero on success
		{
			m_CommPortDM_Open = false;
		}
		else
			throw CommPortDMError::CommPortDMError(CommPortDMError::CLOSE_ERROR);
	}


	int CommPortDM::WriteBuffer(BYTE* buffer, unsigned int ByteCount) {
		VerifyOpen();
		DWORD dummy;
		if ((ByteCount == 0) || (buffer == NULL))
			return 0;

		if (!WriteFile(m_CommPortDM_Handle, buffer, ByteCount, &dummy, NULL))
			throw CommPortDMError::CommPortDMError(CommPortDMError::WRITE_ERROR);

		return (int)dummy;
	}

	void CommPortDM::SetBaudRate(unsigned int newBaud)
	{
		unsigned int oldBaudRate = m_CommPortDM_dcb.BaudRate; // make a backup of the old baud rate
		m_CommPortDM_dcb.BaudRate = newBaud;                // assign new rate

		if (m_CommPortDM_Open)                     // check for open comm port
		{
			if (!SetCommState(m_CommPortDM_Handle, &m_CommPortDM_dcb))   // try to set the new comm settings
			{                                      // if failure
				m_CommPortDM_dcb.BaudRate = oldBaudRate;        // restore old baud rate
				throw CommPortDMError(CommPortDMError::BAD_BAUD_RATE);     // bomb out
			}
		}
	}

	void CommPortDM::SetCommPort(std::string port)
	{
		VerifyClosed();   // can't change comm port once comm is open
		// could close and reopen, but don't want to

		m_CommPortDM_COM = port;
	}


	void CommPortDM::FlushCommPort(void) {
		VerifyOpen();

		if (!FlushFileBuffers(m_CommPortDM_Handle))
			throw CommPortDMError::CommPortDMError(CommPortDMError::FLUSHFILEBUFFERS);

	}
	int CommPortDM::ReadBytes(BYTE* buffer, unsigned int MaxBytes) {

		VerifyOpen();
		DWORD bytes_read;
		int error;

		if (!ReadFile(m_CommPortDM_Handle, buffer, MaxBytes, &bytes_read, NULL)) {
			error = GetLastError();
			throw CommPortDMError::CommPortDMError(CommPortDMError::READ_ERROR);
		}

		// add a null terminate if bytes_read < byteCount
		// if the two are equal, there is no space to put a null terminator
		if (bytes_read < MaxBytes && bytes_read > 0)
			buffer[bytes_read] = '\0';

		return bytes_read;
	}
