using Edv__Id_Tag_Access.myLog;
using PCSC;
using Serilog;

namespace Edv__Id_Tag_Access.Devices.NFC
{
    internal class NFC_Reader
    {
        SCardReader? glb_Reader = null;
        string? glb_ReaderName = null;

        byte[] glb_IO_RecvBuffer = new byte[256];
        int glb_IO_RecvBuffer_Total_Bytes = 0;

        public NFC_Reader()
        {
        }

        public int Init(string reader_name)
        {
            glb_ReaderName = reader_name;
            return 0;
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
                if (glb_Reader != null)
                {
                    glb_Reader.Disconnect(SCardReaderDisposition.Leave);
                    glb_Reader = null;
                }
            }
            catch (Exception ex)
            {
                // Log.Error(ex.ToString());
            }
        }


        public int Detect_Card()
        {
            var context = ContextFactory.Instance.Establish(SCardScope.System);

            glb_Reader = new SCardReader(context);

            var rc = glb_Reader.Connect(glb_ReaderName,
                SCardShareMode.Shared,
                SCardProtocol.Any);

            if (rc != SCardError.Success)
            {
                Log.Error("Error conectando");
                return 1;
            }

            Log.Information("Tarjeta conectada!");
            return 0;
        }

        public int IO(byte[] sendBuffer, int sendBufferLength, byte[] receiveData, ref int receiveDataLength, ref ushort sw1sw2)
        {
            int status = 0;

            while (true)
            {
                try
                {
                    if (glb_Reader is null)
                    {
                        status = 1;
                        break;
                    }

                    Log.Logger.HexDump(sendBuffer, data_lenght: sendBufferLength, message: "APDU Tx Data : ");

                    glb_IO_RecvBuffer_Total_Bytes = receiveData.Length; // Si no se indica esto, NO FUNCIONA !


                    SCardError rc = glb_Reader.Transmit(sendBuffer, sendBufferLength, glb_IO_RecvBuffer, ref glb_IO_RecvBuffer_Total_Bytes);
                    if (rc != SCardError.Success)
                    {
                        Log.Error("Error transmitiendo: " + rc);
                        status = 2;
                        break;
                    }

                    if (glb_IO_RecvBuffer_Total_Bytes < 2)
                    {
                        Log.Error("Error en respuesta : Ans Len < 2");
                        status = 3;
                        break;
                    }

                    Log.Logger.HexDump(glb_IO_RecvBuffer, data_lenght: glb_IO_RecvBuffer_Total_Bytes, message: "APDU Rx Data : ");

                    receiveDataLength = glb_IO_RecvBuffer_Total_Bytes - 2;

                    if (receiveDataLength != 0)
                    {
                        Array.Copy(glb_IO_RecvBuffer, 0, receiveData, 0, receiveDataLength);
                    }

                    sw1sw2 = (ushort)((glb_IO_RecvBuffer[glb_IO_RecvBuffer_Total_Bytes - 2] << 8) | glb_IO_RecvBuffer[glb_IO_RecvBuffer_Total_Bytes - 1]);

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
