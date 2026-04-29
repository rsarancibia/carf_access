using Edv__Id_Tag_Access.myLog;
using Edv__Id_Tag_Access.Pace;
using Edv__Id_Tag_Access.Secure_Utils;
using PCSC.Iso7816;
using Serilog;
using System.Runtime.InteropServices;

namespace Edv__Id_Tag_Access.Cedula
{
    internal class Cedula_IO
    {

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Test_Reader();

        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Register_TxRx_Callback(TxRxCallback cb);


        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int TxRxCallback(
            IntPtr tx,
            int txLen,
            IntPtr rx,
            ref int rxLen
        );
        /// <summary>
        /// ///////////////////////////////////////////
        /// </summary>
        /// <param name="cb"></param>


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Register_TxRxNfc_Callback(TRANSMIT cb);


        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void TRANSMIT(
            IntPtr dummy,
            IntPtr tx,
            int txLen,
            IntPtr rx,
            ref int rxLen
        );


        /// <summary>
        /// ///////////////////////////////////////////////////////////////
        /// </summary>


        private const string cTIPO_CEDULA_ERROR = "IO Error";
        private const string cTIPO_CEDULA_NEW = "Nueva";
        private const string cTIPO_CEDULA_OLD = "Antigua";


        private static TxRxCallback? _callbackRef;


        private byte[] APDU_HEADER__SELECT_APP = { 0x00, 0xA4, 0x04, 0x0C };

        private byte[] APDU_PAYLOAD_SELECT_INSTANCE__NEW_APPICAO_ON = { 0xA0, 0x00, 0x00, 0x00, 0x77, 0x03, 0x08, 0x70, 0x07, 0x01, 0xFE, 0x00, 0x00, 0x04, 0x00, 0x01 };

        private byte[] APDU_PAYLOAD_SELECT_INSTANCE__OLD_APPICAO_ON = { 0xA0, 0x00, 0x00, 0x02, 0x47, 0x10, 0x01 };

        private byte[] APDU_PAYLOAD_SELECT_INSTANCE__NEW_APPICAO_OFF = { 0xA0, 0x00, 0x00, 0x00, 0x77, 0x03, 0x0C, 0x60, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x05, 0x00 };

        private byte[] APDU_PAYLOAD_SELECT_INSTANCE__OLD_APPICAO_OFF = { 0xE8, 0x28, 0xBD, 0x08, 0x0F, 0xD2, 0x50, 0x43, 0x68, 0x6C, 0x43, 0x43, 0x2D, 0x65, 0x49, 0x44 };

        private byte[] APDU_HEADER__SELECT_MASTER_FILE_OLD = { 0x00, 0xA4, 0x00, 0x0C };
        private byte[] APDU_PAYLOAD_SELECT_MASTER_FILE_OLD = { 0x3F, 0x00 };

        private byte[] APDU_HEADER__SELECT_DIR = { 0x00, 0xA4, 0x02, 0x0C };
        private byte[] APDU_PAYLOAD_SELECT_DIR = { 0x2F, 0x00 };

        private byte[] APDU_HEADER__READ_BINARY_ON_DIR = { 0x00, 0xB0, 0x00, 0x00 };

        private byte[] APDU_HEADER__SELECT_CARD_ACCESS = { 0x00, 0xA4, 0x02, 0x0C };
        private byte[] APDU_PAYLOAD_SELECT_CARD_ACCESS = { 0x01, 0x1C };

        private byte[] APDU_HEADER___GENERAL_AUTHENTICATE = { 0x10, 0x86, 0x00, 0x00 };
        private byte[] APDU_PAYLOAD__GENERAL_AUTHENTICATE = { 0x7C, 0x00 };


        private byte[] APDU_HEADER___GENERAL_AUTHENTICATE_PH4 = { 0x00, 0x86, 0x00, 0x00 };



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

        bool glb_Old_Tag = false;

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


