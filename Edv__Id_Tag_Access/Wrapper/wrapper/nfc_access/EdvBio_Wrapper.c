#include <utils.h>
#include <Tag_ICAO.h>
#include <BIoGetData.h>
#include <string.h>
#include <secure_tools.h>
#include <Client_Info.h>

static stHndBio     g_SBIO;
static TRANSMIT     g_TxRxNfc = NULL;
static GETCARD      g_Card_Get = NULL;
static DISCONNECT   g_Carf_Disconnect = NULL;


__declspec(dllexport)
int Edv_Init()
{
    memset((void *)&g_SBIO, 0, sizeof(stHndBio));

    return ICAOInit(&g_SBIO);
}


__declspec(dllexport)
int Edv_Prueba_Connect(int appIcao)
{
    stHndICAO* pHndICAO = (stHndICAO*)g_SBIO.HndICAO;

    if(pHndICAO == NULL)
    {
        return 120;
    }

    return (int)ConnectCard(pHndICAO, NULL, appIcao);
}




//
// Callback Lector NFC
//
__declspec(dllexport)
void Register_TxRxNfc_Callback(TRANSMIT cb)
{
    stHndICAO* pHndICAO = (stHndICAO*)g_SBIO.HndICAO;
    
    g_TxRxNfc = cb;

    pHndICAO->fn_Transmit = cb;
}

__declspec(dllexport)
void Register_Nfc_Tag_Detect_Callback(GETCARD cb)
{
    stHndICAO* pHndICAO = (stHndICAO*)g_SBIO.HndICAO;

    g_Card_Get = cb;

    pHndICAO->fn_GetCard = cb;
}

__declspec(dllexport)
void Register_Nfc_Tag_Diconnect_Callback(DISCONNECT cb)
{
    stHndICAO* pHndICAO = (stHndICAO*)g_SBIO.HndICAO;

    g_Carf_Disconnect = cb;

    pHndICAO->fn_Disconnect = cb;
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
int Edv_Licencia_Get_Client_Info(unsigned char* path_public_key, unsigned char *clientInfo, int *infoLen)
{
    int status = 0;
    char hwid[256] = { 0 };
 
    unsigned char   encrypted[1024 * 4];
    int             encrypted_total = 0;
    unsigned char   encriptado_b64[10 * 104];
    int             encriptado_b64_len = sizeof(encriptado_b64);


    while(TRUE)
    {
        // Pasos a seguir:
        // 1. hwid contiene una cadena.
        // 2. Se calcula un hash256 de hwid. Esto genera 32 bytes
		// 3. Se concatena el hash con hwid (hash || hwid). Esto genera 32 + strlen(hwid) bytes.
        // 4. Se encripta el hash con llave pública.
        // 5. El resultado de encriptación se pasa a base64.

        // Paso 1            
        status = build_hwid(hwid);

        if(status != 0)
        {
            PutInLog(NULL, LOG_LEVEL_ERROR, "Cannot get hardware info : %d", status);
            status = 1;
            break;
        }

        PutInLog(NULL, LOG_LEVEL_INFORMATIONAL, "HWID : %s", hwid);

        // Paso 2
        unsigned char hash[32];
        if(SHA256((unsigned char*)hwid, strlen(hwid), hash) == 0)
        {
            status = 2;
            break;
        }
        
        // Paso 3
        unsigned char baTmp[sizeof(hash) + sizeof(hwid)];
        int baTmp_Len = sizeof(hash) + strlen(hwid);

        memcpy((void *)baTmp, (void *)hash, sizeof(hash));
        memcpy((void*)(baTmp + sizeof(hash)), (void*)hwid, strlen(hwid));

        // Paso 4
        size_t len = 0;
        status = rsa_encrypt(baTmp, baTmp_Len, encrypted, &len, path_public_key);
        if (status != 0)
        {
            status = 3;
            break;
        }

        encrypted_total = (int)len;

        // Paso 5
        status = Base64Encode(encrypted, encrypted_total, clientInfo, infoLen);
        if (status != 0)
        {
            status = 4;
            break;
        }

        status = 0;
    
        break;
    }

    return status;
}

__declspec(dllexport)
int Edv_License__Add_License(unsigned char* path_license, unsigned char* path_public_key)
{
	int status = 0;

    while (TRUE)
    {
        status = Add_User_License(path_license, path_public_key);
        break;
    }

	return status;
}