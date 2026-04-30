using Edv__Id_Tag_Access.Cedula;
using Edv__Id_Tag_Access.Devices.FingerReader;
using Edv__Id_Tag_Access.Devices.NFC;
using Edv__Id_Tag_Access.ImgEngines;
using Edv__Id_Tag_Access.myLog;
using Edv__Id_Tag_Access.Pace;
using Serilog;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using System.Text;
using static System.Formats.Asn1.AsnWriter;

namespace Edv__Id_Tag_Access
{
    public class EdvLibAPi
    {
        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr PACE_CreateSecret(byte[] secret, int len);

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Edv_Set_Template(byte[] template, int template_len);


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Register_Log_callback(Log_Callback cb);


        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void Log_Callback(
            IntPtr infoBuffer,
            int infoBufferLen
        );

        private static Log_Callback? _Log_Callback_Ref;


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Edv_Init();


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Edv_Moc();

        private const string cREADER_NAME = "HID Global OMNIKEY 5022 Smart Card Reader 0"; 


        private NFC_Reader?         glb_Nfc_Reader = null;
        private FingerPrintReader?  glb_FingerPrint_Reader = null;
        private Cedula_IO?          glb_Cedula_IO = null;


        public delegate void api_Tag_Type(string tag_type);

        private api_Tag_Type? glb_api_Tag_Type = null;


        public int Init(api_Tag_Type tag_type)
        {
            int status = 0;
            int res = 0;

            while (true)
            {
                LoggerConfig.Init();

                Register_Log_callback(Log_Fnc);

                Log.Information("DLL Init - Start"); 

                //Test();

                string qr_rafa = "https://portal.sidiv.registrocivil.cl/docstatus?RUN=12845657-0&type=CEDULA&serial=B5F089055&mrz=B5F089055075040793504071&name=RAFAEL%20SEBASTI%C1N%20%20ARANCIBIA%20AMPUERO";
                Cedula_Info.Set_Info__Qr(qr_rafa);


                res = status = Edv_Init();
                if (res != 0)
                {
                    Log.Error("Edv_Init() = " + res.ToString());

                    status = 10;
                    break;
                }

                if (glb_Nfc_Reader is not null)
                {
                    status = 1;
                    break;
                }

                if(glb_FingerPrint_Reader is not null)
                {
                    status = 2;
                    break;
                }

                glb_Nfc_Reader = new();
                if(glb_Nfc_Reader.Init(cREADER_NAME) != 0)
                    {
                    status = 3;
                    break;
                }

                glb_FingerPrint_Reader = new();
                if (glb_FingerPrint_Reader.Init() != 0)
                {
                    status = 4;
                    break;
                }

                glb_api_Tag_Type = tag_type;

                if (glb_Cedula_IO is null)
                {
                    glb_Cedula_IO = new();

                    glb_Cedula_IO.SetIO(glb_Nfc_Reader.Detect_Card, glb_Nfc_Reader.IO, (x) => glb_api_Tag_Type(x));
                }

                break;
            }

            Log.Information("DLL Init - Done : " + status.ToString());

            return status;
        }

        public void Test_Lector()
        { 
            if (glb_Cedula_IO is not null)
            {
                //Cedula_IO.Test_Reader();

                glb_Nfc_Reader?.Detect_Card();

                Edv_Moc();

            }
        }


        public int Id_Tag__MOC()
        {
            int status = 0;

            while(true)
            {
                if (glb_Nfc_Reader is null)
                {
                    status = 1;
                    break;
                }

                if (glb_Cedula_IO is null)
                {
                    status = 2;
                    break;
                }

                glb_Cedula_IO.MOC();

                break;
            }

            return status;
        }

