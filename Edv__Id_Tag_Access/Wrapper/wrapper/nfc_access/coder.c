#ifndef _WIN32
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#endif
#include "coder.h"
#include <openssl/pem.h>
#include <openssl/x509.h>

#include "generictypedefs.h"

// Add new functions to process Hash and Crypt Buffers
// TODO

const char 			mc_sBASE_32_CHARACTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
const char 			mc_sBASE_64_CHARACTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";



/////////////////////////////////////////////////////////////////////////
// int getKey(int iCryptNID, int iHashNID, int iRounds, unsigned char *ucpSeed, int iSeedLen, unsigned char *ucpKey, int *ipKeyLen, unsigned char *ucpIv, int *ipIvLen)
/////////////////////////////////////////////////////////////////////////
int getKey(int iCryptNID, int iHashNID, int iRounds, unsigned char *ucpSeed, int iSeedLen, unsigned char *ucpKey, int *ipKeyLen, unsigned char *ucpIv, int *ipIvLen)
{
    long					lError = CODER_ERROR_NO_ERROR;
    long					lReturn = 0;
    long					lCryptKeySize = 0;
    long					lCryptIVSize = 0;
    EVP_CIPHER				*opCryptNID = NULL;
    EVP_MD					*opHashNID = NULL;

    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();

    opCryptNID = (EVP_CIPHER *)EVP_get_cipherbyname(OBJ_nid2sn(iCryptNID));
    if(opCryptNID == NULL)
    {
        lError = CODER_ERROR_KEY_CRYPTNID;
        goto JMP_getKey;
    }

    lCryptKeySize = EVP_CIPHER_key_length(opCryptNID);
    lCryptIVSize = EVP_CIPHER_iv_length(opCryptNID);

    if(ucpKey == NULL || ucpIv == NULL)
    {
        *ipKeyLen = lCryptKeySize;
        *ipIvLen = lCryptIVSize;
        goto JMP_getKey;
    }

    opHashNID = (EVP_MD *)EVP_get_digestbyname(OBJ_nid2sn(iHashNID));
    if(opHashNID == NULL)
    {
        lError = CODER_ERROR_KEY_HASHTNID;
        goto JMP_getKey;
    }

    lReturn = EVP_BytesToKey(opCryptNID, opHashNID, NULL, ucpSeed, iSeedLen, iRounds, ucpKey, ucpIv);
    if(lReturn == 0)
    {
        lError = CODER_ERROR_KEY_BYTESTOKEY;
        goto JMP_getKey;
    }

    *ipKeyLen = lCryptKeySize;
    *ipIvLen = lCryptIVSize;

    JMP_getKey:
    EVP_cleanup();
    return lError;
}



/////////////////////////////////////////////////////////////////////////
// int getCrypt(int iCryptNID, int iMode, unsigned char *ucpKey, int iKeyLen, unsigned char *ucpIv, int iIvLen, unsigned char *ucpDataIn, int iDataInLen, unsigned char *ucpDataOut, int *ipDataOutLen, char iASCII)
/////////////////////////////////////////////////////////////////////////
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


/////////////////////////////////////////////////////////////////////////
// int getHash(int iHashNID, unsigned char *ucpDataIn, int iDataInLen, unsigned char *ucpDataOut, int *ipDataOutLen, char iASCII)
/////////////////////////////////////////////////////////////////////////

