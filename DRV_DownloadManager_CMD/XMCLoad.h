#include <tchar.h>
#include "CommPortDM_Thread.h"
#include <minwinbase.h>
#include <minwindef.h>

int XMCLoad(void* handle, CommPortDM_Thread* thread, CommPortDM* pcom, const char* fileHex, bool IEC, bool st, unsigned char hmi);
long XMLoadGetFileSize(const char* filePath);

