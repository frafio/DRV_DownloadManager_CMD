#pragma once 

class CommPortDMError
{

public:
    enum ErrorType
    {
        BAD_SERIAL_PORT,
        BAD_BAUD_RATE,
        BAD_PORT_NUMBER,
        BAD_STOP_BITS,
        BAD_PARITY,
        BAD_BYTESIZE,
        PORT_ALREADY_OPEN,
        PORT_NOT_OPEN,
        OPEN_ERROR,
        WRITE_ERROR,
        READ_ERROR,
        CLOSE_ERROR,
        PURGECOMM,
        FLUSHFILEBUFFERS,
        GETCOMMSTATE,
        SETCOMMSTATE,
        SETUPCOMM,
        SETCOMMTIMEOUTS,
        CLEARCOMMERROR
    };
    typedef  ErrorType ErrorType_t;

    CommPortDMError(ErrorType_t error);
    CommPortDMError() = default;
};