int getHash(int iHashNID, unsigned char *ucpDataIn, int iDataInLen, unsigned char *ucpDataOut, int *ipDataOutLen, char iASCII)
{
    long					lError = CODER_ERROR_NO_ERROR;
    long					lReturn = 0;
    long					lDigestSize = 0;
    long					lTmp1 = 0;
    long					lTmp2 = 0;
    EVP_MD					*opHashNID = NULL;
    EVP_MD_CTX				*opHashContext;

    OpenSSL_add_all_digests();

    opHashNID = (EVP_MD *)EVP_get_digestbyname(OBJ_nid2sn(iHashNID));
    if(opHashNID == NULL)
    {
        lError = CODER_ERROR_DIGEST_NID;
        goto JMP_getDigest;
    }

    lDigestSize = EVP_MD_size(opHashNID);
    if(ucpDataOut == NULL)
    {
        *ipDataOutLen = lDigestSize;
        goto JMP_getDigest;
    }

    if(*ipDataOutLen < lDigestSize && iASCII == 0)
    {
        *ipDataOutLen = lDigestSize;
        lError = CODER_ERROR_DIGEST_OVERFLOW;
        goto JMP_getDigest;
    }

    if(*ipDataOutLen < (lDigestSize + lDigestSize) && iASCII != 0)
    {
        *ipDataOutLen = lDigestSize + lDigestSize;
        lError = CODER_ERROR_DIGEST_OVERFLOW_ASCII;
        goto JMP_getDigest;
    }

        //memset(&oHashContext, 0, sizeof(oHashContext));
#ifdef _WIN32
        opHashContext = EVP_MD_CTX_new();
#elif ANDROID_JNI
        opHashContext = EVP_MD_CTX_create();
#else
#error "Add platform's version specific function new or create evp message digest context."
#endif
    EVP_MD_CTX_init(opHashContext);

    lReturn = EVP_DigestInit_ex(opHashContext, opHashNID, NULL);
    if (lReturn == 0)
    {
        lError = CODER_ERROR_DIGEST_INIT;
        goto JMP_getDigest;
    }

    lReturn = EVP_DigestUpdate(opHashContext, ucpDataIn, iDataInLen);
    if (lReturn == 0)
    {
        lError = CODER_ERROR_DIGEST_UPDATE;
        goto JMP_getDigest;
    }

    lReturn = EVP_DigestFinal_ex(opHashContext, ucpDataOut, (unsigned int *)ipDataOutLen);
    if (lReturn == 0)
    {
        lError = CODER_ERROR_DIGEST_FINAL;
        goto JMP_getDigest;
    }

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

    JMP_getDigest:
#ifdef _WIN32
    EVP_MD_CTX_free(opHashContext);
#elif ANDROID_JNI
    EVP_MD_CTX_free(opHashContext); //EVP_MD_CTX_cleanup(opHashContext);
#else
#error "Add platform's version specific function free or cleanup evp message digest context."
#endif
    EVP_cleanup();
    return lError;
}

