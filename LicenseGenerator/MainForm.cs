

namespace LicenseGenerator
{
//    HRTOneKiFBTbfwDK/6Uii+zLZAHNr0SJvlqkakl8wlC0qywMFEtvdtzHm1Ah53ql3IEu+YCwRzKVVDo3odArDUoo0N58dkYVN6IujofU1Ind9pnuznPewTj5z6JGJxbehdnr4Tzy7DL/1j9xz0qfbyLXPahNrktpYIorogwyqr6u2fF472SfgAvJ5RzFuGl4+13fLiwRtpoYCqyxQyvj1jIDDRxXTME6oes6F3KNPvgZEHpDMnnavrdnusV4Tjumn3OBqdjex+/MhyEZanxow3oh2i2dNQU70QO2e0dqudZjrLEqE5N1o1QetY2buzXppSE0/mpcgxyZQgOUpCCQ3Q==

//[23:51:43 INF][23:51:43(WRP) EMER] HWID : 178BFBFF00A70F80|0C115F12

//                       client_HwId_plain = "178BFBFF00A70F80|0C115F12"

    public partial class MainForm : Form
    {
        //string path = @"i:\RProteus\Cedula\Cedula_Access\Edv__Id_Tag_Access\Wrapper\wrapper\secure\private.pem";
        string path = @"private.pem";

        LicenseData         g_oLic = new();
        LicenseData.Data    g_oLicData = new();

        public MainForm()
        {
            InitializeComponent();
        }
        private void btn_Select_Public_Key_File_Click(object sender, EventArgs e)
        {
            string path = string.Empty;

            Show_Dialog_Pickup_File("Llave PÚBLICA", ref path);

            lblPath_Public_Key.Text = path;
        }

        private void bnLicense_Generate_Click(object sender, EventArgs e)
        {
            string title = "Build license";

            if (g_oLic.Build_License("licenciaUser.bin", g_oLicData) == 0)
            {
                MessageBox.Show("Build license OK", title);
            }
            else
            {
                MessageBox.Show("ERROR : No fue posible generar licencia", title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void btn_Select_Private_Key_File_Click(object sender, EventArgs e)
        {
            string path = string.Empty;

            Show_Dialog_Pickup_File("Llave PRIVADA", ref path);

            lblPath_Private_Key.Text = path;
        }

        private void bnLicense_Copy_Click(object sender, EventArgs e)
        {

        }


        private bool Show_Dialog_Pickup_File(string title, ref string path)
        {
            OpenFileDialog dlg = new OpenFileDialog();

            dlg.Title = "Seleccionar archivo - " + title;
            dlg.Filter = "Rsa Keys (*.pem)|*.pem| Rsa Keys (*.pub)|*.pub";

            if (dlg.ShowDialog() == DialogResult.OK)
            {
                //string path = dlg.FileName;

                //MessageBox.Show(path);

                path = dlg.FileName;

                return true;

            }

            return false;

        }

        private void btnProcessUserB64Info_Click(object sender, EventArgs e)
        {
            string user_hwid = "";
            int status;

            status = g_oLic.Init(path);
            if (status != 0)
            {
                lblUserHwId.Text = "Error starting Lic module : " + status.ToString();
                return;
            }

            status = g_oLic.Get_Info_From_Client_License(tbUserLicenseB64.Text, ref user_hwid);
            if (status != 0)
            {
                lblUserHwId.Text = "Error getting info : " + status.ToString();
                return;
            }

            lblUserHwId.Text = "User HwId : " + user_hwid;

            g_oLicData.HwId__Set(user_hwid);
        }
    }
}