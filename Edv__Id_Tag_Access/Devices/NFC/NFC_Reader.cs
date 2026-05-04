using Edv__Id_Tag_Access.myLog;
using PCSC;
using Serilog;

namespace Edv__Id_Tag_Access.Devices.NFC
{
    internal class NFC_Reader
    {
        SCardReader?    g_oReader = null;
        string?         g_ReaderName = null;

        byte[]          g_IO_RecvBuffer = new byte[256];
        int             g_IO_RecvBuffer_Total_Bytes = 0;

        public NFC_Reader()
        {
        }

        public int Init(string reader_name)
        {
            int status = 0;

            g_ReaderName = reader_name;

            try
            {
                var context = ContextFactory.Instance.Establish(SCardScope.System);

                g_oReader = new SCardReader(context);
            }
            catch (Exception ex)
            {
                status = 100;
            }

            return status;
        }

        public static int Show_Reader()
        {
            using (var context = ContextFactory.Instance.Establish(SCardScope.System))
            {
                var readers = context.GetReaders();

                if (readers == null || readers.Length == 0)
                {
                    Log.Error("No hay lectores");
                    return 1;
                }

                foreach (var reader in readers)
                {
                    Log.Information("Lector: " + reader);
                }

                return 0;
            }
        }


        public void End()
        {
            try
            {
                Disconnect_Card();
                g_oReader = null;
            }
            catch (Exception ex)
            {
            }
        }


        public int Detect_Card(ref bool tag_on_field)
        {
            int status      = 0;
            
            tag_on_field    = false;

            try
            {
                if (g_oReader is not null)
                {
                    var rc = g_oReader.Connect( g_ReaderName,
                                                SCardShareMode.Shared,
                                                SCardProtocol.Any);

                    if (rc != SCardError.Success)
                    {
                        tag_on_field = false;
                    }
                    else
                    {
                        tag_on_field = true;
                    }
                }
                else
                {
                    status = 1;
                }
            }
            catch (Exception ex)
            {
                status = 100;
            }

            return status;
        }


        public int Disconnect_Card()
        {
            if (g_oReader != null)
            {
                g_oReader.Disconnect(SCardReaderDisposition.Leave);
            }

            return 0;
        }




        public int IO(byte[] sendBuffer, int sendBufferLength, byte[] receiveData, ref int receiveDataLength, ref ushort sw1sw2)
        {
            int status = 0;

            while (true)
            {
                try
                {
                    if (g_oReader is null)
                    {
                        status = 1;
                        break;
                    }

                    Log.Logger.HexDump(sendBuffer, data_lenght: sendBufferLength, message: "APDU Tx Data : ");

                    g_IO_RecvBuffer_Total_Bytes = receiveData.Length; // Si no se indica esto, NO FUNCIONA !


                    SCardError rc = g_oReader.Transmit(sendBuffer, sendBufferLength, g_IO_RecvBuffer, ref g_IO_RecvBuffer_Total_Bytes);
                    if (rc != SCardError.Success)
                    {
                        Log.Error("Error transmitiendo: " + rc);
                        status = 2;
                        break;
                    }

                    if (g_IO_RecvBuffer_Total_Bytes < 2)
                    {
                        Log.Error("Error en respuesta : Ans Len < 2");
                        status = 3;
                        break;
                    }

                    Log.Logger.HexDump(g_IO_RecvBuffer, data_lenght: g_IO_RecvBuffer_Total_Bytes, message: "APDU Rx Data : ");

                    receiveDataLength = g_IO_RecvBuffer_Total_Bytes - 2;

                    if (receiveDataLength != 0)
                    {
                        Array.Copy(g_IO_RecvBuffer, 0, receiveData, 0, receiveDataLength);
                    }

                    sw1sw2 = (ushort)((g_IO_RecvBuffer[g_IO_RecvBuffer_Total_Bytes - 2] << 8) | g_IO_RecvBuffer[g_IO_RecvBuffer_Total_Bytes - 1]);

                    Log.Information(string.Format("Sw1Sw2 : 0x{0:X4}", sw1sw2));

                }
                catch (Exception ex)
                {
                    Log.Error(ex.ToString());
                    status = 4;
                }

                break;
            
            } // ~while (true)

            return status;
        }
    }
}
