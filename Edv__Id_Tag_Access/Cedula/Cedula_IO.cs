using Edv__Id_Tag_Access.myLog;
using Edv__Id_Tag_Access.Pace;
using Edv__Id_Tag_Access.Secure_Utils;
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

        private byte[] APDU_HEADER___GENERAL_AUTHENTICATE = { 0x10, 0x86, 0x00, 0x00 };
        private byte[] APDU_PAYLOAD__GENERAL_AUTHENTICATE = { 0x7C, 0x00 };

        /// <summary>
        /// ///////////////////////////////////////////////////////////////////////////////////////////////////////
        /// </summary>

        private ushort APDU_ANS_OK = 0x9000;

        private IntPtr glb_Pcd_ctx = IntPtr.Zero;
        private IntPtr glb_Secure = IntPtr.Zero;


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

                    byte[] aTag = new byte[1];
                    byte[] aDataTmp1 = new byte[255];
                    byte[] aApduData = new byte[255];
                    byte[] aApduHeader = new byte[4];

                    // Prepare Security Enviroment Data
                    Array.Fill<byte>(aApduHeader, 0x00);
                    Array.Fill<byte>(aApduData, 0x00);

                    aApduHeader[0] = 0x00;
                    aApduHeader[1] = 0x22;
                    aApduHeader[2] = 0xC1;
                    aApduHeader[3] = 0xA4;

                    aDataTmp1[0] = 0x04;    // OID
                    aDataTmp1[1] = 0x00;
                    aDataTmp1[2] = 0x7F;
                    aDataTmp1[3] = 0x00;
                    aDataTmp1[4] = 0x07;
                    aDataTmp1[5] = 0x02;
                    aDataTmp1[6] = 0x02;
                    aDataTmp1[7] = 0x04;
                    aDataTmp1[8] = 0x02;
                    aDataTmp1[9] = 0x04;

                    aTag[0] = 0x80;

                    int iApduDataLen = 0;


                    int wrapLen1 = SecUtils.TLV(aTag, 1, aDataTmp1, 10, aApduData, 0, aApduData.Length);
                    if (wrapLen1 > 0)
                    {
                        aDataTmp1[0] = 0x01;    // Type MRZ
                        aTag[0] = 0x83;
                        int wrapLen2 = SecUtils.TLV(aTag, 1, aDataTmp1, 1, aApduData, wrapLen1, aApduData.Length - wrapLen1);
                        if (wrapLen2 > 0)
                        {
                            iApduDataLen = wrapLen2 + wrapLen1;
                        }
                        else
                        {
                            //    pSBIO->lError = SOLEMICAO_ERROR_CONNETC_WRAP;
                            //    PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x83");
                            //    goto JMP_connectcard;

                            iApduDataLen = -1;
                        }
                    }
                    else
                    {
                        //pSBIO->lError = SOLEMICAO_ERROR_CONNETC_WRAP;
                        //PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x80");
                        //goto JMP_connectcard;

                        iApduDataLen = -2;
                    }

                    byte[] payload_sm = new byte[iApduDataLen];
                    Array.Copy( aDataTmp1, 0, payload_sm, 0, iApduDataLen);    



                    Apdu_Plain_Build(aApduHeader, payload: payload_sm, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        Log.Error("No se pudo ejecutar solicitud de canal seguro -> " + $"0x{glb_Sw1Sw2:X4}");
                    }

                    Pace_Do.OpenPACE_Init();

                    glb_Secure =    Pace_Do.PACE_CreateSecret(Pace_Do.Get_SecretFormat(Cedula_Info.baNumeroDocumento, Cedula_Info.baFechaNacimiento, Cedula_Info.baFechaExpiracion), 91);
        
                    if (glb_Secure == IntPtr.Zero)
                    {
                        status = 20;
                        break;
                    }

                    if (glb_Pcd_ctx != IntPtr.Zero)
                    {
                        Pace_Do.EAC_Free(glb_Pcd_ctx);
                        break;
                    }

                    glb_Pcd_ctx = Pace_Do.EAC_Create();
                    if (glb_Pcd_ctx == IntPtr.Zero)
                    {
                        status = 21;
                        break;
                    }
                    
                    if(Pace_Do.EAC_InitCardAccess(glb_Pcd_ctx, glb_CardAccess, (UIntPtr)(glb_CardAccess.Length)) == 0)
                    {
                        status = 22;
                        break;
                    }


                    Apdu_Plain_Build(APDU_HEADER___GENERAL_AUTHENTICATE, payload: APDU_PAYLOAD__GENERAL_AUTHENTICATE, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        Log.Error("Error en General Authentication -> " + $"0x{glb_Sw1Sw2:X4}");

                        status = 23;
                        break;
                    }


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
