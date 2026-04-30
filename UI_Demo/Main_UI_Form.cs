using Edv__Id_Tag_Access;
using SixLabors.ImageSharp.ColorSpaces;
using SixLabors.ImageSharp.PixelFormats;
using System.Runtime.InteropServices;
using System.Text;
//using Serilog;

namespace UI_Demo
{
    public partial class Main_UI_Form : Form
    {
        //[DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        //public static extern void Register_Log_callback(Log_Callback cb);


        //[UnmanagedFunctionPointer(CallingConvention.Cdecl)] 
        //public delegate void Log_Callback(
        //    IntPtr infoBuffer,
        //    int infoBufferLen
        //);

        //private static Log_Callback? _Log_Callback_Ref;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////

        private EdvLibAPi? glb_EdvLibAPi = null;

        private const string cTIPO_CEDULA = "Tipo de cédula : ";
        private const string cTIPO_CEDULA_UNK = "Desconocida";


        public Main_UI_Form()
        {
            InitializeComponent();
        }

        private void Log_Fnc(IntPtr buffer, int bufferLen)
        {
            // Copiar TX desde puntero nativo → array .NET
            byte[] managedTx = new byte[bufferLen];
            Marshal.Copy(buffer, managedTx, 0, bufferLen);

            string s = Encoding.UTF8.GetString(managedTx);

            richTB_Log.AppendText(s + "\n");

            //Log.Information(s);

        }



        private void Main_UI_Form_Load(object sender, EventArgs e)
        {
            LogViewer.Attach(richTB_Log);

            //_Log_Callback_Ref = Log_Fnc;
            //Register_Log_callback(_Log_Callback_Ref);


            lblTipo_Cedula.Text = cTIPO_CEDULA;
            lblTipo_Cedula_Val.Text = cTIPO_CEDULA_UNK;

            glb_EdvLibAPi = new(); 

            glb_EdvLibAPi.Init((x) =>
            {
                lblTipo_Cedula_Val.Text = x;
            });
        }

        private void button1_Click(object sender, EventArgs e)
        {
            //glb_EdvLibAPi?.Id_Tag__Read();

            int image_width = 0;
            int image_height = 0;
            int dpi = 0;
            byte[]? img_raw_buffer = null;

            glb_EdvLibAPi?.Finger__Get_Width_Height_Dpi(ref image_width, ref image_height, ref dpi);

            if (glb_EdvLibAPi?.Finger__Capture(ref img_raw_buffer) == 0)
            {
                Bitmap bmp = Helper.RawToBitmap(img_raw_buffer, image_width, image_height);
                pbDisplay.Image = bmp;

                //byte[] iso_image = Array.Empty<byte>();
                //glb_EdvLibAPi?.Finger__Get_Iso_19794_2(img_raw_buffer, (ushort)image_width, (ushort)image_height, (ushort)dpi, ref iso_image);

            }
        }

        private void Main_UI_Form_FormClosing(object sender, FormClosingEventArgs e)
        {
            glb_EdvLibAPi?.End();
            glb_EdvLibAPi = null;
        }

        private void btnDoMoc_Click(object sender, EventArgs e)
        {
            lblTipo_Cedula_Val.Text = cTIPO_CEDULA_UNK;
            glb_EdvLibAPi?.Id_Tag__MOC();
        }

        private void btnClearLog_Click(object sender, EventArgs e)  
        {
            richTB_Log.Clear();
        }

        private void button1_Click_1(object sender, EventArgs e)   
        {
            glb_EdvLibAPi?.Test_Lector();
        }
    }
}