        private IODelegate? glb_IO_Delegate = null;
        private Detect_Card? glb_Detect_Card_Delegate = null;
        private Tag_Type? glb_Tag_Type = null;


        public void SetIO(Detect_Card detectCardFunc, IODelegate ioFunc, Tag_Type tag_type)
        {
            glb_Detect_Card_Delegate = detectCardFunc;
            glb_IO_Delegate = ioFunc;
            glb_Tag_Type = tag_type;

            _callbackRef = MyTxRx;
            Register_TxRx_Callback(_callbackRef);

            Register_TxRxNfc_Callback(TransmitRecieve);

            Pace_Do.OpenPACE_Init();

        }

        private void TransmitRecieve(IntPtr dummy,
                                        IntPtr tx,
                                        int txLen,
                                        IntPtr rx,
                                        ref int rxLen)
        {
            if (glb_IO_Delegate is null) return;

            byte[] managedTx = new byte[txLen];
            Marshal.Copy(tx, managedTx, 0, txLen);

            byte[] received = new byte[256];
            int total_received = 0;
            ushort sw1asw2 = 0;

            glb_IO_Delegate(managedTx, txLen, received, ref total_received, ref sw1asw2);

            // 🔹 Copiar respuesta hacia rx (memoria no administrada)
            if (total_received > 0)
            {
                Marshal.Copy(received, 0, rx, total_received);
                rxLen = total_received;
            }
            else
            {
                rxLen = 0;
            }
        }

        private int MyTxRx(IntPtr tx, int txLen, IntPtr rx, ref int rxLen)
        {
            // Copiar TX desde puntero nativo → array .NET
            byte[] managedTx = new byte[txLen];
            Marshal.Copy(tx, managedTx, 0, txLen);

            Log.Logger.HexDump(managedTx, data_lenght: managedTx.Length, message: "APDU a enviar");

            //// Simular respuesta
            //byte[] response = new byte[] { 0x90, 0x00 };

            //// Copiar respuesta hacia buffer nativo
            //Marshal.Copy(response, 0, rx, response.Length); 
            //rxLen = response.Length;


            glb_Detect_Card_Delegate();

            if (glb_IO_Delegate is not null)
            {
                Log.Information("PASO 2");

                glb_IO_Delegate(managedTx, managedTx.Length, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2); 

                if (glb_Sw1Sw2 == APDU_ANS_OK)
                {
                    //Set_Tag_Type__New();
                }
            }


            return 1;
        }


