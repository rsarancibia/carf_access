
#include <generictypedefs.h>
#include <CypherDechypher.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <utils.h>

/// <summary>
// openssl genpkey -algorithm RSA -out private.pem -pkeyopt rsa_keygen_bits:2048 && openssl pkey -in private.pem -pubout -out public.pem 
/// </summary>

int rsa_encrypt(const unsigned char* plaintext, size_t plaintext_len,
    unsigned char* ciphertext, size_t* ciphertext_len,
    const char* pubkey_path)
{
    FILE* fp = fopen(pubkey_path, "r");
    if (!fp) return 1;

    EVP_PKEY* pubkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);
    if (!pubkey) return 2;

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pubkey, NULL);
    if (!ctx) return 3;

    if (EVP_PKEY_encrypt_init(ctx) <= 0) return 4;

    // Usar OAEP (recomendado)
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    // Primero obtener tamaño
    if (EVP_PKEY_encrypt(ctx, NULL, ciphertext_len, plaintext, plaintext_len) <= 0)
        return 5;

    // Luego encriptar
    if (EVP_PKEY_encrypt(ctx, ciphertext, ciphertext_len, plaintext, plaintext_len) <= 0)
        return 6;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pubkey);

    return 0;
}

static unsigned char    *g_baUserLicenseToCheck = NULL;
static long             g_UserLicenseToCheck_Len = 0;

unsigned char* read_file(const char* path, long* size)
{
    unsigned char* buffer = NULL;
    
    FILE* f = fopen(path, "rb");
    
    if(f != NULL)
    {
        fseek(f, 0, SEEK_END);
        *size = ftell(f);
        fseek(f, 0, SEEK_SET);

        buffer = malloc(*size);
        fread(buffer, 1, *size, f);
        fclose(f);
    }
    return buffer;
}

void Check_Buffer_License()
{
    if (g_baUserLicenseToCheck != NULL)
    {
        free(g_baUserLicenseToCheck);
        g_baUserLicenseToCheck = NULL;
    }
}


int verify_signature(
    unsigned char* data, size_t data_len,
    unsigned char* sig, size_t sig_len,
    const char* pubkey_path)
{
    int status = 0;
    EVP_PKEY* pubkey = NULL;
    EVP_MD_CTX* ctx = NULL;

    while (TRUE)
    {
        FILE* f = fopen(pubkey_path, "r");
        if (f == NULL)
        {
            status = 1;
            break;
        }

        pubkey = PEM_read_PUBKEY(f, NULL, NULL, NULL);
        
        fclose(f);

        if(pubkey == NULL)
        {
            status = 2;
            break;
        }

        ctx = EVP_MD_CTX_new();
        if (!ctx) 
        {
            status = 3;
            break;
        }

        if (EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pubkey) != 1)
        {
            status = 4;
            break;
        }

        if (EVP_DigestVerifyUpdate(ctx, data, data_len) != 1)
        {
            status = 5;
            break;
        }

        if(EVP_DigestVerifyFinal(ctx, sig, sig_len) != 1)
        {
            status = 6;
            break;
        }

        break;
    }
    
    if(ctx != NULL)     EVP_MD_CTX_free(ctx);
    if(pubkey != NULL)  EVP_PKEY_free(pubkey);

    return status;
}

int Add_User_License(unsigned char *path_license, unsigned char *path_public_key)
{
    int status = 0;
    
    while(TRUE)
    { 
        Check_Buffer_License();

        g_baUserLicenseToCheck = read_file(path_license, &g_UserLicenseToCheck_Len);
        if(g_baUserLicenseToCheck == NULL)
        {
            status = 1;
            break;
        }
   
        //
		// Longitud del campos "License data". Es un arreglo de longitud L, en donde
        // los primero 4 bytes indican el "payload len".
        int license_info_total_len =    0x1000000 * g_baUserLicenseToCheck[0] + 
                                        0x10000 * g_baUserLicenseToCheck[1] + 
                                        0x100 * g_baUserLicenseToCheck[2] + 
                                        g_baUserLicenseToCheck[3] + 4;

        int singature_len = g_UserLicenseToCheck_Len - license_info_total_len;

        if (verify_signature(g_baUserLicenseToCheck, license_info_total_len,
            g_baUserLicenseToCheck + license_info_total_len, singature_len,
            path_public_key) != 0)
        {
            status = 2;
            break;
        }

        if(Add_Client_License_Info(g_baUserLicenseToCheck + 4) != 0)
        {
            status = 3;
            break;
        }

        break;
    }

    Check_Buffer_License();
    return status;
}