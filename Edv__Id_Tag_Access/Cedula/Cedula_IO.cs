using Edv__Id_Tag_Access.myLog;
using Edv__Id_Tag_Access.Pace;
using Serilog;
using Serilog.Core;

namespace Edv__Id_Tag_Access.Cedula
{
    internal class Cedula_IO
    {
        private const string cTIPO_CEDULA_ERROR = "IO Error";
        private const string cTIPO_CEDULA_NEW = "Nueva";
        private const string cTIPO_CEDULA_OLD = "Antigua";


        private byte[] APDU_HEADER__SELECT_APP = { 0x00, 0xA4, 0x04, 0x0C};

        private byte[] APDU_PAYLOAD_SELECT_INSTANCE__NEW_APPICAO_ON = { 0xA0, 0x00, 0x00, 0x00, 0x77, 0x03, 0x08, 0x70, 0x07, 0x01, 0xFE, 0x00, 0x00, 0x04, 0x00, 0x01};

        private byte[] APDU_PAYLOAD_SELECT_INSTANCE__OLD_APPICAO_ON = { 0xA0, 0x00, 0x00, 0x02, 0x47, 0x10, 0x01 };

        private byte[] APDU_PAYLOAD_SELECT_INSTANCE__NEW_APPICAO_OFF = { 0xA0, 0x00, 0x00, 0x00, 0x77, 0x03, 0x0C, 0x60, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x05, 0x00 };

        private byte[] APDU_PAYLOAD_SELECT_INSTANCE__OLD_APPICAO_OFF = { 0xE8, 0x28, 0xBD, 0x08, 0x0F, 0xD2, 0x50, 0x43, 0x68, 0x6C, 0x43, 0x43, 0x2D, 0x65, 0x49, 0x44 };

        private byte[] APDU_HEADER__SELECT_MASTER_FILE_OLD = { 0x00, 0xA4, 0x00, 0x0C };
        private byte[] APDU_PAYLOAD_SELECT_MASTER_FILE_OLD = { 0x3F, 0x00};

        private byte[] APDU_HEADER__SELECT_DIR = { 0x00, 0xA4, 0x02, 0x0C };
        private byte[] APDU_PAYLOAD_SELECT_DIR = { 0x2F, 0x00 };

        private byte[] APDU_HEADER__READ_BINARY_ON_DIR = { 0x00, 0xB0, 0x00, 0x00 };

        private byte[] APDU_HEADER__SELECT_CARD_ACCESS = { 0x00, 0xA4, 0x02, 0x0C };
        private byte[] APDU_PAYLOAD_SELECT_CARD_ACCESS = { 0x01, 0x1C };

        private ushort APDU_ANS_OK = 0x9000;

        byte[] glb_Send = new byte[255 + 6]; //+ 6 !!! ???
        int glb_Send_Total_Bytes = 0;

        byte[] glb_RcvData = new byte[255];
        int glb_RcvData_Total_Bytes = 0;

        ushort glb_Sw1Sw2 = 0;

        bool   glb_Old_Tag = false;

        byte[] glb_CardAccess = new byte[256];

        public delegate int IODelegate(
            byte[] sendBuffer,
            int sendBufferLength,
            byte[] receiveData,
            ref int receiveDataLength,
            ref ushort sw1sw2
        );

        public delegate int Detect_Card();

        public delegate void Tag_Type(string tag_type);


        private IODelegate?     glb_IO_Delegate = null;
        private Detect_Card?    glb_Detect_Card_Delegate = null;
        private Tag_Type?       glb_Tag_Type = null;


        public void SetIO(Detect_Card detectCardFunc, IODelegate ioFunc, Tag_Type tag_type)
        {
            glb_Detect_Card_Delegate    =   detectCardFunc;
            glb_IO_Delegate             =   ioFunc;
            glb_Tag_Type                =   tag_type;
        }


        public int MOC()
        { 
            int status = 0;


            while (true)
            {
                if (Connect(false) != 0)
                {
                    glb_Tag_Type?.Invoke(cTIPO_CEDULA_ERROR);

                    status = 1;
                    break;
                }

                break;
            }

            return status;
        
        }


