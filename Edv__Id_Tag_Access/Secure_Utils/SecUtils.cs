
using System.Runtime.InteropServices;

namespace Edv__Id_Tag_Access.Secure_Utils
{
    internal class SecUtils
    {

        public const int MAX_KEY_LEN = 384;
        public const int MAX_CRYPT_DATA_LEN = 4096;

        // NID_des_ecb
        public const int NID_des_ecb_KeySize = 8;
        public const int NID_des_ecb_BlockSize = 8;

        // NID_des_cbc
        public const int NID_des_cbc_KeySize = 8;
        public const int NID_des_cbc_IVSize = 8;
        public const int NID_des_cbc_BlockSize = 8;

        // NID_des_ede_cbc
        public const int NID_des_ede_cbc_KeySize = 16;
        public const int NID_des_ede_cbc_IVSize = 8;
        public const int NID_des_ede_cbc_BlockSize = 8;

        // NID_des_ede3_cbc
        public const int NID_des_ede3_cbc_KeySize = 24;
        public const int NID_des_ede3_cbc_IVSize = 8;
        public const int NID_des_ede3_cbc_BlockSize = 8;

        // NID_aes_128_cbc
        public const int NID_aes_128_cbc_KeySize = 16;
        public const int NID_aes_128_cbc_IVSize = 16;
        public const int NID_aes_128_cbc_BlockSize = 16;


        public const int NID_aes_128_cbc = 419;



        // NID_aes_192_cbc
        public const int NID_aes_192_cbc_KeySize = 24;
        public const int NID_aes_192_cbc_IVSize = 16;
        public const int NID_aes_192_cbc_BlockSize = 16;

        // NID_aes_256_cbc
        public const int NID_aes_256_cbc_KeySize = 32;
        public const int NID_aes_256_cbc_IVSize = 16;
        public const int NID_aes_256_cbc_BlockSize = 16;

        // NID_aes_256_ecb
        public const int NID_aes_256_ecb_KeySize = 32;
        public const int NID_aes_256_ecb_BlockSize = 16;
        // NID_md4
        public const int NID_md4_BlockSize = 16;

        // NID_md5
        public const int NID_md5_BlockSize = 16;

        // NID_sha
        public const int NID_sha_BlockSize = 20;

        // NID_sha1
        public const int NID_sha1_BlockSize = 20;

        // NID_sha224
        public const int NID_sha224_BlockSize = 28;

        // NID_sha256
        public const int NID_sha256_BlockSize = 32;

        // NID_sha384
        public const int NID_sha384_BlockSize = 48;

        // NID_sha512
        public const int NID_sha512_BlockSize = 64;

        public const int CODER_CRYPT_DECRYPT = 0;
        public const int CODER_CRYPT_ENCRYPT = 1;
        public const int CODER_CRYPT_DECRYPT_PKCS5 = 2;
        public const int CODER_CRYPT_ENCRYPT_PKCS5 = 3;


        // GETHASHFILE
        public const int CODER_ERROR_NO_ERROR = 0;
        public const int CODER_ERROR_DIGESTFILE_OPEN = 1;
        public const int CODER_ERROR_DIGESTFILE_NID = 2;
        public const int CODER_ERROR_DIGESTFILE_OVERFLOW = 3;
        public const int CODER_ERROR_DIGESTFILE_OVERFLOW_ASCII = 4;
        public const int CODER_ERROR_DIGESTFILE_INIT = 5;
        public const int CODER_ERROR_DIGESTFILE_READ = 6;
        public const int CODER_ERROR_DIGESTFILE_UPDATE = 7;
        public const int CODER_ERROR_DIGESTFILE_FINAL = 8;

        // GETHASH
        public const int CODER_ERROR_DIGEST_NID = 10;
        public const int CODER_ERROR_DIGEST_OVERFLOW = 11;
        public const int CODER_ERROR_DIGEST_OVERFLOW_ASCII = 12;
        public const int CODER_ERROR_DIGEST_INIT = 13;
        public const int CODER_ERROR_DIGEST_READ = 14;
        public const int CODER_ERROR_DIGEST_UPDATE = 15;
        public const int CODER_ERROR_DIGEST_FINAL = 16;

        // GETKEY
        public const int CODER_ERROR_KEY_CRYPTNID = 20;
        public const int CODER_ERROR_KEY_HASHTNID = 21;
        public const int CODER_ERROR_KEY_BYTESTOKEY = 22;

        // GETCRYPT
        public const int CODER_ERROR_CRYPT_NID = 30;
        public const int CODER_ERROR_CRYPT_DECRYPTPAD = 31;
        public const int CODER_ERROR_CRYPT_ENCRYPTPAD = 32;
        public const int CODER_ERROR_CRYPT_DECRYPTPKCS5 = 33;
        public const int CODER_ERROR_CRYPT_MODE = 34;
        public const int CODER_ERROR_CRYPT_OVERFLOW = 35;
        public const int CODER_ERROR_CRYPT_OVERFLOW_ASCII = 36;
        public const int CODER_ERROR_CRYPT_INIT = 37;
        public const int CODER_ERROR_CRYPT_UPDATE = 38;
        public const int CODER_ERROR_CRYPT_FINAL = 39;

        // ...
        public const int CODER_ERROR_STATUS_INI = 1000;
        public const int CODER_ERROR_STATUS_END = 1001;



        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int getCrypt(
        int iCryptNID,
        int iMode,
        byte[] ucpKey,
        int iKeyLen,
        byte[] ucpIv,
        int iIvLen,
        byte[] ucpDataIn,
        int iDataInLen,
        byte[] ucpDataOut,
        ref int ipDataOutLen,
        byte iASCII
    ); 


        public static int TLV_Encode(byte[] tag, int tagLen, byte[] inData, int inData_Offset ,int inDataLen, byte[] outData, int outData_Offset)
        {
            int total_len_out = -1;
            int len_fields = 0;
            int offset = 0;


            while (true)
            {
                // Validaciones
                if (tagLen <= 0 || inDataLen < 0 || inData_Offset < 0 || outData.Length == 0 || outData_Offset < 0)
                {
                    break;
                }

                if(outData_Offset > (outData.Length - 1))
                {
                    break;
                }

                if (inData.Length < (inData_Offset + inDataLen))
                {
                    break;
                }

                if (inDataLen <= 127) len_fields = 1;
                else if (inDataLen <= 255) len_fields = 2;
                else if (inDataLen <= 65535) len_fields = 3;
                else break;

                // Total de salida:
                if ((tagLen + len_fields + inDataLen) <= (outData.Length - outData_Offset))
                {
                    total_len_out = tagLen + len_fields + inDataLen;
                }
                else break;

                //////////////////////////////////////
                // Build TLV
                //////////////////////////////////////

                offset = outData_Offset;

                // Tag
                for (int i = 0; i < tagLen; i++)
                {
                    outData[offset + i] = tag[i];
                }

                offset += tagLen;


                // Length
                switch (len_fields)
                {
                    case 1:
                        outData[offset] = (byte)inDataLen;
                        break;

                    case 2:
                        outData[offset]        = 0x81;
                        outData[offset + 1]    = (byte)inDataLen;
                        break;

                    case 3:
                        outData[offset]     = 0x82;
                        outData[offset + 1] = (byte)((inDataLen >> 8) & 0xFF); 
                        outData[offset + 2] = (byte)(inDataLen & 0xFF);
                        break;
                }

                offset += len_fields;

                Array.Copy(inData, inData_Offset, outData, offset, inDataLen);


                total_len_out = tagLen + len_fields + inDataLen;

                break;            
            }

            return total_len_out;
        }
    }
}
