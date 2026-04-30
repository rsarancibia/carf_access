
namespace Edv__Id_Tag_Access.ImgEngines
{
    internal class IsoCompact
    {
        public const int MAX_MOC_ISO_LEN_TO_TX = 120;
        public const int ISOCOMPACT_SCALE = 100;

        public static int Convert(byte[] ISOData, int targetSize, int scale, ref byte[] salida)
        {
            byte[]? m_fingerTemplate = null;

            int status = 0;

            int isoDataLen = ISOData.Length;

            int minutiaeNum = 0;

            while (true)
            {
                if (scale <= 0)
                {
                    status = 210;
                    break;
                }

                if (isoDataLen <= 28)
                {
                    status = 1;
                    break;
                }

                minutiaeNum = ISOData[27] & 0xFF;

                if (!(isoDataLen > (minutiaeNum * 6 + 28)))
                {
                    status = 2;
                    break;
                }

                int resolutionX = (ISOData[18] & 0xFF) * 256 + ISOData[19] & 0xFF;

                int resolutionY = (ISOData[20] & 0xFF) * 256 + ISOData[21] & 0xFF;

                if (!(resolutionX > 0 && resolutionY > 0))
                {
                    status = 3;
                    break;
                }

                int templateLen = minutiaeNum * 3;
                m_fingerTemplate = new byte[templateLen];

                for (int i = 0, j = 28; i < templateLen; i += 3, j += 6)
                {
                    m_fingerTemplate[i] = (byte)(((ISOData[j] & 0x3F) * 256 + (ISOData[j + 1] & 0xFF)) * scale / resolutionX);
                    m_fingerTemplate[i + 1] = (byte)(((ISOData[j + 2] & 0x3F) * 256 + (ISOData[j + 3] & 0xFF)) * scale / resolutionY);
                    m_fingerTemplate[i + 2] = (byte)((ISOData[j] & 0xC0) + ((ISOData[j + 4] >> 2) & 0x3F));
                }

               
                int array_len_output = templateLen;

                if (array_len_output > targetSize)
                {
                    array_len_output = (targetSize - (targetSize % 3));
                }

                salida = new byte[array_len_output];

                Array.Copy(m_fingerTemplate, 0, salida, 0, salida.Length);
                break;
            }

            return status;
        }



    }
}
