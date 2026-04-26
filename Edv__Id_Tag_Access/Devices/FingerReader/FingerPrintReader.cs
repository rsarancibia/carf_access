using SecuGen.FDxSDKPro.Windows;
using Serilog;


namespace Edv__Id_Tag_Access.Devices.FingerReader
{
    internal class FingerPrintReader
    {
        private SGFingerPrintManager? glb_mFPM = null;
        private byte[]? glb_fpImage;

        private int glb_Img_Width;
        private int glb_Img_Height;
        private int glb_Img_dpi;

        public byte[]? Img_Raw_Buffer { get => glb_fpImage;}
        public int Img_Width { get => glb_Img_Width; set => glb_Img_Width = value; }
        public int Img_Height { get => glb_Img_Height; set => glb_Img_Height = value; }
        public int Img_dpi { get => glb_Img_dpi; set => glb_Img_dpi = value; }

        public int Init()
        {
            int status = 0;
            int port_addr;
            int iError = 0;

            Log.Information("FingerPrintReader - Init - Start");

            while (true)
            {
                try
                {
                    SGFPMDeviceName device_name = SGFPMDeviceName.DEV_FDU03;

                    glb_mFPM = new SGFingerPrintManager();

                    iError = glb_mFPM.Init(device_name);
                    if (iError != 0)
                    {
                        Log.Error("Error al inicializar el lector de huellas: " + iError);
                        status = 1;
                        break;
                    }

                    port_addr = (int)SGFPMPortAddr.USB_AUTO_DETECT;

                    iError = glb_mFPM.OpenDevice(port_addr);
                    if (iError != 0)
                    {
                        Log.Error("Error al abrir el lector de huellas: " + iError);
                        status = 2;
                        break;
                    }

                    SGFPMDeviceInfoParam pInfo = new SGFPMDeviceInfoParam();
                    pInfo = new SGFPMDeviceInfoParam();

                    iError = glb_mFPM.GetDeviceInfo(pInfo);
                    if (iError != 0)
                    {
                        Log.Error("Error al obtener la información del dispositivo: " + iError);
                        status = 2;
                        break;
                    }

                    glb_fpImage = new byte[pInfo.ImageWidth * pInfo.ImageHeight];

                    Img_Width = pInfo.ImageWidth;

                    Img_Height = pInfo.ImageHeight;

                    Img_dpi = pInfo.ImageDPI;

                }
                catch (Exception ex)
                {
                    Log.Error("Excepcion al inicializar el lector de huellas: " + ex.Message);
                    status = 100;
                    break;
                }

                break;
            }

            Log.Information("FingerPrintReader - Init - End : " + status.ToString());

            return status;
        }

        public int CaptureFingerPrint()
        {
            int status = 0;
            int iError = 0;

            Log.Information("FingerPrintReader - CaptureFingerPrint - Start");

            try
            {
                if (glb_mFPM is null)
                {
                    status = 1;
                }
                else
                {
                    iError = glb_mFPM.GetImage(glb_fpImage);

                    if (iError != 0)
                    {
                        Log.Error("Error al capturar la huella: " + iError);
                        status = 2;
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Error("Excepcion al capturar la huella: " + ex.Message);
                status = 100;
            }

            Log.Information("FingerPrintReader - Init - End : " + status.ToString());

            return status;
        }

        public int End()
        {
            int status = 0;
            try
            {
                if (glb_mFPM != null)
                {
                    glb_mFPM.CloseDevice();
                    glb_mFPM.Dispose();
                    glb_mFPM = null;
                }
            }
            catch (Exception ex)
            {
                Log.Error("Error al cerrar el lector de huellas: " + ex.Message);
                status = 100;
            }
            return status;
        }

    }
}
