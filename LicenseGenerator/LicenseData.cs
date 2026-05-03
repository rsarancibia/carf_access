using System.Security.Cryptography;
using System.Text;
using static System.Runtime.InteropServices.JavaScript.JSType;

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

        private const int cUSER_LEN = 128;
        private const int cHWID_LEN = 64;
        private const int cDAYS_LEN = 16;

        private const int cINFO_LEN_LEN = 4;


        private const int cHASH_LEN = 32;

        private RSA?    g_oRsa;
        private SHA256? g_oSha256;

        public class Data
        {
            byte[] User = new byte[cUSER_LEN];
            byte[] HwId = new byte[cHWID_LEN];
            byte[] Days = new byte[cDAYS_LEN];
            byte[] SerialData = new byte[cUSER_LEN + cHWID_LEN + cDAYS_LEN + cINFO_LEN_LEN];

            public Data() {

                User__Clear();
                HwId__Clear();
                Days__Clear();
            }

            public void User__Set(string user_name_in)
            {
                int total_to_handle = user_name_in.Length;

                if (user_name_in.Length > (User.Length - 1))
                {
                    total_to_handle = User.Length - 1;
                }
                
                int bytesWritten = Encoding.UTF8.GetBytes(
                    user_name_in,
                    0,
                    total_to_handle,
                    User,
                    User.Length - total_to_handle
                );

                Field_Set(User);
            }

            public void HwId__Set(string hwid_in)
            { 
                int total_to_handle = hwid_in.Length;

                if (hwid_in.Length > (HwId.Length - 1))
                {
                    total_to_handle = HwId.Length - 1;
                }

                int bytesWritten = Encoding.UTF8.GetBytes(
                    hwid_in,
                    0,
                    total_to_handle,
                    HwId,
                    HwId.Length - total_to_handle
                );

                Field_Set(HwId);
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

            public byte[] Serialize()
            {
                byte[] len = BitConverter.GetBytes((int)(SerialData.Length - cINFO_LEN_LEN));
                if (BitConverter.IsLittleEndian)
                {
                    Array.Reverse(len);
                }

                Array.Copy(len, 0, SerialData, 0, cINFO_LEN_LEN);
                Array.Copy(User, 0, SerialData, cINFO_LEN_LEN, User.Length);
                Array.Copy(HwId, 0, SerialData, cINFO_LEN_LEN + User.Length, HwId.Length);
                Array.Copy(Days, 0, SerialData, cINFO_LEN_LEN + User.Length + HwId.Length, Days.Length);

                return SerialData;
            }
        }



        private const string cCOMPANY_NAME  = "EDV";


        public LicenseData()
        {
            g_oRsa = RSA.Create();

            g_oSha256 = SHA256.Create();
        }

        public int Init(string path_private_key)
        {
            int status = 0;

            try
            {
                string pem = File.ReadAllText(path_private_key);

                g_oRsa?.ImportFromPem(pem.ToCharArray());

            }
            catch (Exception e)
            {
                status = 100;            
            }

            return status;
        }

        public void End()
        { 
        
        }


        public int Get_Info_From_Client_License(string b64_encrypted_client_license, ref string client_HwId_plain)
        {
            int status = 0;

            while (true)
            {
                if (g_oRsa is null)
                {
                    status = 1;
                    break;
                }

                if (g_oSha256 is null)
                {
                    status = 2;
                    break;
                }

                try
                {
                    byte[] encryptedData = Convert.FromBase64String(b64_encrypted_client_license);
                    byte[] decrypted = g_oRsa.Decrypt(encryptedData, RSAEncryptionPadding.OaepSHA1);

                    
                    
                    if (decrypted.Length < cHASH_LEN)
                    {
                        status = 3;
                        break;
                    }

                    byte[] hash = new byte[cHASH_LEN];
                    byte[] hwid = new byte[decrypted.Length - cHASH_LEN];

                    Array.Copy(decrypted, 0, hash, 0, hash.Length);
                    
                    Array.Copy(decrypted, cHASH_LEN, hwid, 0, hwid.Length);

                    byte[] hashHwId = g_oSha256.ComputeHash(hwid);

                    bool iguales = hash.SequenceEqual(hashHwId);

                    if (!iguales)
                    {
                        status = 4;
                        break;
                    }

                    client_HwId_plain = Encoding.UTF8.GetString(hwid);
                }
                catch
                {
                    status = 100;
                }

                break;
            }

            return status;
        }


        public long Set__Expiration_Date(int days_from_now)
        {
            //int dias = 7;

            //DateTime expiryUtc = DateTime.UtcNow.AddDays(dias);

            //long expiryEpoch = ((DateTimeOffset)expiryUtc).ToUnixTimeSeconds();

            //Console.WriteLine(expiryEpoch);

            return DateTimeOffset.UtcNow.AddDays(days_from_now).ToUnixTimeSeconds();
        }

        public int Build_License(string file_path_out_license, Data oDataLic)
        {
            int status = 0;

            try {

                while (true)
                {
                    if (g_oRsa is null)
                    {
                        status = 1;
                        break;
                    }

                    byte[] dataSerialized = oDataLic.Serialize();

                    byte[] signature = g_oRsa.SignData(
                        dataSerialized,
                        HashAlgorithmName.SHA256,
                        RSASignaturePadding.Pkcs1
                    );

                    //string signatureBase64 = Convert.ToBase64String(signature);

                    using (var fs = new FileStream(file_path_out_license, FileMode.Create, FileAccess.Write))
                    {
                        // Escribir data
                        fs.Write(dataSerialized, 0, dataSerialized.Length);

                        // Escribir firma
                        fs.Write(signature, 0, signature.Length);
                    }

                    break;
                }
            }
            catch {

                status = 100;
            }

            return status;
        }

    }
}
