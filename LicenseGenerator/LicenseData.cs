using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;


/*
 * 
 * 

string cpu = GetCpuId();
string disk = GetDiskId();

string raw = cpu + disk;

byte[] hash = SHA256.HashData(Encoding.UTF8.GetBytes(raw));


*/

namespace LicenseGenerator
{
    internal class LicenseData
    {

        private const int cFIELD_ENABLE     = 0xFF;
        private const int cFIELD_DISABLE    = 0;

        public class Data
        {
            byte[] User = new byte[128];
            byte[] HwId = new byte[64];
            byte[] Days = new byte[16];
 
            public Data() {

                User__Clear();
                HwId__Clear();
                Days__Clear();
            }

            public void User__Set(string user_name)
            {
                int total_to_handle = user_name.Length;

                if (user_name.Length > (User.Length - 1))
                {
                    total_to_handle = User.Length - 1;
                }
                
                int bytesWritten = Encoding.UTF8.GetBytes(
                    user_name,
                    0,
                    total_to_handle,
                    User,
                    User.Length - total_to_handle
                );

                Field_Set(User);
            }

            void User__Clear()
            {
                Field_Clear(User);
            }

            void HwId__Clear()
            {
                Field_Clear(HwId);
            }

            void Days__Clear()
            {
                Field_Clear(Days);
            }

            void Field_Clear(byte[] buffer)
            {
                Array.Fill<byte>(buffer, 0);
                buffer[0] = cFIELD_DISABLE;
            }

            void Field_Set(byte[] buffer)
            {
                buffer[0] = cFIELD_ENABLE;
            }
        }



        private const string cCOMPANY_NAME  = "EDV";
        private static string path_Private_Key_file = "";


        public static void Set__Private_Key(string path)
        {
            path_Private_Key_file = path;
        }



        private static byte[] SignData(byte[] data, string privateKeyPem)
        {
            using (RSA rsa = RSA.Create())
            {
                rsa.ImportFromPem(privateKeyPem);

                return rsa.SignData(
                    data,
                    HashAlgorithmName.SHA256,
                    RSASignaturePadding.Pkcs1
                );
            }
        }


        public long Set__Expiration_Date(int days_from_now)
        {
            /*
            int dias = 7;

DateTime expiryUtc = DateTime.UtcNow.AddDays(dias);

long expiryEpoch = ((DateTimeOffset)expiryUtc).ToUnixTimeSeconds();

Console.WriteLine(expiryEpoch);
            */

            return DateTimeOffset.UtcNow.AddDays(days_from_now).ToUnixTimeSeconds();
        }

    }
}
