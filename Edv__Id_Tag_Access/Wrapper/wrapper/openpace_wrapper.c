#include <eac/eac.h>
#include <eac/pace.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/objects.h>


#include <stdio.h>
#include <openssl/buffer.h>


#include <utils.h>

BUF_MEM* BUF_MEM_create_init(const unsigned char* data, size_t len);

int IO_Reader(
    unsigned char* tx,
    int txLen,
    unsigned char* rx,
    int* rxLen);



/////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////
__declspec(dllexport)
int OpenPACE_Init()
{
    EAC_init();
	
	return 0;
}

__declspec(dllexport)
void OpenPACE_Cleanup()
{
    EAC_cleanup();
}

/////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////
__declspec(dllexport)
void* EAC_Create()
{
    
	#if 1
	
	void *ptr = EAC_CTX_new();

	//printf("EAC_Create Void :  %u\r\n", (int)ptr);
	
	return ptr;
	
	#else
	
	return EAC_CTX_new();
	
	#endif
}


__declspec(dllexport)
void EAC_Free(void* ctx)
{
    if (ctx)
        EAC_CTX_clear_free((EAC_CTX*)ctx);
}

__declspec(dllexport)
int EAC_InitCardAccess(
    void* ctx,
    unsigned char* data,
    size_t dataLen)
{
	if (!ctx || !data)
        return 0;


	//printf("EAC_Init Card Access :  %u\r\n", (int)ctx);

    return EAC_CTX_init_ef_cardaccess(
        data,
        dataLen,
        (EAC_CTX*)ctx);
}

/////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////
__declspec(dllexport)
void* PACE_CreateSecret(const unsigned char* secret, int len)
{

	printf("MRZ IN\r\n");
	for(int i = 0; i < len; i++) printf("%02X ",secret[i]);
	printf("\r\n");


	void *ptr = PACE_SEC_new(secret, len, PACE_MRZ);

	//printf("Secure Ptr Val : %u\r\n", (int)ptr);
	
    return ptr;//PACE_SEC_new(secret, len, PACE_MRZ);
}

__declspec(dllexport)
void PACE_FreeSecret(void* sec)
{
    if (sec)
        PACE_SEC_clear_free((PACE_SEC*)sec);
}

// __declspec(dllexport)
// int EAC_SetSecret(
    // void* ctx,
    // void* sec)
// {
    // if (!ctx || !sec)
        // return 0;

    // return EAC_CTX_set_secret((EAC_CTX*)ctx, (PACE_SEC*)sec);
// }

/////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////
__declspec(dllexport)
int PACE_Step1_Alloc(
    void* ctx,
    void* sec,
    unsigned char** output,
    int* outputLen)
{
    if (!ctx || !sec || !output || !outputLen)
        return 0;

    BUF_MEM* buf = PACE_STEP1_enc_nonce(
        (EAC_CTX*)ctx,
        (PACE_SEC*)sec);

    if (!buf)
        return 0;

    *output = (unsigned char*)malloc(buf->length);
    if (!*output)
    {
        BUF_MEM_free(buf);
        return 0;
    }

    memcpy(*output, buf->data, buf->length);
    *outputLen = (int)buf->length;

    BUF_MEM_free(buf);

    return 1;
}


__declspec(dllexport)
void OpenPACE_Free(unsigned char* ptr)
{
    if (ptr)
        free(ptr);
}


__declspec(dllexport)
int PACE_Step2(
    void* ctx,
    void* sec,
    unsigned char* input,
    int inputLen)
{
	printf("RANA 0 -> %d\r\n", inputLen);
	for(int i = 0; i < inputLen; i++) printf("%02X ",input[i]);
	printf("\r\n");
	
	
	
	
    if (!ctx || !sec || !input)
        return 0;

    BUF_MEM* inBuf = NULL;

	// printf("RANA 1A: %u\r\n", (int)sec);
	// printf("RANA 1B: %u\r\n", (int)sec);

	// printf("RANA 1C: %u\r\n", (int)ctx);
	// printf("RANA 1D: %u\r\n", (int)ctx);



    // Crear BUF_MEM de entrada
    inBuf = BUF_MEM_create_init(input, inputLen);
    if (!inBuf)
        return 0;

	printf("RANA 2\r\n");



//int 	PACE_STEP2_dec_nonce (const EAC_CTX *ctx, const PACE_SEC *pi, const BUF_MEM *enc_nonce)
                 
    int result = PACE_STEP2_dec_nonce(
        (EAC_CTX*)ctx,
        (PACE_SEC*)sec,
        inBuf);

printf("RANA 3 -> %d \r\n", result);

    BUF_MEM_free(inBuf);
	
	printf("RANA 4\r\n");
	

    return result;
}

