using Serilog;
using System.Text;

namespace Edv__Id_Tag_Access.myLog
{
    public static class LoggingExtensions
    {
        public static void HexDump(this ILogger logger, byte[] buffer_2_show, int data_lenght = -1, string message = "HexDump")
        {
            if (data_lenght < 0)
            {
                data_lenght = buffer_2_show.Length;
            }

            var dump = HexDumpInternal(buffer_2_show, data_lenght);

            logger.Information($"{message}\n{dump}");
        }

        private static string HexDumpInternal(byte[] buffer_2_show, int data_length, int bytesPerLine = 16)
        {
            var sb = new StringBuilder();

            if (data_length < bytesPerLine)
            {
                bytesPerLine = data_length;
            }

            for (int i = 0; i < data_length; i += bytesPerLine)
            {
                int remaining = data_length - i;
                int currentLineLength = Math.Min(bytesPerLine, remaining);

                sb.Append($"{i:X8}  ");

                for (int j = 0; j < currentLineLength; j++)
                    sb.Append($"{buffer_2_show[i + j]:X2} ");

                sb.Append(" ");

                for (int j = 0; j < currentLineLength; j++)
                {
                    var b = buffer_2_show[i + j];
                    sb.Append(b >= 32 && b <= 126 ? (char)b : '.');
                }

                sb.AppendLine();
            }

            return sb.ToString();
        }
    }
}
