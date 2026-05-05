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
int Edv_Get_DgData(int timeout_ms)
{
    stHndICAO* pHndICAO = (stHndICAO*)g_SBIO.HndICAO;

    pHndICAO->iGetFinger    = TRUE;
    pHndICAO->iGetICAO      = TRUE;

    return ICAOCapture(pHndICAO, 0, timeout_ms);
}

__declspec(dllexport)
int Edv__Set_Tag_Info(unsigned char* docNum, int docnum_len, unsigned char* DoB, int dob_len, unsigned char* DoE, int doe_len)
{
    stHndICAO* pHndICAO = (stHndICAO*)g_SBIO.HndICAO;
    int     status = 0;
    char    IcaoKey[sizeof(pHndICAO->sICAOKey)] = { 0 };


    while (TRUE)
    {
        if (docNum == NULL || docnum_len <= 0)
        {
            status = 1000;
            break;
        }

        if (DoB == NULL || dob_len <= 0)
        {
            status = 1001;
            break;
        }

        if (DoE == NULL || doe_len <= 0)
        {
            status = 1002;
            break;
        }

        if ((docnum_len + dob_len + doe_len) > sizeof(pHndICAO->sICAOKey))
        {
            status = 1003;
            break;
        }

        memcpy((void*)IcaoKey, (void*)docNum, docnum_len);
        memcpy((void*)(IcaoKey + docnum_len), (void*)DoB, dob_len);
        memcpy((void*)(IcaoKey + docnum_len + dob_len), (void*)DoE, doe_len);

        memcpy((void*)(pHndICAO->sICAOKey), (void*)IcaoKey, docnum_len + dob_len + doe_len);
        break;
    }

    return status;
}

__declspec(dllexport)
int Edv_Moc()
{
    stHndICAO* pHndICAO = (stHndICAO*)g_SBIO.HndICAO;
    int match = 100;

    return ICAOMOC(g_SBIO.HndICAO, 0, 1, 0, &match);
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