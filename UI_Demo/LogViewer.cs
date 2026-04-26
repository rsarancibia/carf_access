using Edv__Id_Tag_Access.myLog;
using Serilog.Events;

namespace UI_Demo
{
    public static class LogViewer
    {
        public static void Attach(RichTextBox rtb)
        {
            // Configuración visual tipo consola
            rtb.BackColor = Color.Black;
            rtb.ForeColor = Color.White;
            rtb.Font = new Font("Consolas", 12);
            rtb.ReadOnly = true;

            LogBridge.OnLog += logEvent =>
            {
                if (rtb.IsDisposed) return;

                rtb.BeginInvoke(() =>
                {
                    // Limitar tamaño (evita lag)
                    if (rtb.Lines.Length > 1000)
                    {
                        rtb.Clear();
                    }

                    Color levelColor = GetColor(logEvent.Level);

                    string timestamp = logEvent.Timestamp.ToString("HH:mm:ss");
                    string level = GetLevelShort(logEvent.Level);
                    string message = logEvent.RenderMessage();

                    // Posición final
                    rtb.SelectionStart = rtb.TextLength;
                    rtb.SelectionLength = 0;

                    // Timestamp
                    rtb.SelectionColor = Color.DarkGray;
                    rtb.AppendText($"[{timestamp} ");

                    // Nivel
                    rtb.SelectionColor = levelColor;
                    rtb.AppendText(level);

                    // Cierre
                    rtb.SelectionColor = Color.DarkGray;
                    rtb.AppendText("] ");

                    // Mensaje
                    rtb.SelectionColor = Color.White;
                    rtb.AppendText(message + "\n");

                    // Excepción (si existe)
                    if (logEvent.Exception != null)
                    {
                        rtb.SelectionColor = Color.Red;
                        rtb.AppendText(logEvent.Exception + "\n");
                    }

                    // Auto-scroll
                    rtb.SelectionStart = rtb.Text.Length;
                    rtb.ScrollToCaret();
                });
            };
        }

        private static Color GetColor(LogEventLevel level)
        {
            return level switch
            {
                LogEventLevel.Verbose => Color.Gray,
                LogEventLevel.Debug => Color.DarkGray,
                LogEventLevel.Information => Color.LightGreen,
                LogEventLevel.Warning => Color.Gold,
                LogEventLevel.Error => Color.Red,
                LogEventLevel.Fatal => Color.DarkRed,
                _ => Color.White
            };
        }

        private static string GetLevelShort(LogEventLevel level)
        {
            return level switch
            {
                LogEventLevel.Verbose => "VRB",
                LogEventLevel.Debug => "DBG",
                LogEventLevel.Information => "INF",
                LogEventLevel.Warning => "WRN",
                LogEventLevel.Error => "ERR",
                LogEventLevel.Fatal => "FTL",
                _ => "UNK"
            };
        }
    }
}
