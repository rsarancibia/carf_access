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

static const char* TEST_PRIVATE_KEY_PEM =
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDtRE4Ovdlxe9Rw\n"
"0qrdB4Uy/dKTKTwOshjlsAeyMjoXEuLI8Tpf/CFkrjBWzxO3IkKhaAhWHttZiQ1j\n"
"kLekCqKtpzeU5vpip7LYocIEwhroZCziiIf8pO08SHnyCcJ8/QpDpCPjuqmsftbr\n"
"am1GIIpWzt7+7mNdBThU7HHH7CMNzbquM6heLS3t0HUhh3dchHFxTN5kE/FaD9/3\n"
"m/ZBHxgDvLzkcGNwlMBHd6fTSDZdTc6M4qizPkYVT53WtQEdOitPqt6AjDktiQ4/\n"
"eM34f2Cp5GRLm4CYJ3rkJor5OVsyfL7cZmI1iPbeTET1xtrrMmPJE21PIY9joxTL\n"
"oUHKKpwpAgMBAAECggEAEU/i8aqq0V/XiwFmF/FO597zFMuFDy3xyN2YnCkTyb/2\n"
"JfMK/zqsfZYDnyvmHH5OlgwbEpyXBPAN9uo16g1gHBNXyAC3+4eQskOQPmxizfrU\n"
"NxW+9WFb0YKYyj8pzpigfXm+KCVEMNrC6AO4lC225llbFkcS3zRMrU6gyTV6w8yP\n"
"c+7o7t9C3GfELqv2DerwWwQI4eXndg3MuBnyMY7/ud9q24MtEtPFbObLoiUCTnmi\n"
"ZV/gKC12uiSSpyRA9UhRPBwCP7hcVn99N3mUCXvCn5BtGSYvvN5YfAdBB1hn38lH\n"
"jd+BpiMUnhz/1RtU5dIr5K0EEqtJ7dJOVL+TmfVvEQKBgQD9lP7+Zx4doLAuo7JG\n"
"KaeAHRoXzZ3Ccfqgg/373g/Rfsar5gsds7QDhqNPMJrIMhUw7YInpo4Yoqv8/Fk8\n"
"K5IszJ6FNPgDCnMdljynXoZ+Yz2yhDSXYMgwwtb6DrCtSgJzmTU6gjkMXRzPQZBz\n"
"ZizhophlMBivtO+tEtCnvU8m0QKBgQDvh3uXm31H4CXU5+tG36foaTXQCJ4PspgB\n"
"zdsPRbEcg6fAcKG/4PuEr4aTzwtH2gY9MPjl2qH7euhg+vdx9kdsBBbUS0x4h1On\n"
"rkWGilnHBAfAlYncwnkYSC2dZeIXh7DSXU6xLbcosh+63bopYi35+I1QbKRJ6TdE\n"
"rhe2LK6l2QKBgQDjafn9AVFa1L7sNNYuYjDbjj3WVwpCVaGkznq6pH2fLYjSpK4X\n"
"Zw/rkZVn8Xj+TRwAEyCEmrQYl9qSOWV1tsWS87a+U7CQWZC/WfrvRrBrLGkqbbIU\n"
"iDFAuZu+CeqcniDwtterrxmmYuLxZCU9uMZLXFw7cxTQjv3bQ2Jo7DfN4QKBgQDA\n"
"JcIKi4xDhVcz5NvDXhVXBldJzC/nzc/M2rNZHAIJKr/+SxwmTbfW5+ugVN3qxMZu\n"
"fgOcd/erQhx597labUM8LkeWA9WTdpOwFsflNsTwNpZ1ckBGnKcByJz2/80QLSS2\n"
"jG1dCpw8hC+Z+tak7gKN3UqXcp+UPwa8DKAu3sc3eQKBgHuGqDw+Hsf7W73qU1Dl\n"
"2w+Tds4SDdOHrQjcNLcZ2dPKBEujunUbRwxvaSgN8hb4FocbddRhh5ckrmv6zsln\n"
"q9WCtr1PLwPg+UKYekPQCwLUIUGiiJ7oyy0h0t3j8c4pS9+MF/KiRs6Tm8DFDOta\n"
"QDgWG4epo+N+nZL314O0r2UX\n"
"-----END PRIVATE KEY-----\n";



static const char* TEST_PUBLIC_KEY_PEM =
"-----BEGIN PUBLIC KEY-----\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA7URODr3ZcXvUcNKq3QeF\n"
"Mv3Skyk8DrIY5bAHsjI6FxLiyPE6X/whZK4wVs8TtyJCoWgIVh7bWYkNY5C3pAqi\n"
"rac3lOb6Yqey2KHCBMIa6GQs4oiH/KTtPEh58gnCfP0KQ6Qj47qprH7W62ptRiCK\n"
"Vs7e/u5jXQU4VOxxx+wjDc26rjOoXi0t7dB1IYd3XIRxcUzeZBPxWg/f95v2QR8Y\n"
"A7y85HBjcJTAR3en00g2XU3OjOKosz5GFU+d1rUBHTorT6regIw5LYkOP3jN+H9g\n"
"qeRkS5uAmCd65CaK+TlbMny+3GZiNYj23kxE9cba6zJjyRNtTyGPY6MUy6FByiqc\n"
"KQIDAQAB\n"
"-----END PUBLIC KEY-----\n";


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
        // 3. Se encripta el hash con llave pública.
        // 4. El resultado de encriptación se pasa a base64.


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
        size_t len = 0;

        status =  rsa_encrypt(hwid, strlen(hwid), encrypted, &len, path_public_key);
        if (status != 0)
        {
            status = 3;
            break;
        }

        encrypted_total = (int)len;

        // Paso 4
        status = Base64Encode(encrypted, encrypted_total, clientInfo, infoLen);
        if (status != 0)
        {
            status = 4;
            break;
        }


        //
        //PutInLog(NULL, LOG_LEVEL_NOTICE, "ENCRYPTAD0_B64: %d - Status : %d", *infoLen, status);
        ////DisplayHex(0, encriptado_b64, 16);

        ////*infoLen = 10;

        //return 0;

        //
        //unsigned char decoded_b64[10 * 104];
        //int decoded_b64_len = sizeof(encriptado_b64);

        //Base64Decode(encriptado_b64, decoded_b64, &decoded_b64_len);

       
        //char decrypted[1024 * 4];
        //int decrypted_total = 0;

        //status = rsa_decrypt(decoded_b64, decoded_b64_len,
        //    decrypted, &decrypted_total,
        //    TEST_PRIVATE_KEY_PEM);

        //PutInLog(NULL, LOG_LEVEL_NOTICE, "DECRYPTED: %d - Status : %d", decrypted_total, status);
        //DisplayHex(0, decrypted, decrypted_total);

        status = 0;
    
        break;
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


