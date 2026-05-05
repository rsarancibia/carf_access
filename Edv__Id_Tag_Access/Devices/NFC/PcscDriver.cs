using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Edv__Id_Tag_Access.Devices.NFC
{
    internal class PcscDriver
    {
        const uint SCARD_SCOPE_SYSTEM = 2;
        const uint SCARD_SHARE_EXCLUSIVE = 1;
        const uint SCARD_PROTOCOL_T0 = 1;
        const uint SCARD_PROTOCOL_T1 = 2;
        const uint SCARD_STATE_UNAWARE = 0x0000;
        const uint SCARD_STATE_PRESENT = 0x0020;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        public struct SCARD_READERSTATE
        {
            public string szReader;
            public IntPtr pvUserData;
            public uint dwCurrentState;
            public uint dwEventState;
            public uint cbAtr;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 36)]
            public byte[] rgbAtr;
        }

        [DllImport("winscard.dll")]
        static extern int SCardEstablishContext(uint dwScope, IntPtr pvReserved1, IntPtr pvReserved2, out IntPtr phContext);

        [DllImport("winscard.dll", CharSet = CharSet.Auto)]
        static extern int SCardListReaders(IntPtr hContext, string mszGroups, byte[] mszReaders, ref int pcchReaders);

        [DllImport("winscard.dll", CharSet = CharSet.Auto)]
        static extern int SCardGetStatusChange(IntPtr hContext, uint dwTimeout, [In, Out] SCARD_READERSTATE[] rgReaderStates, int cReaders);

        [DllImport("winscard.dll")]
        static extern int SCardConnect(IntPtr hContext, string szReader, uint dwShareMode, uint dwPreferredProtocols,
            out IntPtr phCard, out uint pdwActiveProtocol);

        [DllImport("winscard.dll")]
        static extern int SCardDisconnect(IntPtr hCard, int dwDisposition);

        [DllImport("winscard.dll")]
        static extern int SCardTransmit(IntPtr hCard, ref SCARD_IO_REQUEST pioSendPci,
            byte[] pbSendBuffer, int cbSendLength,
            ref SCARD_IO_REQUEST pioRecvPci,
            byte[] pbRecvBuffer, ref int pcbRecvLength);

        [StructLayout(LayoutKind.Sequential)]
        public struct SCARD_IO_REQUEST
        {
            public uint dwProtocol;
            public uint cbPciLength;
        }

        private IntPtr _context;
        private IntPtr _card;
        private uint _protocol;
        private string _reader;

        public PcscDriver(string? readerName = null)
        {
            SCardEstablishContext(SCARD_SCOPE_SYSTEM, IntPtr.Zero, IntPtr.Zero, out _context);

            int pcchReaders = 0;
            SCardListReaders(_context, null, null, ref pcchReaders);

            byte[] mszReaders = new byte[pcchReaders];
            SCardListReaders(_context, null, mszReaders, ref pcchReaders);

            string allReaders = Encoding.ASCII.GetString(mszReaders);
            string[] readers = allReaders.Split('\0', StringSplitOptions.RemoveEmptyEntries);

            if (readers.Length == 0)
                throw new Exception("No hay lectores PC/SC disponibles");

            if (readerName == null)
            {
                _reader = readers[0]; // fallback
            }
            else
            {
                foreach (var r in readers)
                {
                    if (r.Contains(readerName))
                    {
                        _reader = r;
                        break;
                    }
                }

                if (_reader == null)
                    throw new Exception("Lector no encontrado: " + readerName);
            }
        }

        public int Connect()
        {
            SCARD_READERSTATE[] states = new SCARD_READERSTATE[1];
            states[0].szReader = _reader;
            states[0].dwCurrentState = SCARD_STATE_UNAWARE;

            while (true)
            {
                int res = SCardGetStatusChange(_context, 1000, states, 1);

                if (res == 0 && (states[0].dwEventState & SCARD_STATE_PRESENT) != 0)
                {
                    int conn = SCardConnect(_context, _reader, SCARD_SHARE_EXCLUSIVE,
                        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                        out _card, out _protocol);

                    if (conn == 0)
                        return 0;
                }

                states[0].dwCurrentState = states[0].dwEventState;
            }
        }


        public bool Connect(out int errorCode)
        {
            errorCode = 0;

            SCARD_READERSTATE[] states = new SCARD_READERSTATE[1];
            states[0].szReader = _reader;
            states[0].dwCurrentState = SCARD_STATE_UNAWARE;

            int res = SCardGetStatusChange(_context, 0, states, 1);

            if (res != 0)
            {
                errorCode = res;
                return false;
            }

            bool cardPresent = (states[0].dwEventState & SCARD_STATE_PRESENT) != 0;

            if (!cardPresent)
                return false;

            int conn = SCardConnect(
                _context,
                _reader,
                SCARD_SHARE_EXCLUSIVE,
                SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                out _card,
                out _protocol
            );

            if (conn != 0)
            {
                errorCode = conn;
                return false;
            }

            return true;
        }


        public void Disconnect()
        {
            if (_card != IntPtr.Zero)
            {
                SCardDisconnect(_card, 0);
                _card = IntPtr.Zero;
            }
        }

        public int Transmit(
            byte[] sendBuffer,
            int sendBufferLength,
            byte[] receiveData,
            ref int receiveDataLength,
            ref ushort sw1sw2)
        {
            if (_card == IntPtr.Zero)
                return 1; // No conectado

            try
            {
                SCARD_IO_REQUEST sendPci = new SCARD_IO_REQUEST
                {
                    dwProtocol = _protocol,
                    cbPciLength = (uint)Marshal.SizeOf(typeof(SCARD_IO_REQUEST))
                };

                SCARD_IO_REQUEST recvPci = sendPci;

                byte[] recvBuffer = new byte[512]; // más holgado
                int recvLength = recvBuffer.Length;

                int res = SCardTransmit(
                    _card,
                    ref sendPci,
                    sendBuffer,
                    sendBufferLength,
                    ref recvPci,
                    recvBuffer,
                    ref recvLength
                );

                if (res != 0)
                    return 2; // error transmit

                if (recvLength < 2)
                    return 3; // respuesta inválida

                // separar data y SW1SW2
                receiveDataLength = recvLength - 2;

                if (receiveDataLength > receiveData.Length)
                    return 4; // buffer insuficiente

                if (receiveDataLength > 0)
                {
                    Array.Copy(recvBuffer, 0, receiveData, 0, receiveDataLength);
                }

                sw1sw2 = (ushort)((recvBuffer[recvLength - 2] << 8) | recvBuffer[recvLength - 1]);

                return 0; // OK
            }
            catch
            {
                return 5; // excepción
            }
        }
    }

}