__declspec(dllexport)
int PACE_Step3A_Alloc(
    void* ctx,
    unsigned char** output,
    int* outputLen)
{
    if (!ctx || !output || !outputLen)
        return 0;

    BUF_MEM* buf = PACE_STEP3A_generate_mapping_data((EAC_CTX*)ctx);

    if (!buf || !buf->data || buf->length <= 0)
        return 0;

    *output = (unsigned char*)malloc(buf->length);
    if (!*output)
    {
        BUF_MEM_free(buf);
        return 0;
    }

    memcpy(*output, buf->data, buf->length);
    *outputLen = (int)buf->length;

    BUF_MEM_free(buf);

    return 1;
}


__declspec(dllexport)
int PACE_Step_3A_Map_Generator(
    void* ctx,
    unsigned char* input,
    int inputLen)
{
    if (!ctx || !input)
        return 0;

    BUF_MEM* inBuf = BUF_MEM_create_init(input, inputLen);
    if (!inBuf)
        return 0;

    int result = PACE_STEP3A_map_generator(
        (EAC_CTX*)ctx,
        inBuf);

    BUF_MEM_free(inBuf);
    return result;
}


__declspec(dllexport)
int PACE_STEP_3B_Generate_Ephemeral(
    void* ctx,
    unsigned char** output,
    int* outputLen)
{
    if (!ctx || !output || !outputLen)
        return 0;

    BUF_MEM* buf = PACE_STEP3B_generate_ephemeral_key((EAC_CTX*)ctx);

    if (!buf || !buf->data || buf->length <= 0)
        return 0;

    *output = (unsigned char*)malloc(buf->length);
    if (!*output)
    {
        BUF_MEM_free(buf);
        return 0;
    }

    memcpy(*output, buf->data, buf->length);
    *outputLen = (int)buf->length;

    BUF_MEM_free(buf);

    return 1;
}


__declspec(dllexport)
int PACE_Step_3B_Compute_Shared_Secret(
    void* ctx,
    unsigned char* input,
    int inputLen)
{
    if (!ctx || !input)
        return 0;

    BUF_MEM* inBuf = BUF_MEM_create_init(input, inputLen);
    if (!inBuf)
        return 0;

    int result = PACE_STEP3B_compute_shared_secret(
        (EAC_CTX*)ctx,
        inBuf);

    BUF_MEM_free(inBuf);
    return result;
}


__declspec(dllexport)
int PACE_Step_3C_Derive_Keys(void* ctx)
{
    if (!ctx)
        return 0;

    int result = PACE_STEP3C_derive_keys((EAC_CTX*)ctx);

    return result;
}

__declspec(dllexport)
int PACE_Step_3D_Compute_Authentication_Token(
    void* ctx,
    unsigned char* input,
    int inputLen,
    unsigned char** output,
    int* outputLen)
{
    if (!ctx || !input || !output || !outputLen)
        return 0;

    BUF_MEM* pub = BUF_MEM_create_init(input, inputLen);
    if (!pub)
        return 0;

    BUF_MEM* buf = PACE_STEP3D_compute_authentication_token(
        (EAC_CTX*)ctx,
        pub);

    BUF_MEM_free(pub);

    if (!buf || !buf->data || buf->length <= 0)
        return 0;

    *output = (unsigned char*)malloc(buf->length);
    if (!*output)
    {
        BUF_MEM_free(buf);
        return 0;
    }

    memcpy(*output, buf->data, buf->length);
    *outputLen = (int)buf->length;

    BUF_MEM_free(buf);

    return 1;
}




