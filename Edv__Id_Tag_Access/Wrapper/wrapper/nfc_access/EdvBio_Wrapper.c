#include <utils.h>
#include <Tag_ICAO.h>
#include <string.h>

stSolemBio	g_SBIO;


__declspec(dllexport)
int Edv_Init()
{
    memset((void *)&g_SBIO, 0, sizeof(stSolemBio));

    return SolemICAOInit(&g_SBIO);
}


//typedef int(__cdecl* GETCARD)		(IN void* pDev);
//typedef int(__cdecl* TRANSMIT)		(IN void* pDev, IN unsigned char* ucpApduCmd, IN long lApduCmdLen, OUT unsigned char* ucpApduRsp, IN OUT long* lpApduRspLen);
//typedef int(__cdecl* DISCONNECT)	(IN void* pDev);




//typedef int (*TxRxNfc_Callback)(
//    void* handle,
//    unsigned char* tx,
//    int txLen,
//    unsigned char* rx,
//    int* rxLen
//    );

static TRANSMIT g_TxRxNfc = NULL;


//
// Insert Callback
//

__declspec(dllexport)
void Register_TxRxNfc_Callback(TRANSMIT cb)
{
    g_TxRxNfc = cb;
}
