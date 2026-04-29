//
// Created by Rafael on 09/03/2025.
//

#include <utils.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

static Log_Callback g_Log = NULL;

//
// insert callback
//
__declspec(dllexport)
void Register_Log_callback(Log_Callback cb)
{
	g_Log = cb;
}

void PutInLog(void *logger, int iLevel, char *sMsg, ...)
{
	char Buffer[1024 * 2];
	char LogTag[16];
	 
	 switch(iLevel)
	 {
     case LOG_LEVEL_EMERGENCY:
			sprintf(LogTag,"EMER");	
			break;
	 case LOG_LEVEL_ALERT:
			sprintf(LogTag,"ALRT");	
			break;
	 case LOG_LEVEL_CRITICAL:
			sprintf(LogTag,"CRIT");	
			break;
	 case LOG_LEVEL_ERROR:
			sprintf(LogTag,"ERR ");	
			break;
	 case LOG_LEVEL_WARNING:
			sprintf(LogTag,"WARN");	
			break;
	 case LOG_LEVEL_NOTICE:
			sprintf(LogTag,"NOTC");	
			break;
	 case LOG_LEVEL_INFORMATIONAL:
			sprintf(LogTag,"INF ");	
			break;
	 case LOG_LEVEL_DEBUG:
			sprintf(LogTag,"DEB ");	
			break;
         default:
			sprintf(LogTag,"UNKW");	
			break;
     }

	sprintf(LogTag,"EMER");	

			
	time_t now = time(NULL);
    
	struct tm* t = localtime(&now);

	sprintf(Buffer, "[%02d:%02d:%02d (WRP) %s] ", t->tm_hour, t->tm_min, t->tm_sec, LogTag);

    va_list ap;
    va_start(ap, sMsg);
	vsprintf(Buffer + strlen(Buffer), sMsg, ap);
    va_end(ap);

    //
    // Acá, Buffer contiene el mensaje a mostrar
    // Luego, se puede almacenar en archivo
    //

     //
     // Envìa a logCat
     //

	if(g_Log != NULL)
	{
		g_Log(Buffer, strlen(Buffer));
	}
}


void DisplayHex(int iLevel, unsigned char *ucpBuffer, unsigned int lLenBuffer)
{
    unsigned int		uTmp1;
    char				sHex[4];
    char				sChar[2];
    unsigned char		cTmp1;
    char				sTmp1[MAX_UTIL_MSG_LEN + 1];
    char				sTmp2[MAX_UTIL_MSG_LEN + 1];

    char				sTmp2Display[MAX_UTIL_MSG_LEN + 1];


    memset(sTmp1, 0, sizeof(sTmp1));
    memset(sTmp2, 0, sizeof(sTmp2));
    for (uTmp1 = 0; uTmp1 < lLenBuffer; uTmp1++)
    {
        cTmp1 = ucpBuffer[uTmp1];
        sprintf(sHex, "%02X ", cTmp1);
        sprintf(sChar,"%c", ((cTmp1 >= 32 && cTmp1 <= 36) || (cTmp1 >= 38 && cTmp1 <= 91) || (cTmp1 >= 93 && cTmp1 <= 125)) ? cTmp1 : '.');
        strcat(sTmp1, sHex);
        strcat(sTmp2, sChar);

        if (((uTmp1 + 1) % MAX_NUMCOL) == 0)
        {
            sprintf(sTmp2Display, "       %s%*s%s", sTmp1, (int)(5 + MAX_NUMCOL * 3 - strlen(sTmp1)), (char *)" ", sTmp2);

            PutInLog(NULL, iLevel, sTmp2Display);

            memset(sTmp1, 0, sizeof(sTmp1));
            memset(sTmp2, 0, sizeof(sTmp2));
        }
    }
    if ((uTmp1 % MAX_NUMCOL) != 0)
    {
        sprintf(sTmp2Display, "       %s%*s%s", sTmp1, (int)(5 + MAX_NUMCOL * 3 - strlen(sTmp1)), (char *)" ", sTmp2);

        PutInLog(NULL, iLevel, sTmp2Display);
    }
}

void ReverseArray(unsigned char* aData, int iStart, int iLen)
{
    if(iStart < 0)
        return;

    if(iLen < 0)
        return;

    unsigned char ucTmp;
    int j;
    for(int i = 0; i < iLen / 2; i++)
    {
        j = iStart + iLen - 1 - i;
        ucTmp = aData[iStart + i];
        aData[iStart + i] = aData[j];
        aData[j] = ucTmp;
    }
}

int32_t bytesToInt32(const unsigned char* buffer, int startIndex) {
    return (int32_t)(
            (buffer[startIndex]) |
            (buffer[startIndex + 1] << 8) |
            (buffer[startIndex + 2] << 16) |
            (buffer[startIndex + 3] << 24)
    );
}

int16_t bytesToInt16(const unsigned char* buffer, int offset) {
    return (int16_t)(
            (buffer[offset]) |
            (buffer[offset + 1] << 8)
    );
}