__declspec(dllexport)
int PACE_Step_3D_Verify(
    void* ctx,
    unsigned char* input,
    int inputLen)
{
    if (!ctx || !input)
        return 0;

    BUF_MEM* token = BUF_MEM_create_init(input, inputLen);
    if (!token)
        return 0;

    int result = PACE_STEP3D_verify_authentication_token(
        (EAC_CTX*)ctx,
        token);

    BUF_MEM_free(token);

    return result;
}


__declspec(dllexport)
int EAC_SetEncryptionCtx(
    void* ctx,
    int id)
{
    if (!ctx)
        return 0;

    return EAC_CTX_set_encryption_ctx(
        (EAC_CTX*)ctx,
        id);
}


/////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////
/*
#define MAX_KEY_LEN						384
#define MAX_CRYPT_DATA_LEN				4096

// NID_des_ecb
#define NID_des_ecb_KeySize				8
#define NID_des_ecb_BlockSize			8
// NID_des_cbc
#define NID_des_cbc_KeySize				8
#define NID_des_cbc_IVSize				8
#define NID_des_cbc_BlockSize			8
// NID_des_ede_cbc
#define NID_des_ede_cbc_KeySize			16
#define NID_des_ede_cbc_IVSize			8
#define NID_des_ede_cbc_BlockSize		8
// NID_des_ede3_cbc
#define NID_des_ede3_cbc_KeySize		24
#define NID_des_ede3_cbc_IVSize			8
#define NID_des_ede3_cbc_BlockSize		8
// NID_aes_128_cbc
#define NID_aes_128_cbc_KeySize			16
#define NID_aes_128_cbc_IVSize			16
#define NID_aes_128_cbc_BlockSize		16
// NID_aes_192_cbc
#define NID_aes_192_cbc_KeySize			24
#define NID_aes_192_cbc_IVSize			16
#define NID_aes_192_cbc_BlockSize		16
// NID_aes_256_cbc
#define NID_aes_256_cbc_KeySize			32
#define NID_aes_256_cbc_IVSize			16
#define NID_aes_256_cbc_BlockSize		16
// NID_aes_256_ecb
#define NID_aes_256_ecb_KeySize			32
#define NID_aes_256_ecb_BlockSize		16

// NID_md4
#define NID_md4_BlockSize				16
// NID_md5
#define NID_md5_BlockSize				16
// NID_sha
#define NID_sha_BlockSize				20
// NID_sha1
#define NID_sha1_BlockSize				20
// NID_sha224
#define NID_sha224_BlockSize			28
// NID_sha256
#define NID_sha256_BlockSize			32
// NID_sha384
#define NID_sha384_BlockSize			48
// NID_sha512
#define NID_sha512_BlockSize			64

enum
{
    CODER_CRYPT_DECRYPT,
    CODER_CRYPT_ENCRYPT,
    CODER_CRYPT_DECRYPT_PKCS5,
    CODER_CRYPT_ENCRYPT_PKCS5
};

enum
{
    // GETHASHFILE
    CODER_ERROR_NO_ERROR = 0,
    CODER_ERROR_DIGESTFILE_OPEN,
    CODER_ERROR_DIGESTFILE_NID,
    CODER_ERROR_DIGESTFILE_OVERFLOW,
    CODER_ERROR_DIGESTFILE_OVERFLOW_ASCII,
    CODER_ERROR_DIGESTFILE_INIT,
    CODER_ERROR_DIGESTFILE_READ,
    CODER_ERROR_DIGESTFILE_UPDATE,
    CODER_ERROR_DIGESTFILE_FINAL,

    // GETHASH
    CODER_ERROR_DIGEST_NID = 10,
    CODER_ERROR_DIGEST_OVERFLOW,
    CODER_ERROR_DIGEST_OVERFLOW_ASCII,
    CODER_ERROR_DIGEST_INIT,
    CODER_ERROR_DIGEST_READ,
    CODER_ERROR_DIGEST_UPDATE,
    CODER_ERROR_DIGEST_FINAL,

    // GETKEY
    CODER_ERROR_KEY_CRYPTNID = 20,
    CODER_ERROR_KEY_HASHTNID,
    CODER_ERROR_KEY_BYTESTOKEY,

    // GETCRYPT
    CODER_ERROR_CRYPT_NID = 30,
    CODER_ERROR_CRYPT_DECRYPTPAD,
    CODER_ERROR_CRYPT_ENCRYPTPAD,
    CODER_ERROR_CRYPT_DECRYPTPKCS5,
    CODER_ERROR_CRYPT_MODE,
    CODER_ERROR_CRYPT_OVERFLOW,
    CODER_ERROR_CRYPT_OVERFLOW_ASCII,
    CODER_ERROR_CRYPT_INIT,
    CODER_ERROR_CRYPT_UPDATE,
    CODER_ERROR_CRYPT_FINAL,

    // ...
    CODER_ERROR_STATUS_INI = 1000,
    CODER_ERROR_STATUS_END
};
*/

