#include <utils.h>
#include <Tag_ICAO.h>
#include <BIoGetData.h>
#include <string.h>
#include <secure_tools.h>
static stSolemBio   g_SBIO;
static TRANSMIT     g_TxRxNfc = NULL;



__declspec(dllexport)
int Edv_Init()
{
    memset((void *)&g_SBIO, 0, sizeof(stSolemBio));

    return SolemICAOInit(&g_SBIO);
}



//typedef int(__cdecl* GETCARD)		(IN void* pDev);
//typedef int(__cdecl* TRANSMIT)		(IN void* pDev, IN unsigned char* ucpApduCmd, IN long lApduCmdLen, OUT unsigned char* ucpApduRsp, IN OUT long* lpApduRspLen);
//typedef int(__cdecl* DISCONNECT)	(IN void* pDev);

//
// Insert Callback
//
__declspec(dllexport)
void Register_TxRxNfc_Callback(TRANSMIT cb)
{
    stSolemICAO* pSolemICAO = (stSolemICAO*)g_SBIO.hSolemICAO;
    
    g_TxRxNfc = cb;

    pSolemICAO->fn_Transmit = cb;





    //g_SBIO.hSolemICAO->fn_GetCard = cb;

    //GETCARD		fn_GetCard;
    //TRANSMIT	fn_Transmit;
    //DISCONNECT	fn_Disconnect;
}

__declspec(dllexport)
int Edv_Moc(unsigned char* docNum, unsigned char* DoB, unsigned char*  DoE)
{
    stSolemICAO* pSolemICAO = (stSolemICAO*)g_SBIO.hSolemICAO;
    int match = 100;
    int status;

    //string qr_rafa = "https://portal.sidiv.registrocivil.cl/docstatus?RUN=12845657-0&type=CEDULA&serial=B5F089055&mrz=B5F089055075040793504071&name=RAFAEL%20SEBASTI%C1N%20%20ARANCIBIA%20AMPUERO";

    char data_rafa[] = "B5F089055075040793504071";

    memcpy((void *)(pSolemICAO->sICAOKey), (void *)data_rafa, sizeof(data_rafa));

    //long ConnectCard(stSolemICAOPtr opSICAO, void* pDev, int appICAO)
    //int SolemICAOMOC(stSolemICAOPtr opSICAO, int iDeviceID, int iFingerRef, int iFieldID, int* ipMatchResult)


    //return (int)ConnectCard(g_SBIO.hSolemICAO, NULL, 1);

    status = SolemICAOMOC(g_SBIO.hSolemICAO, 0, 1, 0, &match);

    PutInLog(NULL, LOG_LEVEL_NOTICE, (char*)"MOC RESUUUUUUUUUUULTS : %d\r\n", match); 

    return status;

}

__declspec(dllexport)
void Edv_Set_Template(unsigned char* template, int template_len)
{
    sBioSetTemplate(template, template_len);
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




}