        private int Connect(bool IcaoApp)
        {
            int status = 0;

            while (true)
            {
                if (glb_IO_Delegate == null)
                {
                    status = 1;
                    break;
                }

                if (glb_Detect_Card_Delegate == null)
                {
                    status = 2;
                    break;
                }

                if (glb_Detect_Card_Delegate() != 0)
                {
                    status = 3;
                    break;
                }

                if (IcaoApp)
                {
                    Apdu_Plain_Build(APDU_HEADER__SELECT_APP, payload: APDU_PAYLOAD_SELECT_INSTANCE__NEW_APPICAO_ON, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);

                    if (glb_Sw1Sw2 == APDU_ANS_OK)
                    {
                        Set_Tag_Type__New();
                    }
                    else
                    {
                        Apdu_Plain_Build(APDU_HEADER__SELECT_APP, payload: APDU_PAYLOAD_SELECT_INSTANCE__OLD_APPICAO_ON, Le: 0);
                        glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                        if (glb_Sw1Sw2 == APDU_ANS_OK)
                        {
                            Set_Tag_Type__Old();
                        }
                        else {
                            status = 4;
                            break;
                        }
                    }
                }
                else
                {
                    Apdu_Plain_Build(APDU_HEADER__SELECT_APP, payload: APDU_PAYLOAD_SELECT_INSTANCE__NEW_APPICAO_OFF, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 == APDU_ANS_OK)
                    {
                        Set_Tag_Type__New();
                    }
                    else
                    {
                        Apdu_Plain_Build(APDU_HEADER__SELECT_APP, payload: APDU_PAYLOAD_SELECT_INSTANCE__OLD_APPICAO_OFF, Le: 0);
                        glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                        if (glb_Sw1Sw2 == APDU_ANS_OK)
                        {
                            Set_Tag_Type__Old();
                        }
                        else
                        {
                            status = 5;
                            break;
                        }
                    }
                }

                if (glb_Old_Tag)
                {
                    Apdu_Plain_Build(APDU_HEADER__SELECT_MASTER_FILE_OLD, payload: APDU_PAYLOAD_SELECT_MASTER_FILE_OLD, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        status = 6;
                        break;
                    }
                }

                //
                // SELECT DIR
                //
                Apdu_Plain_Build(APDU_HEADER__SELECT_DIR, payload: APDU_PAYLOAD_SELECT_DIR, Le: 0);
                glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                if (glb_Sw1Sw2 != APDU_ANS_OK)
                {
                    Log.Warning("No se pudo seleccionar el directorio raíz -> " + $"0x{glb_Sw1Sw2:X4}");
                }
                else
                {
                    //
                    // READBINARY on DIR
                    //
                    Apdu_Plain_Build(APDU_HEADER__READ_BINARY_ON_DIR, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        Log.Warning("No se pudo leer el directorio raíz -> " + $"0x{glb_Sw1Sw2:X4}");
                    }
                }

                //
                // SELECT CARDACCESS
                //
                Apdu_Plain_Build(APDU_HEADER__SELECT_CARD_ACCESS, payload: APDU_PAYLOAD_SELECT_CARD_ACCESS, Le: 0);
                glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                if (glb_Sw1Sw2 != APDU_ANS_OK)
                {
                    Log.Warning("No se pudo ejecutar CARD ACCESS -> " + $"0x{glb_Sw1Sw2:X4}");
                }
                else
                {
                    //
                    // READBINARY on DIR
                    //
                    Apdu_Plain_Build(APDU_HEADER__READ_BINARY_ON_DIR, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        Log.Warning("No se pudo leer el directorio raíz -> " + $"0x{glb_Sw1Sw2:X4}");
                    }
                    else
                    {

                        Log.Logger.HexDump(glb_RcvData, data_lenght: glb_RcvData_Total_Bytes, message: "Contenido de CardAccess READ Binary");

                        Array.Copy(glb_RcvData, 0, glb_CardAccess, 0, glb_RcvData_Total_Bytes);
                    }

                }


                if (!glb_Old_Tag)
                { 
                    // = Tarjeta nueva -> PACE
                    Pace_Do.OpenPACE_Init();

                    IntPtr ptr =    Pace_Do.PACE_CreateSecret(Pace_Do.Get_SecretFormat(Cedula_Info.baNumeroDocumento, Cedula_Info.baFechaNacimiento, Cedula_Info.baFechaExpiracion), 91);


                    glb_Old_Tag = false;

                }

                break;

            }

            return status;
        }

        private void Set_Tag_Type__Old()
        {
            glb_Old_Tag = true;
            glb_Tag_Type?.Invoke(cTIPO_CEDULA_OLD);
        }

        private void Set_Tag_Type__New()
        {
            glb_Old_Tag = false;
            glb_Tag_Type?.Invoke(cTIPO_CEDULA_NEW);
        }


        private void Apdu_Plain_Build(byte[] header, byte? Lc = null, byte[]? payload = null, byte? Le = null)
        {
            // CLA | INS | P1 | P2 | [Lc] | [DATA] | [Le]

            if (header == null || header.Length != 4)
                throw new ArgumentException("Header debe tener 4 bytes");

            int offset = 0;

            // Copiar header
            Array.Copy(header, 0, glb_Send, 0, 4);
            offset = 4;

            byte len = 0;

            if (payload != null)
            {
                len = Lc ?? (byte)payload.Length;

                if (len > payload.Length)
                    throw new ArgumentOutOfRangeException(nameof(Lc), "Lc mayor que payload");

                glb_Send[offset++] = len;

                if (len > 0)
                {
                    Array.Copy(payload, 0, glb_Send, offset, len);
                    offset += len;
                }
            }

            // Le (opcional)
            if (Le.HasValue)
            {
                glb_Send[offset++] = Le.Value;
            }

            glb_Send_Total_Bytes = offset;
        }





    }
}
