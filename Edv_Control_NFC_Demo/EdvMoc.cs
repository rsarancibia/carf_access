using System.Runtime.InteropServices;

namespace Edv_Control_NFC_Demo
{
    public partial class MocUI : UserControl
    {
        //[DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        //public static extern int Edv_Init();



        public MocUI()
        {
            InitializeComponent();
        }

        public int Init()
        {
            int status = 0;

            while(true)
            {
                //if (Edv_Init() != 0)
                //{
                //    status = 1;
                //    break;
                //}
                
                
                break;
            }
                



            return status;

        }

    }
}