        public int Finger__Capture(ref byte[] raw_image)
        {
            int status = 0;

            while (true)
            {
                if (glb_FingerPrint_Reader is null)
                {
                    status = 1;
                    break;
                }

                if (glb_FingerPrint_Reader.CaptureFingerPrint() != 0)
                {
                    status = 2;
                    break;
                }

                raw_image = glb_FingerPrint_Reader.Img_Raw_Buffer ?? Array.Empty<byte>();

                byte[] iso_image = Array.Empty<byte>();

                if (Finger__Get_Iso_19794_2(    raw_image,
                                                (ushort)glb_FingerPrint_Reader.Img_Width,
                                                (ushort)glb_FingerPrint_Reader.Img_Height,
                                                (ushort)glb_FingerPrint_Reader.Img_dpi, ref iso_image) != 0)
                {
                    status = 3;
                    break;
                }

                byte[] iso_compact = Array.Empty<byte>();

                if (IsoCompact.Convert( iso_image,
                                        IsoCompact.MAX_MOC_ISO_LEN_TO_TX,
                                        IsoCompact.ISOCOMPACT_SCALE,
                                        ref iso_compact) != 0)
                {
                    status = 3;
                    break;
                }

                Edv_Set_Template(iso_compact, iso_compact.Length);

                Log.Logger.HexDump(iso_compact,data_lenght : 16);

                break;
            }

            return status;
        }


        public int Finger__Get_Iso_19794_2(byte[] raw_img, ushort raw_width, ushort raw_height, ushort raw_dpi, ref byte[] raw_iso)
        {
            int status = 0;

            while (true)
            {
                byte[] fmd = new byte[Fp_FX.FJFX_FMD_BUFFER_SIZE];
                uint size = (uint)fmd.Length;

                int result = Fp_FX.fjfx_create_fmd_from_raw(
                    raw_img,
                    raw_dpi,
                    raw_height,
                    raw_width,
                    Fp_FX.FJFX_FMD_ISO_19794_2_2005,
                    fmd,
                    ref size
                );

                raw_iso = new byte[size];

                Array.Copy(fmd, raw_iso, size);

                Log.Logger.HexDump(raw_iso,data_lenght : raw_iso.Length, message: "ISO 19794-2 Fingerprint Data");

                break;
            }

            return status;
        }

        public void Finger__Get_Width_Height_Dpi(ref int image_width, ref int image_height, ref int dpi)
        {
            if (glb_FingerPrint_Reader is null)
            {
                return;
            }
            image_width = glb_FingerPrint_Reader.Img_Width;
            image_height = glb_FingerPrint_Reader.Img_Height;
            dpi = glb_FingerPrint_Reader.Img_dpi;
        }
        public void End()
        {
            try {

                Log.Information("DLL End - Start");


                glb_Nfc_Reader?.End();
                glb_Nfc_Reader = null;

                glb_FingerPrint_Reader?.End();
                glb_FingerPrint_Reader = null;
            }
            catch { 
            
            }

            Log.Information("DLL End - Done");
        }

        private void Log_Fnc(IntPtr buffer, int bufferLen)
        {
            // Copiar TX desde puntero nativo → array .NET
            byte[] managedTx = new byte[bufferLen];
            Marshal.Copy(buffer, managedTx, 0, bufferLen);

            string s = Encoding.UTF8.GetString(managedTx);
            
            Log.Information(s);
        }