/*
__declspec(dllexport)
int getCrypt(int iCryptNID, int iMode, unsigned char *ucpKey, int iKeyLen, unsigned char *ucpIv, int iIvLen, unsigned char *ucpDataIn, int iDataInLen, unsigned char *ucpDataOut, int *ipDataOutLen, char iASCII)
{
    long					lError = CODER_ERROR_NO_ERROR;
    long					lReturn = 0;
    long					lBlockSize = 0;
    int						iDataOutLen = 0;
    int						iTmp1 = 0;
    long					lTmp1 = 0;
    long					lTmp2 = 0;
    int						iPadding = 0;
    EVP_CIPHER				*opCryptNID = NULL;
    EVP_CIPHER_CTX 			*opCryptContext;

    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();

    opCryptNID = (EVP_CIPHER *)EVP_get_cipherbyname(OBJ_nid2sn(iCryptNID));
    if(opCryptNID == NULL)
    {
        lError = CODER_ERROR_CRYPT_NID;
        goto JMP_getCrypt;
    }

    lBlockSize = EVP_CIPHER_block_size(opCryptNID);
    switch(iMode)
    {
        case CODER_CRYPT_DECRYPT:
            if (iDataInLen % lBlockSize != 0)
            {
                lError = CODER_ERROR_CRYPT_DECRYPTPAD;
                goto JMP_getCrypt;
            }
            iDataOutLen = iDataInLen;
            iMode = 0;
            iPadding = 0;
            break;
        case CODER_CRYPT_ENCRYPT:
            if (iDataInLen % lBlockSize != 0)
            {
                lError = CODER_ERROR_CRYPT_ENCRYPTPAD;
                goto JMP_getCrypt;
            }
            iDataOutLen = iDataInLen;
            if (iDataOutLen % lBlockSize != 0)
            {
                iDataOutLen = iDataInLen + (lBlockSize - iDataInLen % lBlockSize);
            }
            iMode = 1;
            iPadding = 0;
            break;
        case CODER_CRYPT_DECRYPT_PKCS5:
            if (iDataInLen % lBlockSize != 0)
            {
                lError = CODER_ERROR_CRYPT_DECRYPTPKCS5;
                goto JMP_getCrypt;
            }
            iDataOutLen = iDataInLen;
            iMode = 0;
            iPadding = 1;
            break;
        case CODER_CRYPT_ENCRYPT_PKCS5:
            iDataOutLen = iDataInLen + (lBlockSize - iDataInLen % lBlockSize);
            iMode = 1;
            iPadding = 1;
            break;
        default:
            lError = CODER_ERROR_CRYPT_MODE;
            goto JMP_getCrypt;
    }

    if(ucpDataOut == NULL)
    {
        *ipDataOutLen = iDataOutLen;
        goto JMP_getCrypt;
    }

    if(*ipDataOutLen < iDataOutLen && iASCII == 0)
    {
        *ipDataOutLen = iDataOutLen;
        lError = CODER_ERROR_CRYPT_OVERFLOW;
        goto JMP_getCrypt;
    }

    if(*ipDataOutLen < (iDataOutLen + iDataOutLen) && iASCII != 0)
    {
        *ipDataOutLen = iDataOutLen + iDataOutLen;
        lError = CODER_ERROR_CRYPT_OVERFLOW_ASCII;
        goto JMP_getCrypt;
    }

    opCryptContext = EVP_CIPHER_CTX_new();

    lReturn = EVP_CipherInit(opCryptContext, opCryptNID, ucpKey, ucpIv, iMode);
    if (lReturn == 0)
    {
        lError = CODER_ERROR_CRYPT_INIT;
        goto JMP_getCrypt;
    }

    EVP_CIPHER_CTX_set_padding(opCryptContext, iPadding);

    lReturn = EVP_CipherUpdate(opCryptContext, ucpDataOut, ipDataOutLen, ucpDataIn, iDataInLen);
    if (lReturn == 0)
    {
        lError = CODER_ERROR_CRYPT_UPDATE;
        goto JMP_getCrypt;
    }

    lReturn = EVP_CipherFinal(opCryptContext, ucpDataOut + *ipDataOutLen, &iTmp1);
    if (lReturn == 0)
    {
        lError = CODER_ERROR_CRYPT_FINAL;
        goto JMP_getCrypt;
    }
    *ipDataOutLen += iTmp1;

    if (iASCII != 0)
    {
        for(lTmp1 = *ipDataOutLen; lTmp1 > 0; lTmp1--)
        {
            lTmp2 = ucpDataOut[lTmp1 - 1] & 15;
            ucpDataOut[lTmp1 + lTmp1 - 1] = (unsigned char)(lTmp2 > 9 ? 'A' + lTmp2 - 10 : '0' + lTmp2);
            lTmp2 = (ucpDataOut[lTmp1 - 1] >> 4) & 15;
            ucpDataOut[lTmp1 + lTmp1 - 2] = (unsigned char)(lTmp2 > 9 ? 'A' + lTmp2 - 10 : '0' + lTmp2);
        }
    }

    JMP_getCrypt:
    EVP_CIPHER_CTX_free(opCryptContext);
    EVP_cleanup();
    return lError;
}
*/