        private int MyTxRx_old(byte[] tx, int txLen, byte[] rx, ref int rxLen) 
        {
            Log.Information("PASO 1A -> " + txLen.ToString());
            Log.Information("PASO 1B -> " + tx[0].ToString());
            Log.Information("PASO 1C -> " + tx[1].ToString());


            //Log.Logger.HexDump(tx, data_lenght: txLen, message: "APDU a enviar");

            return 0;


            glb_Detect_Card_Delegate();

            if (glb_IO_Delegate is not null)
            {
                Log.Information("PASO 2");

                glb_IO_Delegate(tx, txLen, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);  

                if (glb_Sw1Sw2 == APDU_ANS_OK)
                {
                    //Set_Tag_Type__New();
                }
            }

            return -1;


            //Console.WriteLine("TX:");
            //for (int i = 0; i < txLen; i++)
            //    Console.Write($"{tx[i]:X2} ");
            //Console.WriteLine();

            //// Simulación respuesta (ejemplo)
            //byte[] response = new byte[] { 0x90, 0x00 };

            //if (rx.Length < response.Length)
            //    return -1;

            //Array.Copy(response, rx, response.Length);
            //rxLen = response.Length;

            //return 1;
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
                    Log.Information("APP ICAO Mode");
                }
                else
                {
                    Log.Information("APP Normal (no ICAO Mode)");
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

                    aTag[0]     = 0x80;

                    int iApduDataLen = 0, len1, len2;

                    len1 = SecUtils.TLV_Encode(aTag, 1, aDataTmp1, 0, 10, aApduData, 0);
                    if (len1 <= 0)
                    {
                        status = 20;
                        break;
                    }

                    aDataTmp1[0]    = 0x01;    // Type MRZ
                    aTag[0]         = 0x83;

                    len2 = SecUtils.TLV_Encode(aTag, 1, aDataTmp1, 0, 1, aApduData, len1);

                    if (len2 <= 0)
                    {
                        status = 21;
                        break;
                    }

                    iApduDataLen = len1+ len2;

                    Log.Logger.HexDump(aApduData, data_lenght: iApduDataLen, message: "Datos a enviar en GENERAL AUTHENTICATE");


                    Apdu_Plain_Build(aApduHeader, payload: aApduData, Lc: (byte)iApduDataLen, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        Log.Error("No se pudo ejecutar solicitud de canal seguro -> " + $"0x{glb_Sw1Sw2:X4}");
                    }

                    

                    //glb_Secure =    Pace_Do.PACE_CreateSecret(Pace_Do.Get_SecretFormat(Cedula_Info.baNumeroDocumento, Cedula_Info.baFechaNacimiento, Cedula_Info.baFechaExpiracion), 90);
        
                    //if (glb_Secure == IntPtr.Zero)
                    //{
                    //    status = 22;
                    //    break;
                    //}

                    if (glb_Pcd_ctx != IntPtr.Zero)
                    {
                        Pace_Do.EAC_Free(glb_Pcd_ctx);
                        break;
                    }

                    glb_Pcd_ctx = Pace_Do.EAC_Create();
                    if (glb_Pcd_ctx == IntPtr.Zero)
                    {
                        status = 23;
                        break;
                    }
                    
                    if(Pace_Do.EAC_InitCardAccess(glb_Pcd_ctx, glb_CardAccess, (UIntPtr)(glb_CardAccess.Length)) == 0)
                    {
                        status = 24;
                        break;
                    }


                    glb_Secure = Pace_Do.PACE_CreateSecret(Pace_Do.Get_SecretFormat(Cedula_Info.baNumeroDocumento, Cedula_Info.baFechaNacimiento, Cedula_Info.baFechaExpiracion), 90);

                    if (glb_Secure == IntPtr.Zero)
                    {
                        status = 22;
                        break;
                    }

                    //
                    // General Authenticate para obtener el mapping
                    //

                    Log.Information("General Authenticate");

                    Apdu_Plain_Build(APDU_HEADER___GENERAL_AUTHENTICATE, payload: APDU_PAYLOAD__GENERAL_AUTHENTICATE, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        Log.Error("Error en General Authentication -> " + $"0x{glb_Sw1Sw2:X4}");

                        status = 25;
                        break;
                    }


                    int res = 0;


                    byte [] data = new byte[32];

                    Array.Copy(glb_RcvData,4,data,0,32);    

                    res = Pace_Do.PACE_Step2(glb_Pcd_ctx,
                                            glb_Secure,
                                            data,
                                            data.Length);




                    IntPtr ptr;
                    int len;

                    res = Pace_Do.PACE_Step3A_Alloc(glb_Pcd_ctx, out ptr, out len);

                    byte[] mapping = new byte[len];
                    Marshal.Copy(ptr, mapping, 0, len);

                    Pace_Do.OpenPACE_Free(ptr);


                    Log.Logger.HexDump(mapping, data_lenght: len, message: "Mapping obtenido en Step3A");

                    //
                    // General authenticate con el mapping obtenido
                    //
                    aDataTmp1[0] = 0x7C;
                    aDataTmp1[1] = (byte)(len + 2);
                    aDataTmp1[2] = 0x81;
                    aDataTmp1[3] = (byte)len;
                    Array.Copy(mapping, 0, aDataTmp1, 4, len);


                    Apdu_Plain_Build(APDU_HEADER___GENERAL_AUTHENTICATE, Lc: (byte)(len + 4), payload: aDataTmp1, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        Log.Error("Error en General Authentication con mapping -> " + $"0x{glb_Sw1Sw2:X4}");

                        status = 26;
                        break;
                    }

                    Log.Logger.HexDump(glb_RcvData, data_lenght: glb_RcvData_Total_Bytes, message: "Mapping obtenido con APDU usando mapping");

                    Array.Copy(glb_RcvData, 4, aDataTmp1, 0, glb_RcvData_Total_Bytes - 4);
                    
                    res = Pace_Do.PACE_Step_3A_Map_Generator(
                        glb_Pcd_ctx,
                        aDataTmp1,
                        glb_RcvData_Total_Bytes - 4);

                    if (res != 1)
                    {
                        status = 27;
                        break;
                    }

                    res = Pace_Do.PACE_STEP_3B_Generate_Ephemeral(  glb_Pcd_ctx,
                                                                    out ptr,
                                                                    out len);

                    if (res != 1)
                    {
                        status = 28;
                        break;
                    }

                    byte[] data2 = new byte[len];
                    Marshal.Copy(ptr, data2, 0, len);
                    Pace_Do.OpenPACE_Free(ptr);

                    Log.Logger.HexDump(data2, data_lenght: len, message: "Respuesta Ephemeral");

                    //
                    // General Authenticate Step 3
                    //

                    aTag[0] = 0x83;
                    len1 = SecUtils.TLV_Encode(aTag, 1, data2, 0, len, aDataTmp1, 0);
                    if (len1 <= 0)
                    {
                        status = 29;
                        break;
                    }

                    aTag[0] = 0x7C;
                    len1 = SecUtils.TLV_Encode(aTag, 1, aDataTmp1, 0, len1, aApduData, 0);
                    if (len1 <= 0)
                    {
                        status = 30;
                        break;
                    }


                    Log.Logger.HexDump(aApduData, data_lenght: len1, message: "Test TLV");

                    //
                    // General Authenticate con el resultado del paso 3
                    //
                    Apdu_Plain_Build(APDU_HEADER___GENERAL_AUTHENTICATE, Lc: (byte)len1, payload: aApduData, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        Log.Error("Error en General Authentication step 3 -> " + $"0x{glb_Sw1Sw2:X4}");

                        status = 31;
                        break;
                    }

                    Log.Logger.HexDump(glb_RcvData, data_lenght: glb_RcvData_Total_Bytes, message: "General Authentication step 3");

                    // TODO : Viene en modo TLV..hay que parsear
                    Array.Copy(glb_RcvData, 4, aDataTmp1, 0, glb_RcvData_Total_Bytes - 4);

                    /*

                    [22:32:39 INF] General Authentication step 3
00000000  7C 63 84 61 04 38 06 C0 F5 7F BE DD 20 36 C7 AA  |c.a.8...... 6..
00000010  5A E5 3C 3E 94 A7 E4 76 46 1D 80 BB 43 C6 17 4F  Z.<>...vF...C..O
00000020  96 A7 C4 67 58 4E D0 0A E2 D5 F5 C5 E0 C2 79 53  ...gXN........yS
00000030  31 0C EC 9A 4A 9D B1 D7 97 48 AA 7B BE C6 82 73  1...J....H.{...s
00000040  64 D0 B5 E2 B3 56 BD E7 1E 8E A9 8F A1 A3 C2 76  d....V.........v
00000050  FE D5 FB 7E 56 E1 3A 4C 7A 4C 01 0A 86 5D 87 74  ...~V.:LzL...].t
00000060  54 E9 A8 07 B4  T....


                    */

                    

                    res = Pace_Do.PACE_Step_3B_Compute_Shared_Secret(   glb_Pcd_ctx,
                                                                        aDataTmp1,
                                                                        glb_RcvData_Total_Bytes - 4);

                    if (res != 1)
                    {
                        status = 32;
                        break;
                    }

                    res = Pace_Do.PACE_Step_3C_Derive_Keys(glb_Pcd_ctx);

                    if (res != 1)
                    {
                        status = 33;
                        break;
                    }

                    res = Pace_Do.PACE_Step_3D_Compute_Authentication_Token(
                        glb_Pcd_ctx,
                        aDataTmp1,
                        glb_RcvData_Total_Bytes - 4,
                        out ptr,
                        out len);

                    if (res != 1)
                    {
                        status = 34;
                        break;
                    }

                    byte[] token = new byte[len];
                    Marshal.Copy(ptr, token, 0, len);

                    Pace_Do.OpenPACE_Free(ptr);

                    //
                    // General Authenticate Fase 4
                    //

                    aTag[0] = 0x85;
                    len1 = SecUtils.TLV_Encode(aTag, 1, token, 0, len, aDataTmp1, 0);
                    if (len1 <= 0)
                    {
                        status = 35;
                        break;
                    }

                    aTag[0] = 0x7C;
                    len1 = SecUtils.TLV_Encode(aTag, 1, aDataTmp1, 0, len1, aApduData, 0);
                    if (len1 <= 0)
                    {
                        status = 36;
                        break;
                    }

                    Apdu_Plain_Build(APDU_HEADER___GENERAL_AUTHENTICATE_PH4, Lc: (byte)len1, payload: aApduData, Le: 0);
                    glb_IO_Delegate(glb_Send, glb_Send_Total_Bytes, glb_RcvData, ref glb_RcvData_Total_Bytes, ref glb_Sw1Sw2);
                    if (glb_Sw1Sw2 != APDU_ANS_OK)
                    {
                        Log.Error("Error en General Authenticate Fase 4 -> " + $"0x{glb_Sw1Sw2:X4}");

                        status = 37;
                        break;
                    }

                    Log.Logger.HexDump(glb_RcvData, data_lenght: glb_RcvData_Total_Bytes, message: "General Authentication PH4 3");

                    // TODO : Viene en modo TLV..hay que parsear
                    Array.Copy(glb_RcvData, 4, aDataTmp1, 0, glb_RcvData_Total_Bytes - 4);

                    res = Pace_Do.PACE_Step_3D_Verify(  glb_Pcd_ctx,
                                                        aDataTmp1,
                                                        glb_RcvData_Total_Bytes - 4);


                    if (res != 1)
                    {
                        status = 38;
                        break;
                    }


                    res = Pace_Do.EAC_SetEncryptionCtx(glb_Pcd_ctx, Pace_Do.EAC_ID_PACE);

                    if (res != 1)
                    {
                        status = 39;
                        break;
                    }

                    test_Cypher();


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


        private void Apdu_Plain_Build(byte[] header, byte? Lc = null, byte[]? payload = null, int? payload_offset = null, byte? Le = null)
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

        private void test_Cypher()
        {

            byte[] key = new byte[16]; // 128-bit
            byte[] iv = new byte[16];
            byte[] dataIn = new byte[16]; // múltiplo del bloque

            // Rellenar con datos de prueba
            for (int i = 0; i < key.Length; i++) key[i] = (byte)i;
            for (int i = 0; i < iv.Length; i++) iv[i] = (byte)(i + 1);
            for (int i = 0; i < dataIn.Length; i++) dataIn[i] = (byte)(i + 2);

            // Buffer de salida
            byte[] dataOut = new byte[64];
            int outLen = dataOut.Length;

            int result = SecUtils.getCrypt(
                SecUtils.NID_aes_128_cbc,
                SecUtils.CODER_CRYPT_ENCRYPT,
                key,
                key.Length,
                iv,
                iv.Length,
                dataIn,
                dataIn.Length,
                dataOut,
                ref outLen,
                0 // binario, no ASCII
            );

            Console.WriteLine($"Result: {result}");
            Console.WriteLine($"OutLen: {outLen}");


        }



    }
}