        /// <summary>
        /// ///////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        private void Test()
        {

            // Chiqui !
            bool boStatus;
            string qr_rafa = "https://portal.sidiv.registrocivil.cl/docstatus?RUN=12845657-0&type=CEDULA&serial=B5F089055&mrz=B5F089055075040793504071&name=RAFAEL%20SEBASTI%C1N%20%20ARANCIBIA%20AMPUERO";

            //public static string Get_Info_From_Qr(string qr_link)

            boStatus =  Cedula_Info.Set_Info__Qr(qr_rafa);



            boStatus = Cedula_Info.Set_Info__Serial("999999001982030142603014");

            int statusPace = Pace_Do.OpenPACE_Init();


            //byte[] data = System.Text.Encoding.UTF8.GetBytes("hola");
            //byte[] hash = new byte[32];

            //int res = Pace_Do.OpenPACE_SHA256(data, data.Length, hash);

            //Console.WriteLine(BitConverter.ToString(hash));


            //IntPtr ctx = Pace_Do.EAC_Create();

            //if (ctx == IntPtr.Zero)
            //{
            //    Console.WriteLine("Error creando contexto");
            //}
            //else
            //{
            //    Console.WriteLine("Contexto OK");
            //}

            //Pace_Do.EAC_Free(ctx);


            int size = 91; // el tamaño que necesites
            byte[] sMrz = new byte[size];

            // llenar con '<'
            sMrz.AsSpan().Fill((byte)'<');

            // último byte en 0
            sMrz[size - 1] = 0;

            Array.Copy(Cedula_Info.baNumeroDocumento, 0, sMrz, 5, Cedula_Info.baNumeroDocumento.Length);

            Array.Copy(Cedula_Info.baFechaNacimiento, 0, sMrz, 30, Cedula_Info.baFechaNacimiento.Length);

            Array.Copy(Cedula_Info.baFechaExpiracion, 0, sMrz, 38, Cedula_Info.baFechaExpiracion.Length);

            IntPtr sec = Pace_Do.PACE_CreateSecret(sMrz, sMrz.Length);

            if (sec == IntPtr.Zero)
            {
                Console.WriteLine("Error creando secreto");
            }
            else
            {
                Console.WriteLine("Secreto OK");
            }

            Pace_Do.PACE_FreeSecret(sec);

            ///////////////////////////////////////////////////////////////////////

            //statusPace = Pace_Do.OpenPACE_Init();

            // 1. Crear contexto
            IntPtr ctx = Pace_Do.EAC_Create();

            // 2. Inicializar con CardAccess
            //byte[] cardAccess = new byte[]
            //{
            //    0x31,0x1F,
            //    0x30,0x1D,
            //    0x06,0x0A,0x04,0x00,0x7F,0x00,0x07,0x02,0x02,0x04,0x02,
            //    0x30,0x0F,
            //    0x02,0x01,0x02,
            //    0x02,0x01,0x01,
            //    0x02,0x01,0x01,
            //    0x02,0x01,0x01
            //};


            //            byte[] cardAccess = new byte[]
            //{
            //    0x30,0x1C,
            //    0x06,0x0A,0x04,0x00,0x7F,0x00,0x07,0x02,0x02,0x04,0x02,
            //    0x30,0x0E,
            //    0x02,0x01,0x02,
            //    0x02,0x01,0x01,
            //    0x02,0x01,0x01,
            //    0x02,0x01,0x01
            //};


            //        byte[] cardAccess = new byte[]
            //        {
            //0x30,0x12,
            //0x06,0x0A,0x04,0x00,0x7F,0x00,0x07,0x02,0x02,0x04,0x02,
            //0x04,0x02,
            //0x01,0x02,
            //0x02,0x01,0x0F
            //        };


    //        byte[] cardAccess = new byte[]
    //        {
    //0x30,0x81,0x80,  // ← PATCH (antes era 0x31)

    //0x30,0x12,
    //0x06,0x0A,0x04,0x00,0x7F,0x00,0x07,0x02,0x02,0x04,0x02,
    //0x04,0x02,
    //0x01,0x02,
    //0x02,0x01,0x0F,

    //0x30,0x15,
    //0x06,0x09,0x04,0x00,0x7F,0x00,0x07,0x02,0x02,0x0C,0x01,
    //0x30,0x03,0x02,0x01,0x01,
    //0x30,0x03,0x03,0x01,0x00,

    //0x30,0x15,
    //0x06,0x09,0x04,0x00,0x7F,0x00,0x07,0x02,0x02,0x0C,0x02,
    //0x30,0x03,0x02,0x01,0x02,
    //0x30,0x03,0x03,0x01,0x00,

    //0x30,0x1D,
    //0x06,0x09,0x04,0x00,0x7F,0x00,0x07,0x02,0x02,0x0C,0x03,
    //0x30,0x03,0x02,0x01,0x03,
    //0x30,0x0B,
    //0x03,0x03,0x04,0x30,0x10,
    //0x85,0x01,0x0C,
    //0x87,0x01,0xFF,

    //0x30,0x1D,
    //0x06,0x09,0x04,0x00,0x7F,0x00,0x07,0x02,0x02,0x0C,0x03,
    //0x30,0x03,0x02,0x01,0x05,
    //0x30,0x0B,
    //0x03,0x03,0x04,0x30,0x10,
    //0x85,0x01,0x0C,
    //0x87,0x01,0xFF
    //        };

            byte[] respuestaReadBin = new byte[]
                {
            0x31, 0x81, 0x80, 0x30, 0x12, 0x06, 0x0A, 0x04, 0x00, 0x7F, 0x00, 0x07, 0x02, 0x02, 0x04, 0x02,
0x04, 0x02, 0x01, 0x02, 0x02, 0x01, 0x0F, 0x30, 0x15, 0x06, 0x09, 0x04, 0x00, 0x7F, 0x00, 0x07,
0x02, 0x02, 0x0C, 0x01, 0x30, 0x03, 0x02, 0x01, 0x01, 0x30, 0x03, 0x03, 0x01, 0x00, 0x30, 0x15,
0x06, 0x09, 0x04, 0x00, 0x7F, 0x00, 0x07, 0x02, 0x02, 0x0C, 0x02, 0x30, 0x03, 0x02, 0x01, 0x02,
0x30, 0x03, 0x03, 0x01, 0x00, 0x30, 0x1D, 0x06, 0x09, 0x04, 0x00, 0x7F, 0x00, 0x07, 0x02, 0x02,
0x0C, 0x03, 0x30, 0x03, 0x02, 0x01, 0x03, 0x30, 0x0B, 0x03, 0x03, 0x04, 0x30, 0x10, 0x85, 0x01,
0x0C, 0x87, 0x01, 0xFF, 0x30, 0x1D, 0x06, 0x09, 0x04, 0x00, 0x7F, 0x00, 0x07, 0x02, 0x02, 0x0C,
0x03, 0x30, 0x03, 0x02, 0x01, 0x05, 0x30, 0x0B, 0x03, 0x03, 0x04, 0x30, 0x10, 0x85, 0x01, 0x0C,
0x87, 0x01, 0xFF };


            byte[] cardAccess = new byte[256];
            Array.Copy(respuestaReadBin, 0, cardAccess, 0, respuestaReadBin.Length);



            //int res = OpenPACE.EAC_InitCardAccess(
            //    ctx,
            //    cardAccess,
            //    (UIntPtr)cardAccess.Length);

            //Console.WriteLine("InitCardAccess: " + res);




            int init = Pace_Do.EAC_InitCardAccess(ctx, cardAccess, (UIntPtr)(cardAccess.Length));


            Console.WriteLine("InitCardAccess: " + init);

            // 3. Crear secreto (ejemplo)
            //byte[] mrz = System.Text.Encoding.ASCII.GetBytes("123456789");

            sec = Pace_Do.PACE_CreateSecret(sMrz, sMrz.Length);

            // 4. Step1
            byte[] buffer = new byte[256];
            int len = buffer.Length;

            //int res = Pace_Do.PACE_Step1(ctx, sec, buffer, ref len);

            //Console.WriteLine("Step1 result: " + res);
            //Console.WriteLine("Length: " + len);




            //ctx = Pace_Do.EAC_Create();
            //sec = Pace_Do.PACE_CreateSecret(sMrz, sMrz.Length);
            //byte[] buffer = new byte[256];
            //int len = buffer.Length;

            //int res2 = Pace_Do.PACE_Step1(ctx, sec, buffer, ref len);

            //if (res2 == 1)
            //{
            //    Console.WriteLine("Step1 OK, length: " + len);
            //}
            //else
            //{
            //    Console.WriteLine("Error en Step1");
            //}
            //Pace_Do.EAC_Free(ctx);
            //Pace_Do.PACE_FreeSecret(sec);




            //Pace_Do.Compute_K_Keys(Cedula_Info.baSerial);


            //Cedula_Info.Set_Info("B5F089055075040793504071");
            //Log.Information("Cedula Info: " + Cedula_Info.strNumeroDocumento + ", " + Cedula_Info.strFechaNacimiento + ", " + Cedula_Info.strFechaExpiracion);


            //T22000129364081251010318
            //bool result = Cedula_Info.Set_Info("T22000129364081251010318");

            //byte[] rama = SecUtils.ComputeSHA1("T22000129364081251010318");
            //Log.Logger.HexDump(rama, data_lenght: rama.Length, message: "SHA1 of T22000129364081251010318");

            //byte[] rama2 = SecUtils.ComputeSHA1(Cedula_Info.baSerial);
            //Log.Logger.HexDump(rama2, data_lenght: rama2.Length, message: "SHA1 of Cedula_Info.baSerial");

            ///////////////////////////////////////////////////////////////////////////////////////////////


            //byte[] data = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
            //                0x10, 0x11,0x12,0x13,0x14};//,0x15,0x16,0x17};

            //Log.Logger.HexDump(data, data_lenght: data.Length, message: "Hex Dump of Data");

            //HexDumpInternal(data, data.Length);

        }

    }
}
