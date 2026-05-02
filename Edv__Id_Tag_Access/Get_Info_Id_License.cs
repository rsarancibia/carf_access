using System.Management;
using System.Security.Cryptography;
using System.Text;

namespace Edv__Id_Tag_Access
{
    internal class Get_Info_Id_License
    {
        public static string Id()
        {
            string raw = GetCpuId() + GetDiskSerial();


            byte[] info = SHA256.HashData(Encoding.UTF8.GetBytes(raw));

            return "Hola";
        }    

        public static string GetCpuId()
        {

            using var searcher = new ManagementObjectSearcher(
                "SELECT ProcessorId FROM Win32_Processor");

            foreach (ManagementObject obj in searcher.Get())
            {
                return obj["ProcessorId"]?.ToString()?.Trim();
            }

            return string.Empty;
        }


        public static string GetDiskSerial()
        {
            using var searcher = new ManagementObjectSearcher(
                "SELECT SerialNumber FROM Win32_PhysicalMedia");

            foreach (ManagementObject obj in searcher.Get())
            {
                var serial = obj["SerialNumber"]?.ToString()?.Trim();
                if (!string.IsNullOrEmpty(serial))
                    return serial;
            }

            return string.Empty;
        }
    }
}
