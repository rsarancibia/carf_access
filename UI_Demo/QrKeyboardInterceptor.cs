using System.Text;
using System.Diagnostics;

namespace UI_Demo
{
   public class QrKeyboardInterceptor : IMessageFilter
    {
        private StringBuilder _buffer = new StringBuilder();
        private Stopwatch _timer = new Stopwatch();

        private const int TIMEOUT_MS = 80; // ajusta según lector
        private bool _reading = false;

        public event Action<string> OnQrRead;

        public bool PreFilterMessage(ref Message m)
        {
            const int WM_KEYDOWN = 0x0100;


            if (m.Msg == WM_KEYDOWN)
            {
                Keys key = (Keys)m.WParam;

                // SI estás leyendo → consume TODO
                if (_reading)
                {
                    char c = KeyToChar(key);

                    if (c != '\0')
                    {
                        if (_timer.ElapsedMilliseconds > TIMEOUT_MS)
                        {
                            FinishRead();
                            _buffer.Clear();
                        }

                        _timer.Restart();
                        _buffer.Append(c);
                    }
                    else
                    {
                        // teclas como ENTER → podrías terminar lectura aquí si quieres
                        if (key == Keys.Enter)
                        {
                            FinishRead();
                        }
                    }

                    return true; // 🔴 IMPORTANTE: consumir SIEMPRE
                }

                // Si aún no estás leyendo
                char first = KeyToChar(key);
                if (first != '\0')
                {
                    _buffer.Clear();
                    _buffer.Append(first);
                    _timer.Restart();
                    _reading = true;

                    return true;
                }
            }


            // verificar timeout incluso si no hay nuevas teclas
            if (_reading && _timer.ElapsedMilliseconds > TIMEOUT_MS)
            {
                FinishRead();
            }

            return false;
        }

        private void FinishRead()
        {
            _reading = false;

            if (_buffer.Length > 0)
            {
                OnQrRead?.Invoke(_buffer.ToString());
            }

            _buffer.Clear();
            _timer.Reset();
        }

        private char KeyToChar(Keys key)
        {
            // básico: letras y números
            if (key >= Keys.A && key <= Keys.Z)
                return key.ToString()[0];

            if (key >= Keys.D0 && key <= Keys.D9)
                return (char)('0' + (key - Keys.D0));

            if (key == Keys.Space) return ' ';
            if (key == Keys.OemMinus) return '-';
            if (key == Keys.OemPeriod) return '.';

            return '\0';
        }
    }
}
