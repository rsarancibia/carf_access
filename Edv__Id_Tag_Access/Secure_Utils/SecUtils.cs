using System.Security.Cryptography;
using System.Text;

namespace Edv__Id_Tag_Access.Secure_Utils
{
    internal class SecUtils
    {
        public static int TLV(byte[] tag, int tagLen, byte[] inData, int inLen, byte[] outData, int outLen)
        {
            int resultLen = -1;
            int pos = 0;

            if (outLen >= tagLen)
            {
                // Copiar TAG
                Buffer.BlockCopy(tag, 0, outData, pos, tagLen);
                pos += tagLen;

                // LENGTH (BER-TLV)
                if (inLen > 0xFF)
                {
                    if (outLen >= (pos + 3))
                    {
                        outData[pos++] = 0x82;
                        outData[pos++] = (byte)((inLen >> 8) & 0xFF);
                        outData[pos++] = (byte)(inLen & 0xFF);
                    }
                }
                else if (inLen > 0x7F)
                {
                    if (outLen >= (pos + 2))
                    {
                        outData[pos++] = 0x81;
                        outData[pos++] = (byte)(inLen & 0xFF);
                    }
                }
                else
                {
                    if (outLen >= (pos + 1))
                    {
                        outData[pos++] = (byte)inLen;
                    }
                }

                // VALUE
                if (outLen >= (pos + inLen))
                {
                    if (inLen > 0 && inData != null)
                    {
                        Buffer.BlockCopy(inData, 0, outData, pos, inLen);
                    }
                    resultLen = pos + inLen;
                }
            }

            return resultLen;
        }


        public static byte[] ComputeSHA256(byte[] data)
        {
            using var sha256 = SHA256.Create();
            return sha256.ComputeHash(data);
        }

        public static byte[] ComputeSHA256(string input)
        {
            byte[] data = Encoding.UTF8.GetBytes(input);
            return ComputeSHA256(data);
        }

        public static byte[] ComputeSHA1(byte[] data)
        {
            using var sha1 = SHA1.Create();
            return sha1.ComputeHash(data);
        }

        public static byte[] ComputeSHA1(string input)
        {
            byte[] data = Encoding.UTF8.GetBytes(input);
            return ComputeSHA1(data);
        }


        public static byte[] ComputeHmacSHA256(byte[] key, byte[] data)
        {
            using var hmac = new HMACSHA256(key);
            return hmac.ComputeHash(data);
        }
    }
}
