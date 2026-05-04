//
// Created by Rafael on 09/03/2025.
//

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <Tag_ICAO.h>

#define MAX_UTIL_MSG_LEN    1024
#define MAX_NUMCOL          16

enum
{
    LOG_LEVEL_EMERGENCY     = 0,
    LOG_LEVEL_ALERT         = 1,
    LOG_LEVEL_CRITICAL      = 2,
    LOG_LEVEL_ERROR         = 3,
    LOG_LEVEL_WARNING       = 4,
    LOG_LEVEL_NOTICE        = 5,
    LOG_LEVEL_INFORMATIONAL = 6,
    LOG_LEVEL_DEBUG         = 7
};

typedef void (*Log_Callback)(
    const unsigned char* infoBuffer,
    int infoBufferLen
    );

typedef void* stLoggerPtr;


void PutInLog(void *logger, int iLevel, char *sMsg, ...);
void DisplayHex(int iLevel, unsigned char *ucpBuffer, unsigned int lLenBuffer);

void ReverseArray(unsigned char* aData, int iStart, int iLen);
int32_t bytesToInt32(const unsigned char* buffer, int startIndex);
int16_t bytesToInt16(const unsigned char* buffer, int offset);

#endif //__UTILS_H__
