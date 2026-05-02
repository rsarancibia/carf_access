#include <utils.h>
#include <Tag_ICAO.h>
#include <BIoGetData.h>
#include <string.h>
#include <secure_tools.h>
#include <Client_Info.h>

static stHndBio     g_SBIO;
static TRANSMIT     g_TxRxNfc = NULL;

//typedef int(__cdecl* GETCARD)		(IN void* pDev);
static GETCARD      g_Card_Get = NULL;

//typedef int(__cdecl* DISCONNECT)	(IN void* pDev);
static DISCONNECT   g_Carf_Disconnect = NULL;


__declspec(dllexport)
int Edv_Init()
{
    memset((void *)&g_SBIO, 0, sizeof(stHndBio));

    return ICAOInit(&g_SBIO);
}

//
// Insert Callback
//
__declspec(dllexport)
void Register_TxRxNfc_Callback(TRANSMIT cb)
{
    stHndICAO* pHndICAO = (stHndICAO*)g_SBIO.HndICAO;
    
    g_TxRxNfc = cb;

    pHndICAO->fn_Transmit = cb;
}

__declspec(dllexport)
int Edv_Moc(unsigned char* docNum, unsigned char* DoB, unsigned char*  DoE) 
{
    stHndICAO* pHndICAO = (stHndICAO*)g_SBIO.HndICAO;
    int match = 100;
    int status;

    //string qr_rafa = "https://portal.sidiv.registrocivil.cl/docstatus?RUN=12845657-0&type=CEDULA&serial=B5F089055&mrz=B5F089055075040793504071&name=RAFAEL%20SEBASTI%C1N%20%20ARANCIBIA%20AMPUERO";

    char data_rafa[] = "B5F089055075040793504071";

    memcpy((void *)(pHndICAO->sICAOKey), (void *)data_rafa, sizeof(data_rafa));

    status = ICAOMOC(g_SBIO.HndICAO, 0, 1, 0, &match);

    PutInLog(NULL, LOG_LEVEL_NOTICE, (char*)"MOC RESUUUUUUUUUUULTS : %d\r\n", match); 

    return status;

}

__declspec(dllexport)
void Edv_Set_Template(unsigned char* template, int template_len)
{
    sBioSetTemplate(template, template_len);
}

__declspec(dllexport)
int Edv_Licencia_Get_Client_Info(unsigned char *clientInfo, int *infoLen)
{
    int status = 0;
    char hwid[256] = { 0 };
 
    status = build_hwid(hwid);
    
    if (status == 0)
    {
        unsigned char hash[32];
        SHA256((unsigned char*)hwid, strlen(hwid), hash);

        unsigned char b64[64];
        int b64_len = sizeof(b64);
        Base64Encode(hash, sizeof(hash), b64, &b64_len);

        *infoLen = b64_len;

        memcpy((void*)clientInfo, (void*)b64, *infoLen);
    }

    return status;
}




__declspec(dllexport)
void Edv_Test_Licencia(void)
{
    int status;
    char Buffer[1024]; 

    test_rsa_sign_verify();


    status = get_http_date(Buffer, sizeof(Buffer));

    if(status == 0)
    {
        PutInLog(NULL, LOG_LEVEL_NOTICE, "ERROR captura de Time from Inet : %d", status);
    }
    else
    {
        DisplayHex(0, Buffer, 128);
    }

    /*
    
    Si devuelve 0, puedes saber por qué falló con:
    DWORD err = GetLastError();
    
    
    Ejemplos típicos:

    ERROR_WINHTTP_HEADER_NOT_FOUND
    ERROR_WINHTTP_INSUFFICIENT_BUFFER
    ERROR_WINHTTP_INVALID_HEADER
    
    */

	char hwid[256] = { 0 };
    
    status = build_hwid(hwid);
    

    PutInLog(NULL, LOG_LEVEL_NOTICE, "HWID: %s", hwid);
    DisplayHex(0, hwid, strlen(hwid));

}


