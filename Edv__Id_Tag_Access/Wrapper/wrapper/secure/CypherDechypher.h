#ifndef __CHYPHER_DECHYPHER_H__
#define __CHYPHER_DECHYPHER_H__


int test_rsa_sign_verify();

int rsa_encrypt(const unsigned char* plaintext, size_t plaintext_len,
    unsigned char* ciphertext, size_t* ciphertext_len,
    const char* pubkey_string);

int rsa_decrypt(const unsigned char* ciphertext, size_t ciphertext_len,
    unsigned char* plaintext, size_t* plaintext_len,
    const char* privkey_path);

int Add_User_License(unsigned char* path_license, unsigned char* path_public_key);


#endif