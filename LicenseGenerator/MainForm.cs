
namespace LicenseGenerator
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }
        private void btn_Select_Public_Key_File_Click(object sender, EventArgs e)
        {
            string path = string.Empty;

            Show_Dialog_Picup_File("Llave PÚBLICA",ref path);

            lblPath_Public_Key.Text = path;
        }

        private void bnLicense_Generate_Click(object sender, EventArgs e)
        {
            LicenseData.Data oData = new();
            oData.User__Set("Hola");
        
        }

        private void btn_Select_Private_Key_File_Click(object sender, EventArgs e)
        {
            string path = string.Empty;

            Show_Dialog_Picup_File("Llave PRIVADA", ref path);

            lblPath_Private_Key.Text = path;
        }

        private void bnLicense_Copy_Click(object sender, EventArgs e)
        {

        }


        private bool Show_Dialog_Picup_File(string title, ref string path)
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


    }
}
