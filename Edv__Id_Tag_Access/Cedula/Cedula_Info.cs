using System.Text;

namespace Edv__Id_Tag_Access.Cedula
{
    internal static class Cedula_Info
    {
        // Myself
        // https://portal.nuevosidiv.registrocivil.cl/document-validity?RUN=12845657-0&type=CEDULA&serial=B5F089055&mrz=B5F089055075040793504071


        // Catalina
        // https://portal.sidiv.registrocivil.cl/docstatus?RUN=26952934-2&type=CEDULA&serial=534557233&mrz=534557233119080662908063


        // Longitudes de los campos en la serialización de la cédula
        // Incluyen dígito verificador
        private const int cCEDULA__DOC_NUM_POS = 0;
        private const int cCEDULA__DOC_NUM_LEN = 10;

        private const int cCEDULA__FECHA_NACIMIENTO_POS = cCEDULA__DOC_NUM_POS + cCEDULA__DOC_NUM_LEN;
        private const int cCEDULA__FECHA_NACIMIENTO_LEN = 7;

        private const int cCEDULA__FECHA_EXPIRACION_POS = cCEDULA__FECHA_NACIMIENTO_POS + cCEDULA__FECHA_NACIMIENTO_LEN;
        private const int cCEDULA__FECHA_EXPIRACION_LEN = 7;

        private const int cCEDULA_SERIAL_LEN = cCEDULA__DOC_NUM_LEN + cCEDULA__FECHA_NACIMIENTO_LEN + cCEDULA__FECHA_EXPIRACION_LEN;

        private const string cCEDULA__TAG_ID = "mrz";


        public static string strFechaNacimiento_HumanFormat { get; private set; } = "";
        public static string strFechaExpiracion_HumanFormat { get; private set; } = "";

        public static string strNumeroDocumento { get; private set; } = "";
        public static string strFechaNacimiento { get; private set; } = "";
        public static string strFechaExpiracion { get; private set; } = "";
        public static string strSerial { get; private set; } = "";


        public static byte[] baNumeroDocumento { get; private set; } = new byte[cCEDULA__DOC_NUM_LEN];
        public static byte[] baFechaNacimiento    { get; private set; } = new byte[cCEDULA__FECHA_NACIMIENTO_LEN];
        public static byte[] baFechaExpiracion { get; private set; } = new byte[cCEDULA__FECHA_EXPIRACION_LEN];

        public static byte[] baSerial { get; private set; } = new byte[cCEDULA_SERIAL_LEN];

        public static bool Set_Info__Qr(string qr_link)
        { 
            string cedula_serial = "";

            while (true)
            {
                
                if(qr_link.Length  < (cCEDULA_SERIAL_LEN + cCEDULA__TAG_ID.Length))
                {
                    break;
                }   

                int posicion = qr_link.IndexOf(cCEDULA__TAG_ID);
                if (posicion < 0)
                {
                    break;
                }

                if((qr_link.Length - posicion) < (cCEDULA_SERIAL_LEN + cCEDULA__TAG_ID.Length))
                {
                    break;
                }

                cedula_serial = qr_link.Substring(posicion + cCEDULA__TAG_ID.Length + 1, cCEDULA_SERIAL_LEN);

                    break;
            }

            return Set_Info__Serial(cedula_serial);
        }

        public static bool Set_Info__Serial(string cedula_serial)
        { 
            bool status = false;

            while (true)
            {
                if (cedula_serial.Length != cCEDULA_SERIAL_LEN)
                {
                    break;
                }

                strSerial = cedula_serial;

                strNumeroDocumento  = cedula_serial.Substring(cCEDULA__DOC_NUM_POS, cCEDULA__DOC_NUM_LEN);
                strFechaNacimiento  = cedula_serial.Substring(cCEDULA__FECHA_NACIMIENTO_POS, cCEDULA__FECHA_NACIMIENTO_LEN);
                strFechaExpiracion  = cedula_serial.Substring(cCEDULA__FECHA_EXPIRACION_POS, cCEDULA__FECHA_EXPIRACION_LEN);

                Array.Copy(Encoding.ASCII.GetBytes(strNumeroDocumento), 0, baNumeroDocumento,   0, cCEDULA__DOC_NUM_LEN);
                Array.Copy(Encoding.ASCII.GetBytes(strFechaNacimiento), 0, baFechaNacimiento,   0, cCEDULA__FECHA_NACIMIENTO_LEN);
                Array.Copy(Encoding.ASCII.GetBytes(strFechaExpiracion), 0, baFechaExpiracion,   0, cCEDULA__FECHA_EXPIRACION_LEN);
                Array.Copy(Encoding.ASCII.GetBytes(strSerial),          0, baSerial,            0, cCEDULA_SERIAL_LEN);

                strFechaNacimiento_HumanFormat = Build_Date(strFechaNacimiento);
                strFechaExpiracion_HumanFormat = Build_Date(strFechaExpiracion);

                status = true;
                break;
            }

            return status;
        }

        private static string Build_Date(string input)
        {
            //string input = "260505X";
            string salida = "";

            if (input.Length >= 6)
            {
                string limpio = input.Substring(0, 6);

                if (DateTime.TryParseExact(
                    limpio,
                    "yyMMdd",
                    System.Globalization.CultureInfo.InvariantCulture,
                    System.Globalization.DateTimeStyles.None,
                    out DateTime fecha))
                {
                    salida = fecha.ToString("dd/MM/yyyy");
                    Console.WriteLine(salida);
                }
            }

            return salida;
        }


    }
}
