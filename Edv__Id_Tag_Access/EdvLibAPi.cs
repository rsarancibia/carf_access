using Edv__Id_Tag_Access.Cedula;
using Edv__Id_Tag_Access.Devices.FingerReader;
using Edv__Id_Tag_Access.Devices.NFC;
using Edv__Id_Tag_Access.ImgEngines;
using Edv__Id_Tag_Access.myLog;
using Edv__Id_Tag_Access.Pace;
using Microsoft.VisualBasic;
using Serilog;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace Edv__Id_Tag_Access
{
    public class EdvLibAPi
    {
        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int Edv_Prueba_Connect(int appIcao);


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Edv_License__Add_License(byte[] path_license, byte[] path_public_key);

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr PACE_CreateSecret(byte[] secret, int len);

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Edv_Set_Template(byte[] template, int template_len);


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Register_Log_callback(Log_Callback cb, int printLogCtrlInfo);


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Edv_Licencia_Get_Client_Info(byte[] publickey_path, byte[] clientInfo, ref int infoLen);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void Log_Callback(
            IntPtr infoBuffer,
            int infoBufferLen,
            int logLevel
        );


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Edv_Init();

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Edv_Get_DgData(int timeout_ms);

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Edv__Set_Tag_Info(byte[] docNum, int docnum_len, byte[] DoB, int dob_len, byte[] DoE, int doe_len);

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Edv_Moc();


        private const string cREADER_NAME = "HID Global OMNIKEY 5022 Smart Card Reader 0";

        // Documentaciòn dice "Ocupar referencias, no llamadas directas"
        // porque el GC podría recolectar el delegado y dejar un puntero colgando en el lado nativo, lo que causaría un crash al intentar llamar al callback.
        private static Log_Callback _Log_Callback_Ref = Log_Fnc;


        private NFC_Reader?         glb_Nfc_Reader = null;
        private FingerPrintReader?  glb_FingerPrint_Reader = null;
        private Cedula_IO?          glb_Cedula_IO = null;


        public delegate void api_Tag_Type(string tag_type);

        private api_Tag_Type? glb_api_Tag_Type = null;


        public int Prueba_Connect_appIcao(int appIcao)
        {
            int status = 0;

            try
            {
                bool tag_on_field = false;
                glb_Nfc_Reader?.Detect_Card(ref tag_on_field);

                status = Edv_Prueba_Connect(appIcao);

            }
            catch (Exception ex)
            {
                status = 100;
            }

            return status;
        }   


        public static int Get_Client_Info(string public_key_path, ref string license)
        {
            int     status = 0;
            byte[]  lic = new byte[1024 * 5];
            int     lic_len = lic.Length;

            try
            {
                while (true)
                {
                    byte[] file_path = Encoding.UTF8.GetBytes(public_key_path);

                    status = Edv_Licencia_Get_Client_Info(file_path, lic, ref lic_len);

                    if (status == 0)
                    {
                        license = Encoding.UTF8.GetString(lic, 0, lic_len);
                    }
                    break;
                }
            }
            catch (Exception ex)
            {
                status = 1000;            
            }

            return status;
        }


        public int Init(api_Tag_Type tag_type, string public_key_path)
        {
            int status = 0;

            while (true)
            {
                LoggerConfig.Init();

                Register_Log_callback(_Log_Callback_Ref, 0);

                Log.Information("DLL Init - Start");

                string path_license = "licenciaUser.bin";

                Edv_License__Add_License(Encoding.UTF8.GetBytes(path_license + "\0"), Encoding.UTF8.GetBytes(public_key_path + "\0"));

                status = Edv_Init();
                if (status != 0)
                {
                    Log.Error("Edv_Init() = " + status.ToString());

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

                    glb_Cedula_IO.SetIO(glb_Nfc_Reader.Detect_Card, glb_Nfc_Reader.Disconnect_Card, glb_Nfc_Reader.IO, (x) => glb_api_Tag_Type(x));
                }

                break;
            }

            Log.Information("DLL Init - Done : " + status.ToString());

            return status;
        }

        public bool Tag__Insert_QR(string qr_link)
        {
            bool status = false;
            try
            {
                status = Cedula_Info.Set_Info__Qr(qr_link);
                if (status)
                {
                    Edv__Set_Tag_Info(Cedula_Info.baNumeroDocumento,
                                    Cedula_Info.baNumeroDocumento.Length,
                                    Cedula_Info.baFechaNacimiento,
                                    Cedula_Info.baFechaNacimiento.Length,
                                    Cedula_Info.baFechaExpiracion,
                                    Cedula_Info.baFechaExpiracion.Length);
                }



            }
            catch (Exception ex)
            {
                Log.Error("Error en Insert_Tag_QR: " + ex.Message);
            }
            
            return status;
        }

        public string Tag__Get_Numero_Documento()
        {
            return Cedula_Info.strNumeroDocumento;
        }

        public string Tag__Get_Fecha_Nacimiento()
        {
            return Cedula_Info.strFechaNacimiento_HumanFormat;
        }

        public string Tag__Get_Fecha_Expiracion()
        {
            return Cedula_Info.strFechaExpiracion_HumanFormat;
        }

        public int Tag__Get_Dg_Data(int timeout_ms)
        {
            return Edv_Get_DgData(timeout_ms);
        }

        public void Test_Lector()
        { 
            if (glb_Cedula_IO is not null)
            {
                try {

                    //bool tag_on_field = false;

                    //glb_Nfc_Reader?.Detect_Card(ref tag_on_field);

                    Edv_Moc();
                }
                catch (Exception ex)
                {
                    Log.Error("Error en Test_Lector: " + ex.Message);
                }
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

                try
                {
                    Edv_Set_Template(iso_compact, iso_compact.Length);
                } catch(Exception e) { 
                
                    int x = 0;
                };
                

                //Log.Logger.HexDump(iso_compact,data_lenght : 16);

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

        enum LogLevelDll
        {
            LOG_LEVEL_EMERGENCY = 0,
            LOG_LEVEL_ALERT = 1,
            LOG_LEVEL_CRITICAL = 2,
            LOG_LEVEL_ERROR = 3,
            LOG_LEVEL_WARNING = 4,
            LOG_LEVEL_NOTICE = 5,
            LOG_LEVEL_INFORMATIONAL = 6,
            LOG_LEVEL_DEBUG = 7
        };

        private static void Log_Fnc(IntPtr buffer, int len, int logLevel)
        {
            try
            {
                byte[] data = new byte[len];
                Marshal.Copy(buffer, data, 0, len);

                string msg = Encoding.ASCII.GetString(data);

                switch (logLevel)
                {
                    case (int)LogLevelDll.LOG_LEVEL_EMERGENCY:
                    case (int)LogLevelDll.LOG_LEVEL_ALERT:
                    case (int)LogLevelDll.LOG_LEVEL_CRITICAL:
                        Log.Fatal(msg);
                        break;

                    case (int)LogLevelDll.LOG_LEVEL_ERROR:
                        Log.Error(msg);
                        break;
                    case (int)LogLevelDll.LOG_LEVEL_WARNING:
                        Log.Warning(msg);
                        break;

                    case (int)LogLevelDll.LOG_LEVEL_NOTICE:
                    case (int)LogLevelDll.LOG_LEVEL_INFORMATIONAL:
                        Log.Information(msg);
                        break;

                    default:
                        Log.Debug(msg);
                        break;
                }
            }
            catch (Exception ex)
            {
                // ⚠️ NUNCA dejes que esto escape
                Console.WriteLine("ERROR en callback: " + ex.Message);
            }
        }
    }
}
