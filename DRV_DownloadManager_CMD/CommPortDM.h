#pragma once

#ifndef COMMPORTDM_H
#define COMMPORTDM_H

#include <tchar.h>
#include <string>
#include <windows.h>
#include <iostream>
#include "CommPortDMError.h"

class CommPortDM
{

public:
    CommPortDM();
    ~CommPortDM();
    void OpenCommPort();
    void CloseCommPort(void);
    int WriteBuffer(BYTE* buffer, unsigned int ByteCount);
    void FlushCommPort(void);
    void SetBaudRate(unsigned int newBaud);
    void ForceClosePort(void);
    void SetCommPort(std::string port);
    int ReadBytes(BYTE* buffer, unsigned int MaxBytes);

    bool GetConnected()
    {
        return m_CommPortDM_Open;
    }

    void* GetHandle() 
    { // allow access to the handle in case the user needs to do something hardcore. Avoid this if possible
        return m_CommPortDM_Handle;
    }
    bool isOpen() {
        return m_CommPortDM_Open;
    }

    void VerifyOpen()
    {
        if (!m_CommPortDM_Open) {
            std::cerr << "ERROR PORT_NOT_OPEN" << std::endl;
            throw CommPortDMError::CommPortDMError(CommPortDMError::PORT_NOT_OPEN);
        }
    }

private:
    /* NOTE FROM TCOMM LIB */
    // Note: the destructor of the commport class automatically closes the
    //       port. This makes copy construction and assignment impossible.
    //       That is why I privatize them, and don't define them. In order
    //       to make copy construction and assignment feasible, we would need
    //       to employ a reference counting scheme.
    CommPortDM(const CommPortDM&);            // privatize copy construction
    CommPortDM& operator=(const CommPortDM&);  // and assignment.

    void VerifyClosed()
    {
        if (m_CommPortDM_Open){
            std::cerr << "ERROR PORT_ALREADY_OPEN" << std::endl;
                throw CommPortDMError::CommPortDMError(CommPortDMError::PORT_ALREADY_OPEN);
            }
    }
    bool          m_CommPortDM_Open;
    //COMMTIMEOUTS  m_CommPortDM_TimeOuts;
    std::string   m_CommPortDM_COM;
    DCB           m_CommPortDM_dcb;        // a DCB is a windows structure used for configuring the port
    void*        m_CommPortDM_Handle;       // handle to the comm port.

};
#endif