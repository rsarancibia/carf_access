
#include <generictypedefs.h>
#include <CypherDechypher.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <string.h>
#include <stdio.h>


EVP_PKEY* load_public_key_from_string(const char* pubkey_pem);
EVP_PKEY* load_private_key_from_string(const char* privkey_pem);



/// <summary>
// openssl genpkey -algorithm RSA -out private.pem -pkeyopt rsa_keygen_bits:2048 && openssl pkey -in private.pem -pubout -out public.pem 
/// </summary>

static const char* TEST_PRIVATE_KEY_PEM_OLD =
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCYNjH297Eq7SwC\n"
"ULG5U87sefgX/KnbgmIisH9IoY76jJUOGlGREIWbnraOwFZVPRCfdN0FV/JLe+nX\n"
"Sph7rJ+7JVQML35kobWRekbHDgyQKTdVM42zvKmxg29dMFcSXcX2HR1EMgNNNzE7\n"
"HjYgM8pIaU/DKb6YU/GsrZPyLavsZPSMB8vdtS4Bhr9W2Al+ln20ix2h91d249w3\n"
"xOSlmkUqMTRWOT3tSQ9aSfCj/aTbbb3I9g66wEDiJjvAMaGY03MdqUX9fzhuYU87\n"
"WwdunpJhXjmp5FtD+3p3D2BFet/L0FhJfnMgMa2dPU315hD6G2FLIUKqVGvmvXYr\n"
"kpJcf+yZAgMBAAECggEADkCj3tLcZm85G9njvF1oPq0gkZlta8Iaj5I8hufm2Bfe\n"
"FKka+mUER7k9saH5q4dZZT73e6q7o70NfUTtQIZeWhMhmrb/RCu+sUm0cyrh+wU6\n"
"+snGIt5/esFahgyy2IPKl7Qr84Dl3X71p4oZihwjXLKsvvLrl36jvnYt6GNFqg+4\n"
"eTjEmlSSEVTipRCEe6f1mzX5+YqxDH3D+oFiFHHoLZU1YG3I4cmFUunW5cPDTMJp\n"
"G9u5SZLhibslpLA911n2fYTxLGwYCbwYzlTyFtAdRMlfEdEboKZmtcRYpjwijoYI\n"
"OiXxueoNONyXJQ6+lf2uITvFpH8FwBJtL+pYJsKZAQKBgQDLInYXx9FOyg8A+ZVc\n"
"4P7kYTWJAqUedWzIoh0JFxWlfGmNPlzOsgDvNAQ2PhvAQOavA1d3ASovbPhRU2B6\n"
"/G8ZK8iAclQO8t0toGtzqjN+1qzlKpm7SPX9qgqUYsI3t4iic4euOlnwZ9WUd38R\n"
"lcC/gQrOX6g0omNyA99yBnynAQKBgQC/0xI2x6tRl6h4QZXFpMPAGBkpaNkKzVEI\n"
"N0WpzmW6NLT9yl7mrI1NjkKbdJi0F2HYviYVI54wGBCpcUUUldxvM0RAuSJvOXEN\n"
"+aoAkGzt6C87YuashsrMQk7njQX0uM6nS5JqEMX3oEfmT+3l+14y7kafFPXzS8pq\n"
"mN2QqhUdmQKBgAUlLlCT4x1XyOGBcOMVX99xBuYdhwkqeELsuEeOqiGy8Ql+1uL1\n"
"Z0inzKODFzjba/xq3UBKa0MgQ3nr3rm+wyGkFkQKoU5voGKTeaVIXl2MGn0DgzlX\n"
"M8PlFreDeN/oajGTM5CaMcUBHEvSawmK/YLcReXMTnpm0pYEuxgsSeQBAoGBAK5S\n"
"PTolB85VORdX2qjLC8002SDllGAZp6sEt70RwSaPoW8Fimqsopi5UFR/iAoZOaVD\n"
"X85UGPxx0ip5siJ78d/oQf1jgSR8mf3uRgzPMv6cwteAEEr3D7LCC3ynjqMPk1U6\n"
"+yqCZbWcJdrqBjK61acJHIqS/NQFq0Rl+OvvdNL5AoGAQRPRBBtAMKsrBwVj40bL\n"
"LdjrzuBaBiF+Dy7PeghAt1ognD66lrSeYQ5hN1/IRQFAMIlytiNYqurIwwQ8WPRm\n"
"AGTnjWaBEP8NLGdXEREvuIsBYfF/3CaauMxGWINNVayxl0j0xipTaJTqh6VqkNWW\n"
"yE62+2vUQhFZz0x8h9H0TYM=\n"
"-----END PRIVATE KEY-----\n";

