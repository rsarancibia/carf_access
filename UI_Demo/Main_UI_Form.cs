using Edv__Id_Tag_Access;
using ImageMagick;

namespace UI_Demo
{
    public partial class Main_UI_Form : Form
    {
        private string g_Public_Key_Path = @"i:\RProteus\Cedula\Cedula_Access\Edv__Id_Tag_Access\Wrapper\wrapper\secure\public.pem";

        private EdvLibAPi? glb_EdvLibAPi = null;

        private const string cTIPO_CEDULA = "Tipo de cédula : ";
        private const string cTIPO_CEDULA_UNK = "Desconocida";

        private const string cLBL_DOC_NUM = "Nro  documento  : ";
        private const string cLBL_FECH_NAC = "Fec. nacimiento : ";
        private const string cLBL_FECH_EXP = "Fec. expiración : ";


        private QrKeyboardInterceptor _qr;

        private LedAnimator? led;

        public Main_UI_Form()
        {
            InitializeComponent();

            _qr = new QrKeyboardInterceptor();
            _qr.OnQrRead += QrReceived;

            Application.AddMessageFilter(_qr);
        }

        private void QrReceived(string data)
        {
            // ⚠️ Esto ya viene limpio, sin interferir UI
            //MessageBox.Show("QR: " + data);

            // o lo que necesites:
            ProcessQR(data);
        }

        private void ProcessQR(string qrData)
        {
            // lógica de procesamiento del QR

            int pos = qrData.IndexOf("MRZ");
            bool status = false;

            if (pos > 0)
            {
                //
                // Procesar Qr
                //
                if (glb_EdvLibAPi is not null)
                {
                    if (glb_EdvLibAPi.Tag__Insert_QR(qrData.Replace("MRZ", "&mrz=")) == true)
                    {
                        led?.Flash();

                        lblQR_NroDoc.Text = cLBL_DOC_NUM + glb_EdvLibAPi.Tag__Get_Numero_Documento();
                        lblQR_FechNac.Text = cLBL_FECH_NAC + glb_EdvLibAPi.Tag__Get_Fecha_Nacimiento();
                        lblQR_FechExp.Text = cLBL_FECH_EXP + glb_EdvLibAPi.Tag__Get_Fecha_Expiracion();
                        status = true;
                    }
                }
            }


            if (status == false)
            {
                MessageBox.Show("ERROR : No fue posible procesar QR", "Error",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
            }
        }

        private void Main_UI_Form_Load(object sender, EventArgs e)
        {
            LogViewer.Attach(richTB_Log);

            lblTipo_Cedula.Text = cTIPO_CEDULA;
            lblTipo_Cedula_Val.Text = cTIPO_CEDULA_UNK;

            glb_EdvLibAPi = new();

            glb_EdvLibAPi.Init((x) =>
            {
                lblTipo_Cedula_Val.Text = x;
            }, g_Public_Key_Path);

            lblQR_NroDoc.Text = cLBL_DOC_NUM;
            lblQR_FechNac.Text = cLBL_FECH_NAC;
            lblQR_FechExp.Text = cLBL_FECH_EXP;

            led = new LedAnimator(btnLed);


            //MessageBox.Show(RuntimeInformation.ProcessArchitecture.ToString());

        }


        private void btnCapture_Finger_Live_Click(object sender, EventArgs e)
        {
            int image_width = 0;
            int image_height = 0;
            int dpi = 0;
            byte[]? img_raw_buffer = null;

            glb_EdvLibAPi?.Finger__Get_Width_Height_Dpi(ref image_width, ref image_height, ref dpi);

            if (glb_EdvLibAPi?.Finger__Capture(ref img_raw_buffer) == 0)
            {
                Bitmap bmp = Helper.RawToBitmap(img_raw_buffer, image_width, image_height);
                pbDisplay.Image = bmp;
            }
        }

        private void Main_UI_Form_FormClosing(object sender, FormClosingEventArgs e)
        {
            glb_EdvLibAPi?.End();
            glb_EdvLibAPi = null;
        }

        private async void btnDoMocF1_Click(object sender, EventArgs e)
        {
            lblTipo_Cedula_Val.Text = cTIPO_CEDULA_UNK;

            if (glb_EdvLibAPi is not null)
            {
                int status = await glb_EdvLibAPi.Tag__MOC();

            }
        }

        private async void btnDoMocF2_Click(object sender, EventArgs e)
        {
            lblTipo_Cedula_Val.Text = cTIPO_CEDULA_UNK;

            if (glb_EdvLibAPi is not null)
            {
                int status = await glb_EdvLibAPi.Tag__MOC();
            }
        }

        //byte[] jp2 = await api.Tag__Get_Face_Picture();


        private void btnClearLog_Click(object sender, EventArgs e)
        {
            richTB_Log.Clear();
        }

        //private void button1_Click_1(object sender, EventArgs e)
        //{
        //    glb_EdvLibAPi?.Tag__MOC();
        //}

        private void GetLicense_Click(object sender, EventArgs e)
        {
            string title = "Get license";
            string license = "";


            int status = EdvLibAPi.Get_Client_Info(g_Public_Key_Path, ref license);

            if (status == 0)
            {
                Clipboard.SetText(license);

                MessageBox.Show("Get license OK", title);
            }
            else
            {
                MessageBox.Show("ERROR : No fue posible generar licencia : " + status.ToString(), title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

        }

        private void btnCnnnect_AppIcao_0_Click(object sender, EventArgs e)
        {
            glb_EdvLibAPi?.Prueba_Connect_appIcao(0);
        }

        private void btnCnnnect_AppIcao_1_Click(object sender, EventArgs e)
        {
            glb_EdvLibAPi?.Prueba_Connect_appIcao(1);
        }

        private async void btnTag_Get_Dg_Click(object sender, EventArgs e)
        {
            if (glb_EdvLibAPi is null)
                return;

            EdvLibAPi.DgDataResult result = await glb_EdvLibAPi.Tag__Get_Dg_Data(4000);

            if (result.Status == 0)
            {
                lblDedo1.Text = EdvLibAPi.FingerDescription(result.Finger1);
                lblDedo2.Text = EdvLibAPi.FingerDescription(result.Finger2);
            }

            MagickNET.Initialize();

            byte[] jp2_face = await glb_EdvLibAPi.Tag__Get_Face_Picture();
            using var image_face = new MagickImage(jp2_face);
            pbUserPhoto.Image = image_face.ToBitmap();

            byte[] jp2_signature = await glb_EdvLibAPi.Tag__Get_Signature_Picture();
            using var image_signature = new MagickImage(jp2_signature);
            pbSignature.Image = image_signature.ToBitmap();
        }
    }
}
