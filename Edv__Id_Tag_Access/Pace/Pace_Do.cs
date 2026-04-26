using Edv__Id_Tag_Access.Cedula;
using System.Runtime.InteropServices;

namespace Edv__Id_Tag_Access.Pace
{
    internal static class Pace_Do
    {

        private static int glb_Secret_size  = 91;
        private static byte[] glb_sMrz      = new byte[glb_Secret_size];


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int OpenPACE_Init();

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int OpenPACE_SHA256(byte[] input, int inputLen, byte[] output);


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr EAC_Create();

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void EAC_Free(IntPtr ctx);

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr PACE_CreateSecret(byte[] secret, int len);

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void PACE_FreeSecret(IntPtr sec);

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int PACE_Step1( IntPtr ctx, IntPtr sec, byte[] output, ref int outputLen);

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int EAC_InitCardAccess(IntPtr ctx, byte[] data, UIntPtr dataLen);

        //////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////
        public static byte[] Get_SecretFormat(byte[] DocNum, byte[] BirthDay, byte[] ExpDate)
        {
            // llenar con '<'
            glb_sMrz.AsSpan().Fill((byte)'<');

            // último byte en 0
            glb_sMrz[glb_Secret_size - 1] = 0;

            Array.Copy(DocNum, 0, glb_sMrz, 5, DocNum.Length);

            Array.Copy(BirthDay, 0, glb_sMrz, 30, BirthDay.Length);

            Array.Copy(ExpDate, 0, glb_sMrz, 38, ExpDate.Length);

            return glb_sMrz;
       }


        //private const int cSHA1_OUTPUT_LEN = 20;

        //private const int cKSEED_LEN = 24;
        //private const int cKM_LEN = 32;


        //public static bool Compute_K_Keys(byte[] serial_cedula)
        //{ 
        //    bool status = false;

        //    byte[] KSeed = new byte[cKSEED_LEN];

        //    byte[] KM = new byte[cKM_LEN];

        //    while (true)
        //    {
        //        Array.Fill(KSeed, (byte)0x00);

        //        Array.Copy(SecUtils.ComputeSHA1(serial_cedula), 0, KSeed, 0, cSHA1_OUTPUT_LEN);

        //        KSeed[cKSEED_LEN - 1] = 3;

        //        Array.Copy(SecUtils.ComputeSHA256(KSeed), 0, KM, 0, cKM_LEN);

        //        Log.Logger.HexDump(KM, data_lenght: KM.Length, message: "KM Key");


        //        //T22000129364081251010318
        //        //bool result = Cedula_Info.Set_Info("T22000129364081251010318");

        //        //byte[] rama = SecUtils.ComputeSHA1("T22000129364081251010318");
        //        //Log.Logger.HexDump(rama, data_lenght: rama.Length, message: "SHA1 of T22000129364081251010318");

        //        //byte[] rama2 = SecUtils.ComputeSHA1(Cedula_Info.baSerial);
        //        //Log.Logger.HexDump(rama2, data_lenght: rama2.Length, message: "SHA1 of Cedula_Info.baSerial");

        //                        status = true;
        //        break;
        //    }

        //    return status;

        //}

    }
}