static const char* TEST_PUBLIC_KEY_PEM_OLD =
"-----BEGIN PUBLIC KEY-----\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAmDYx9vexKu0sAlCxuVPO\n"
"7Hn4F/yp24JiIrB/SKGO+oyVDhpRkRCFm562jsBWVT0Qn3TdBVfyS3vp10qYe6yf\n"
"uyVUDC9+ZKG1kXpGxw4MkCk3VTONs7ypsYNvXTBXEl3F9h0dRDIDTTcxOx42IDPK\n"
"SGlPwym+mFPxrK2T8i2r7GT0jAfL3bUuAYa/VtgJfpZ9tIsdofdXduPcN8TkpZpF\n"
"KjE0Vjk97UkPWknwo/2k2229yPYOusBA4iY7wDGhmNNzHalF/X84bmFPO1sHbp6S\n"
"YV45qeRbQ/t6dw9gRXrfy9BYSX5zIDGtnT1N9eYQ+hthSyFCqlRr5r12K5KSXH/s\n"
"mQIDAQAB\n"
"-----END PUBLIC KEY-----\n";




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


/*

DATA1 = datos + CRC (crc no es necesario)
SIGN = Sign(private_key, DATA1)

LIC = Base64(DATA1 || SIGN)

*/


int test_rsa_sign_verify()
{
    const unsigned char data[] = "Rafael-License-Test";
    size_t data_len = strlen((const char*)data);

    printf("RsaTest - INIT \n");


    EVP_PKEY* priv = NULL;
    EVP_PKEY* pub = NULL;
    EVP_MD_CTX* sign_ctx = NULL;
    EVP_MD_CTX* verify_ctx = NULL;

    unsigned char* sig = NULL;
    size_t sig_len = 0;

    int ok = 0;


	printf("PRIVTE : %s\n", TEST_PRIVATE_KEY_PEM);
	
    printf("PUBLIC : %s\n", TEST_PUBLIC_KEY_PEM);   


    // ---------------------------
    // 1. Cargar claves desde PEM
    // ---------------------------
    BIO* bio_priv = BIO_new_mem_buf(TEST_PRIVATE_KEY_PEM, -1);
    BIO* bio_pub = BIO_new_mem_buf(TEST_PUBLIC_KEY_PEM, -1);

    priv = PEM_read_bio_PrivateKey(bio_priv, NULL, NULL, NULL);
    pub = PEM_read_bio_PUBKEY(bio_pub, NULL, NULL, NULL);

    printf("RsaTest - STP 0\n");

    if (!priv)
    {
        printf("ERROR: Private key load failed\n");
    }

    if (!pub)
    {
        printf("ERROR: Public key load failed\n");
    }


    if (!priv || !pub) goto cleanup;

    // ---------------------------
    // 2. Firmar
    // ---------------------------
    sign_ctx = EVP_MD_CTX_new();

    printf("RsaTest - STP 1\n");

    if (EVP_DigestSignInit(sign_ctx, NULL, EVP_sha256(), NULL, priv) <= 0)
        goto cleanup;

    printf("RsaTest - STP 2\n");

    if (EVP_DigestSignUpdate(sign_ctx, data, data_len) <= 0)
        goto cleanup;

    printf("RsaTest - STP 3\n");

    // obtener tamaño firma
    if (EVP_DigestSignFinal(sign_ctx, NULL, &sig_len) <= 0)
        goto cleanup;

    printf("RsaTest - STP 4\n");

    sig = (unsigned char*)OPENSSL_malloc(sig_len);

    printf("RsaTest - STP 5\n");

    if (EVP_DigestSignFinal(sign_ctx, sig, &sig_len) <= 0)
        goto cleanup;

    printf("RsaTest - STP 6\n");

    // ---------------------------
    // 3. Verificar
    // ---------------------------
    verify_ctx = EVP_MD_CTX_new();

    printf("RsaTest - STP 7\n");

    if (EVP_DigestVerifyInit(verify_ctx, NULL, EVP_sha256(), NULL, pub) <= 0)
        goto cleanup;

    printf("RsaTest - STP 8\n");

    if (EVP_DigestVerifyUpdate(verify_ctx, data, data_len) <= 0)
        goto cleanup;

    printf("RsaTest - STP 9\n");

    int ret = EVP_DigestVerifyFinal(verify_ctx, sig, sig_len);

    printf("RsaTest - STP 10\n");

    if (ret == 1)
    {
        printf("Firma valida\n");
        ok = 1;
    }
    else if (ret == 0)
    {
        printf("Firma invalida\n");
    }
    else
    {
        printf("Error en verificacion\n");
    }

cleanup:

    printf("RsaTest - CLEAN UP \n");

    if (sig) OPENSSL_free(sig);
    if (sign_ctx) EVP_MD_CTX_free(sign_ctx);
    if (verify_ctx) EVP_MD_CTX_free(verify_ctx);
    if (priv) EVP_PKEY_free(priv);
    if (pub) EVP_PKEY_free(pub);
    if (bio_priv) BIO_free(bio_priv);
    if (bio_pub) BIO_free(bio_pub);

    return ok;
}





