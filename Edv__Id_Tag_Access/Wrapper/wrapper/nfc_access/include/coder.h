#ifndef __CODER_H__
#define __CODER_H__

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/evp.h>

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
extern int				getKey(int iCipherNID, int iHashNID, int iRounds, unsigned char *ucpSeed, int iSeedLen, unsigned char *ucpKey, int *ipKeyLen, unsigned char *ucpIv, int *ipIvLen);
extern int				getCrypt(int iCryptNID, int iMode, unsigned char *ucpKey, int iKeyLen, unsigned char *ucpIv, int iIvLen, unsigned char *ucpDataIn, int iDataInLen, unsigned char *ucpDataOut, int *ipDataOutLen, char iASCII);
extern int				getHash(int iHashNID, unsigned char *ucpDataIn, int iDataInLen, unsigned char *ucpDataOut, int *ipDataOutLen, char iASCII);
extern int				getHashFile(int iHashNID, char *sPathName, unsigned char *ucpDataOut, int *ipDataOutLen, char iASCII);
extern void				getScr(unsigned char *dataInOut, int dataInOutLen);
extern void				getDcr(unsigned char *dataInOut, int dataInOutLen);
extern int 					Base32Decode(char* cpCode, unsigned char* ucpData, unsigned long* ulpSizeData);
extern int 					Base32Encode(unsigned char* ucpData, unsigned long ulSizeData, char* cpCode, unsigned long* ulpSizeCode);
extern int 					Base64Decode(char* cpCode, unsigned char* ucpData, unsigned long* ulpSizeData);
extern int 					Base64Encode(unsigned char* ucpData, unsigned long ulSizeData, char* cpCode, unsigned long* ulpSizeCode);

#endif // __CODER_H__