/*

/////////////////////////////////////////////////////////////////////////
// int getHashFile(int iHashNID, char *sPathName, unsigned char *ucpDataOut, int *ipDataOutLen, char iASCII)
/////////////////////////////////////////////////////////////////////////
int getHashFile(int iHashNID, char *sPathName, unsigned char *ucpDataOut, int *ipDataOutLen, char iASCII)
{
    char				    sTmp1[MAX_UTIL_MSG_LEN + 1];
    FILE					*hFile = NULL;
    long					lError = CODER_ERROR_NO_ERROR;
    long					lReturn = 0;
    long					lDigestSize = 0;
    long					lTmp1 = 0;
    long					lTmp2 = 0;
    EVP_MD					*opHashNID = NULL;
    EVP_MD_CTX				*opHashContext;

    fopen_sec(hFile, sPathName, (char *)"rb");
    if (hFile == NULL)
    {
        lError = CODER_ERROR_DIGESTFILE_OPEN;
        goto JMP_getDigestFile;
    }

    OpenSSL_add_all_digests();

    opHashNID = (EVP_MD *)EVP_get_digestbyname(OBJ_nid2sn(iHashNID));
    if(opHashNID == NULL)
    {
        lError = CODER_ERROR_DIGESTFILE_NID;
        goto JMP_getDigestFile;
    }

    lDigestSize = EVP_MD_size(opHashNID);
    if(ucpDataOut == NULL)
    {
        *ipDataOutLen = lDigestSize;
        goto JMP_getDigestFile;
    }

    if(*ipDataOutLen < lDigestSize && iASCII == 0)
    {
        *ipDataOutLen = lDigestSize;
        lError = CODER_ERROR_DIGESTFILE_OVERFLOW;
        goto JMP_getDigestFile;
    }

    if(*ipDataOutLen < (lDigestSize + lDigestSize) && iASCII != 0)
    {
        *ipDataOutLen = lDigestSize + lDigestSize;
        lError = CODER_ERROR_DIGESTFILE_OVERFLOW_ASCII;
        goto JMP_getDigestFile;
    }

        //memset(&oHashContext, 0, sizeof(oHashContext));
#ifdef _WIN32
        opHashContext = EVP_MD_CTX_new();
#elif ANDROID_JNI
        opHashContext = EVP_MD_CTX_create();
#else
    #error "Add platform's version specific function new or create evp message digest context."
#endif
    EVP_MD_CTX_init(opHashContext);

    lReturn = EVP_DigestInit_ex(opHashContext, opHashNID, NULL);
    if (lReturn == 0)
    {
        lError = CODER_ERROR_DIGESTFILE_INIT;
        goto JMP_getDigestFile;
    }

    do
    {
        lTmp1 = fread(sTmp1, sizeof(char), sizeof(sTmp1) - 1, hFile);
        if (lTmp1 < 0)
        {
            lError = CODER_ERROR_DIGESTFILE_READ;
            goto JMP_getDigestFile;
        }

        lReturn = EVP_DigestUpdate(opHashContext, sTmp1, lTmp1);
        if (lReturn == 0)
        {
            lError = CODER_ERROR_DIGESTFILE_UPDATE;
            goto JMP_getDigestFile;
        }
    } while(feof(hFile) == 0);

    lReturn = EVP_DigestFinal_ex(opHashContext, ucpDataOut, (unsigned int *)ipDataOutLen);
    if (lReturn == 0)
    {
        lError = CODER_ERROR_DIGESTFILE_FINAL;
        goto JMP_getDigestFile;
    }

    if (iASCII != 0)
    {
        for(lTmp1 = lDigestSize; lTmp1 > 0; lTmp1--)
        {
            lTmp2 = ucpDataOut[lTmp1 - 1] & 15;
            ucpDataOut[lTmp1 + lTmp1 - 1] = (unsigned char)(lTmp2 > 9 ? 'A' + lTmp2 - 10 : '0' + lTmp2);
            lTmp2 = (ucpDataOut[lTmp1 - 1] >> 4) & 15;
            ucpDataOut[lTmp1 + lTmp1 - 2] = (unsigned char)(lTmp2 > 9 ? 'A' + lTmp2 - 10 : '0' + lTmp2);
        }
    }

    JMP_getDigestFile:
    if (hFile != NULL)
    {
        fclose(hFile);
    }
#ifdef _WIN32
        EVP_MD_CTX_free(opHashContext);
#elif ANDROID_JNI
    EVP_MD_CTX_free(opHashContext); //EVP_MD_CTX_cleanup(opHashContext);
#else
    #error "Add platform's version specific function free or cleanup evp message digest context."
#endif
    EVP_cleanup();
    return lError;
}

/////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////
void getScr(unsigned char *dataInOut, int dataInOutLen)
{
    unsigned char 	h[8] = {0x00, 0x97, 0x50, 0xde, 0x66, 0x0f, 0x68, 0x95};
    unsigned char 	y[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    unsigned char 	cTmp;
    int 			iTmp = 0;

    for(iTmp = 0; iTmp < dataInOutLen; iTmp++)
    {
        y[7] = y[6];
        y[6] = y[5];
        y[5] = y[4];
        y[4] = y[3];
        y[3] = y[2];
        y[2] = y[1];
        y[1] = y[0];

        cTmp = (h[6]^y[6])+(h[7]^y[7]);
        cTmp = (h[5]^y[5])+cTmp;
        cTmp = (h[4]^y[4])+cTmp;
        cTmp = (h[3]^y[3])+cTmp;
        cTmp = (h[2]^y[2])+cTmp;
        cTmp = (h[1]^y[1])+cTmp;
        cTmp = (dataInOut[iTmp])+cTmp;

        y[0] = cTmp;
        dataInOut[iTmp] = y[0];
    }
}

/////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////
void getDcr(unsigned char *dataInOut, int dataInOutLen)
{
    unsigned char 	h[8] = {0x00, 0x97, 0x50, 0xde, 0x66, 0x0f, 0x68, 0x95};
    unsigned char 	y[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char 	cTmp;
    int 			iTmp = 0;

    for(iTmp = 0; iTmp < dataInOutLen; iTmp++)
    {
        y[7] = y[6];
        y[6] = y[5];
        y[5] = y[4];
        y[4] = y[3];
        y[3] = y[2];
        y[2] = y[1];
        y[1] = y[0];

        cTmp = dataInOut[iTmp];
        cTmp = cTmp - (h[1]^y[1]);
        cTmp = cTmp - (h[2]^y[2]);
        cTmp = cTmp - (h[3]^y[3]);
        cTmp = cTmp - (h[4]^y[4]);
        cTmp = cTmp - (h[5]^y[5]);
        cTmp = cTmp - (h[6]^y[6]);
        cTmp = cTmp - (h[7]^y[7]);

        y[0] = dataInOut[iTmp];

        dataInOut[iTmp] = cTmp;
    }
}

/////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////
int Base32Decode(char* cpCode, unsigned char* ucpData, unsigned long* ulpSizeData)
{
    unsigned long		lPosition = 0;
    int					iData = 0;
    int					iDataLeft = 0;
    char* pChar = NULL;
    char				cChar = 0;

    if (cpCode == NULL || ucpData == NULL || ulpSizeData == NULL)
    {
        return 1;
    }
    if (*ulpSizeData <= 0)
    {
        return 2;
    }

    memset(ucpData, 0, *ulpSizeData);
    for (pChar = cpCode; lPosition < *ulpSizeData && *pChar != '\000'; pChar++)
    {
        cChar = *pChar;
        if (cChar == ' ' || cChar == '\t' || cChar == '\r' || cChar == '\n' || cChar == '-')
        {
            continue;
        }
        iData <<= 5;

        // Deal with commonly mistyped characters
        if (cChar == '0')
        {
            cChar = 'O';
        }
        else
        {
            if (cChar == '1')
            {
                cChar = 'L';
            }
            else
            {
                if (cChar == '8')
                {
                    cChar = 'B';
                }
            }
        }

        // Look up one base32 digit
        if ((cChar >= 'A' && cChar <= 'Z') || (cChar >= 'a' && cChar <= 'z'))
        {
            cChar = (cChar & 0x1F) - 1;
        }
        else
        {
            if (cChar >= '2' && cChar <= '7')
            {
                cChar -= '2' - 26;
            }
            else
            {
                return 3;
            }
        }

        iData |= cChar;
        iDataLeft += 5;
        if (iDataLeft >= 8)
        {
            ucpData[lPosition++] = iData >> (iDataLeft - 8);
            iDataLeft -= 8;
        }
    }
    if (lPosition < *ulpSizeData)
    {
        ucpData[lPosition] = '\000';
    }
    *ulpSizeData = lPosition;

    return 0;
}

/////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////
int Base32Encode(unsigned char* ucpData, unsigned long ulSizeData, char* cpCode, unsigned long* ulpSizeCode)
{
    unsigned long	ulPosition = 0;
    int 			iData = 0;
    int 			iDataLeft = 8;
    unsigned long	ulNext = 1;
    int				iPad = 0;

    if (ucpData == NULL || ulSizeData <= 0 || cpCode == NULL || ulpSizeCode == NULL)
    {
        return 1;
    }
    if (*ulpSizeCode <= 0)
    {
        return 2;
    }

    memset(cpCode, 0, *ulpSizeCode);
    for (iData = ucpData[0]; ulPosition < *ulpSizeCode && (iDataLeft > 0 || ulNext < ulSizeData); iDataLeft -= 5)
    {
        if (iDataLeft < 5)
        {
            if (ulNext < ulSizeData)
            {
                iData <<= 8;
                iData |= ucpData[ulNext++] & 0xFF;
                iDataLeft += 8;
            }
            else
            {
                iPad = 5 - iDataLeft;
                iData <<= iPad;
                iDataLeft += iPad;
            }
        }
        cpCode[ulPosition++] = mc_sBASE_32_CHARACTERS[0x1F & (iData >> (iDataLeft - 5))];
    }
    if (ulPosition < *ulpSizeCode)
    {
        cpCode[ulPosition] = '\000';
    }
    *ulpSizeCode = ulPosition;

    return 0;
}

*/