int rsa_encrypt(const unsigned char* plaintext, size_t plaintext_len,
    unsigned char* ciphertext, size_t* ciphertext_len,
    const char* pubkey_string)
{
    //FILE* fp = fopen(pubkey_path, "r");
    //if (!fp) return -1;

    //EVP_PKEY* pubkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    //fclose(fp);
    //if (!pubkey) return -2;

    EVP_PKEY* pubkey = load_public_key_from_string(pubkey_string);
    if (!pubkey) return -2;


    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pubkey, NULL);
    if (!ctx) return -3;

    if (EVP_PKEY_encrypt_init(ctx) <= 0) return -4;

    // Usar OAEP (recomendado)
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    // Primero obtener tamaño
    if (EVP_PKEY_encrypt(ctx, NULL, ciphertext_len, plaintext, plaintext_len) <= 0)
        return -5;

    // Luego encriptar
    if (EVP_PKEY_encrypt(ctx, ciphertext, ciphertext_len, plaintext, plaintext_len) <= 0)
        return -6;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pubkey);

    return 0;
}


int rsa_decrypt(const unsigned char* ciphertext, size_t ciphertext_len,
    unsigned char* plaintext, size_t* plaintext_len,
    const char* privkey_path)
{
    //FILE* fp = fopen(privkey_path, "r");
    //if (!fp) return -1;

    //EVP_PKEY* privkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    //fclose(fp);
    //if (!privkey) return -2;

    EVP_PKEY* privkey = load_private_key_from_string(privkey_path);
    if (!privkey) return -2;



    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privkey, NULL);
    if (!ctx) return -3;

    if (EVP_PKEY_decrypt_init(ctx) <= 0) return -4;

    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    // Obtener tamaño
    if (EVP_PKEY_decrypt(ctx, NULL, plaintext_len, ciphertext, ciphertext_len) <= 0)
        return -5;

    // Desencriptar
    if (EVP_PKEY_decrypt(ctx, plaintext, plaintext_len, ciphertext, ciphertext_len) <= 0)
        return -6;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(privkey);

    return 0;
}



EVP_PKEY* load_public_key_from_string(const char* pubkey_pem)
{
    BIO* bio = NULL;
    EVP_PKEY* pkey = NULL;

    // Crear BIO desde memoria
    bio = BIO_new_mem_buf(pubkey_pem, -1);
    if (!bio) return NULL;

    // Leer clave pública
    pkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);

    BIO_free(bio);

    return pkey; // NULL si falla
}


EVP_PKEY* load_private_key_from_string(const char* privkey_pem)
{
    BIO* bio = NULL;
    EVP_PKEY* pkey = NULL;

    bio = BIO_new_mem_buf((void*)privkey_pem, -1);
    if (bio == NULL)
        return NULL;

    pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);

    BIO_free(bio);

    return pkey;

}