__declspec(dllexport)
int OpenPACE_AES_Encrypt(unsigned char* key, unsigned char* input, unsigned char* output)
{
    AES_KEY aesKey;
    AES_set_encrypt_key(key, 128, &aesKey);
    AES_encrypt(input, output, &aesKey);
    return 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Callback IO
//////////////////////////////////////////////////////////////////////////////////////////////

typedef int (*TxRx_Callback)(
    unsigned char* tx,
    int txLen,
    unsigned char* rx,
    int* rxLen
);

static TxRx_Callback g_TxRx = NULL;


//
// Insert Callback
//

__declspec(dllexport)
void Register_TxRx_Callback(TxRx_Callback cb)
{
	g_TxRx = cb;
}

//
// Test
//

//__declspec(dllexport)
int IO_Reader(
    unsigned char* tx,
    int txLen,
    unsigned char* rx,
    int* rxLen)
{
	
	printf("Buffer to TX1 : ");
	for(int i = 0; i < txLen; i++) printf("0x%02X ", tx[i]);
	printf("\r\n");
	
    if (!g_TxRx)
        return -1; // no hay callback

	printf("Buffer to TX2 : ");
	for(int i = 0; i < txLen; i++) printf("0x%02X ", tx[i]);
	printf("\r\n");



    return g_TxRx(tx, txLen, rx, rxLen);
}


__declspec(dllexport)
int Test_Reader(void)
{
	unsigned char TxBuffer[] =  {
		0x00, 0xA4, 0x04, 0x0C, 0x10, 0xA0, 0x00, 0x00,
		0x00, 0x77, 0x03, 0x0C, 0x60, 0x00, 0x00, 0x00,
		0xFE, 0x00, 0x00, 0x05, 0x00, 0x00
	};

	DisplayHex(0, TxBuffer, sizeof(TxBuffer));

	return 0;

	/*
	printf("POR LA XUXA\r\n");


	unsigned char RxBuffer[256];
	int total_received = 0;

	return IO_Reader(TxBuffer, sizeof(TxBuffer), RxBuffer, &total_received);*/
}