/////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////
int Base64Decode(char* cpCode, unsigned char* ucpData, unsigned long* ulpSizeData)
{
    unsigned long		lPosition;
    unsigned long		lLenCode;
    unsigned long		lBits;
    char* pChar;

    if (cpCode == NULL || ucpData == NULL || ulpSizeData == NULL)
    {
        return 1;
    }
    if (*ulpSizeData <= 0)
    {
        return 2;
    }

    lLenCode = strlen(cpCode);

	PutInLog(NULL, 0, "Base64Decode : lLenCode : %lu\r\n", lLenCode);

    memset(ucpData, 0, *ulpSizeData);
    for (lPosition = 0, *ulpSizeData = 0; lPosition < lLenCode; lPosition += 4)
    {
        lBits = 0;
        if ((pChar = (char*)strchr(mc_sBASE_64_CHARACTERS, cpCode[lPosition])) != NULL)
        {
            lBits |= (((pChar - mc_sBASE_64_CHARACTERS) & 0xff) << 18);

            if ((pChar = (char*)strchr(mc_sBASE_64_CHARACTERS, cpCode[lPosition + 1])) != NULL)
            {
                lBits |= (((pChar - mc_sBASE_64_CHARACTERS) & 0xff) << 12);

                if ((pChar = (char*)strchr(mc_sBASE_64_CHARACTERS, cpCode[lPosition + 2])) != NULL)
                {
                    lBits |= (((pChar - mc_sBASE_64_CHARACTERS) & 0xff) << 6);

                    if ((pChar = (char*)strchr(mc_sBASE_64_CHARACTERS, cpCode[lPosition + 3])) != NULL)
                    {
                        lBits |= ((pChar - mc_sBASE_64_CHARACTERS) & 0xff);
                    }
                }
            }
        }
        ucpData[(*ulpSizeData)++] = (char)((lBits & 0xff0000) >> 16);
        ucpData[(*ulpSizeData)++] = (char)((lBits & 0xff00) >> 8);
        ucpData[(*ulpSizeData)++] = (char)(lBits & 0xff);
    }
    if (cpCode[lPosition - 2] == 0x3d || lLenCode % 4 == 2)
    {
        (*ulpSizeData) -= 2;
    }
    else
    {
        if (cpCode[lPosition - 1] == 0x3d || lLenCode % 4 == 3)
        {
            (*ulpSizeData)--;
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////
int Base64Encode(unsigned char* ucpData, unsigned long ulSizeData, char* cpCode, unsigned long* ulpSizeCode)
{
    unsigned long	lPosition = 0;
    unsigned long	lBits;

    if (ucpData == NULL || ulSizeData <= 0 || cpCode == NULL || ulpSizeCode == NULL)
    {
        return 1;
    }
    if (*ulpSizeCode <= 0)
    {
        return 2;
    }

    memset(cpCode, 0, *ulpSizeCode);
    *ulpSizeCode = 0;
    while (ulSizeData >= (lPosition + 3))
    {
        lBits = 0;
        if (lPosition < ulSizeData)
        {
            lBits |= ((ucpData[lPosition++] & 0xff) << 16);
        }
        if (lPosition < ulSizeData)
        {
            lBits |= ((ucpData[lPosition++] & 0xff) << 8);
        }
        if (lPosition < ulSizeData)
        {
            lBits |= (ucpData[lPosition++] & 0xff);
        }
        cpCode[(*ulpSizeCode)++] = mc_sBASE_64_CHARACTERS[(lBits & 0x00fc0000) >> 18];
        cpCode[(*ulpSizeCode)++] = mc_sBASE_64_CHARACTERS[(lBits & 0x0003f000) >> 12];
        cpCode[(*ulpSizeCode)++] = mc_sBASE_64_CHARACTERS[(lBits & 0x00000fc0) >> 6];
        cpCode[(*ulpSizeCode)++] = mc_sBASE_64_CHARACTERS[lBits & 0x0000003f];
    }
    if (ulSizeData - lPosition > 0 && ulSizeData - lPosition < 3)
    {
        if (ulSizeData - lPosition - 1 != 0)
        {
            lBits = ((ucpData[lPosition++] & 0xff) << 16);
            lBits |= ((ucpData[lPosition++] & 0xff) << 8);
            cpCode[(*ulpSizeCode)++] = mc_sBASE_64_CHARACTERS[(lBits & 0x00fc0000) >> 18];
            cpCode[(*ulpSizeCode)++] = mc_sBASE_64_CHARACTERS[(lBits & 0x0003f000) >> 12];
            cpCode[(*ulpSizeCode)++] = mc_sBASE_64_CHARACTERS[(lBits & 0x00000fc0) >> 6];
            cpCode[(*ulpSizeCode)++] = '=';
        }
        else
        {
            lBits = ((ucpData[lPosition++] & 0xff) << 16);
            cpCode[(*ulpSizeCode)++] = mc_sBASE_64_CHARACTERS[(lBits & 0x00fc0000) >> 18];
            cpCode[(*ulpSizeCode)++] = mc_sBASE_64_CHARACTERS[(lBits & 0x0003f000) >> 12];
            cpCode[(*ulpSizeCode)++] = '=';
            cpCode[(*ulpSizeCode)++] = '=';
        }
    }

    return 0;
}