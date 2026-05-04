using System.Runtime.InteropServices;

namespace Edv__Id_Tag_Access.Cedula
{
    internal class Cedula_IO
    {
        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Register_TxRxNfc_Callback(TRANSMIT cb);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int TRANSMIT(
            IntPtr dummy,
            IntPtr tx,
            int txLen,
            IntPtr rx,
            ref int rxLen
        );


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Register_Nfc_Tag_Detect_Callback(GETCARD cb);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int GETCARD(
            IntPtr pDev
        );


        [DllImport("openpace_wrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Register_Nfc_Tag_Diconnect_Callback(DISCONNECT cb);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int DISCONNECT(
            IntPtr pDev
        );

        public enum  TAG_CONNECT
        {
            SMARTCARD_ERROR_NO_ERROR = 0,
            SMARTCARD_ERROR_NO_CARD_AVAILABLE
        };



        private const string cTIPO_CEDULA_ERROR = "IO Error";
        private const string cTIPO_CEDULA_NEW = "Nueva";
        private const string cTIPO_CEDULA_OLD = "Antigua";

        public delegate int Card__TxRx_Delegate(
            byte[]      sendBuffer,
            int         sendBufferLength,
            byte[]      receiveData,
            ref int     receiveDataLength,
            ref ushort  sw1sw2
        );

        public delegate int Card__Detect_Delegate(ref bool tag_present);
        public delegate int Card__Disconnect_Delegate();

        public delegate void Tag_Type(string tag_type);


        private Card__TxRx_Delegate?            g_Nfc_IO_Delegate = null;
        private Card__Detect_Delegate?          g_Nfc_Card_Detect_Delegate = null;
        private Card__Disconnect_Delegate?      g_Nfc_Card_Disconnect_Delegate = null;
        private Tag_Type?                       g_Tag_Type = null;


        private TRANSMIT?   g_NFC_TxRx_Callback;
        private GETCARD?    g_NFC_Card_Detect_Callback;
        private DISCONNECT? g_NFC_Card_Disconnect_Callback;


        private bool g_Tag_Old = false;


        public void SetIO(Card__Detect_Delegate card_detect_fnc, Card__Disconnect_Delegate card_disconnet_fnc, Card__TxRx_Delegate ioFunc, Tag_Type tag_type)
        {
            g_Nfc_Card_Detect_Delegate      = card_detect_fnc;
            g_Nfc_Card_Disconnect_Delegate  = card_disconnet_fnc;
            g_Nfc_IO_Delegate               = ioFunc;
            g_Tag_Type                      = tag_type;

            // Persistencia contra GC !
            g_NFC_TxRx_Callback = NFC_TransmitRecieve;
            Register_TxRxNfc_Callback(g_NFC_TxRx_Callback);

            g_NFC_Card_Detect_Callback = NFC_Detect;
            Register_Nfc_Tag_Detect_Callback(g_NFC_Card_Detect_Callback);

            g_NFC_Card_Disconnect_Callback = NFC_Disconnect;
            Register_Nfc_Tag_Diconnect_Callback(g_NFC_Card_Disconnect_Callback);
        }


        private int NFC_Detect(IntPtr dummy)
        {
            bool tag_on_field = false;

            if (g_Nfc_Card_Detect_Delegate is not null)
            {
                g_Nfc_Card_Detect_Delegate.Invoke(ref tag_on_field);
            }

            return tag_on_field ? (int)TAG_CONNECT.SMARTCARD_ERROR_NO_ERROR : (int)TAG_CONNECT.SMARTCARD_ERROR_NO_CARD_AVAILABLE;    
        }

        private int NFC_Disconnect(IntPtr dummy)
        {
            g_Nfc_Card_Disconnect_Delegate?.Invoke();
            return 0;
        }


        private int NFC_TransmitRecieve(   IntPtr dummy,
                                        IntPtr tx,
                                        int txLen,
                                        IntPtr rx,
                                        ref int rxLen)
        {
            if (g_Nfc_IO_Delegate is null) return 1;

            byte[] managedTx = new byte[txLen];
            Marshal.Copy(tx, managedTx, 0, txLen);

            byte[] received = new byte[256];
            int total_received = 0;
            ushort sw1asw2 = 0;

            g_Nfc_IO_Delegate(managedTx, txLen, received, ref total_received, ref sw1asw2);


            byte[] baSw = new byte[2];
            baSw[0] = (byte)(sw1asw2 >> 8);   // MSB
            baSw[1] = (byte)(sw1asw2 & 0xFF); // LSB

            rxLen = 2 + total_received;
            
            if (total_received > 0)
            Marshal.Copy(received, 0, rx, total_received);
            
            
            Marshal.Copy(baSw, 0, rx + total_received, 2);

            return 0;
        }

        private void Set_Tag_Type__Old()
        {
            g_Tag_Old = true;
            g_Tag_Type?.Invoke(cTIPO_CEDULA_OLD);
        }

        private void Set_Tag_Type__New()
        {
            g_Tag_Old = false;
            g_Tag_Type?.Invoke(cTIPO_CEDULA_NEW);
        }




    }
}
