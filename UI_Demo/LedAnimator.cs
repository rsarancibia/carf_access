//using System;
//using System.Drawing;
using System.Drawing.Drawing2D;
using System.Reflection;
//using System.Windows.Forms;

namespace UI_Demo
{

    internal class LedAnimator
    {
        private readonly Button _button;
        private readonly System.Windows.Forms.Timer _timer;

        private int _intensity = 0;

        // Duración total aprox:
        private const int FadeDurationMs = 2000;

        // Intervalo del timer
        private const int TimerInterval = 30;

        // Intensidad máxima
        private const int MaxIntensity = 255;

        // Cuánto baja cada tick
        private readonly float _step;

        public LedAnimator(Button button)
        {
            _button = button;

            // --- Apariencia del botón ---
            _button.Text = "";
            _button.FlatStyle = FlatStyle.Flat;
            _button.FlatAppearance.BorderSize = 0;
            _button.BackColor = Color.Black;
            _button.TabStop = false;

            MakeCircular();

            // Redibujar si cambia tamaño
            _button.Resize += (s, e) => MakeCircular();

            // Timer NO bloqueante (UI friendly)
            _timer = new System.Windows.Forms.Timer();
            _timer.Interval = TimerInterval;
            _timer.Tick += Timer_Tick;

            // cálculo de decremento
            _step = (float)MaxIntensity / (FadeDurationMs / TimerInterval);

            // Pintado custom
            _button.Paint += Button_Paint;
        }

        // ----------------------------------------------------
        // Hace el botón circular
        // ----------------------------------------------------
        private void MakeCircular()
        {
            GraphicsPath path = new GraphicsPath();

            path.AddEllipse(0, 0, _button.Width - 1, _button.Height - 1);

            _button.Region = new Region(path);
        }

        // ----------------------------------------------------
        // Flash del LED
        // ----------------------------------------------------
        public void Flash()
        {
            _intensity = MaxIntensity;

            _timer.Stop();
            _timer.Start();

            _button.Invalidate();
        }

        // ----------------------------------------------------
        // Animación fade-out
        // ----------------------------------------------------
        private void Timer_Tick(object sender, EventArgs e)
        {
            _intensity -= (int)_step;

            if (_intensity <= 0)
            {
                _intensity = 0;
                _timer.Stop();
            }

            _button.Invalidate();
        }

        // ----------------------------------------------------
        // Dibujo del LED
        // ----------------------------------------------------
        private void Button_Paint(object sender, PaintEventArgs e)
        {
            e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;


            int border = 1;

            Rectangle rect = new Rectangle(
                border / 2,
                border / 2,
                _button.Width - border,
                _button.Height - border);


            //Rectangle rect = new Rectangle(
            //    0,
            //    0,
            //    _button.Width - 1,
            //    _button.Height - 1);






            // Fondo oscuro
            using (SolidBrush backBrush = new SolidBrush(Color.FromArgb(25, 125, 25)))
            {
                e.Graphics.FillEllipse(backBrush, rect);
            }

            // Color LED actual
            Color ledColor = Color.FromArgb(0, _intensity, 0);

            using (SolidBrush ledBrush = new SolidBrush(ledColor))
            {
                Rectangle inner = new Rectangle(
                    4,
                    4,
                    _button.Width - 8,
                    _button.Height - 8);

                e.Graphics.FillEllipse(ledBrush, inner);
            }

            // Brillo superior
            using (SolidBrush shine = new SolidBrush(Color.FromArgb(_intensity / 3, 255, 255, 255)))
            {
                Rectangle shineRect = new Rectangle(
                    _button.Width / 4,
                    _button.Height / 6,
                    _button.Width / 3,
                    _button.Height / 5);

                e.Graphics.FillEllipse(shine, shineRect);
            }

            // Borde
            //using (Pen pen = new Pen(Color.FromArgb(60, 60, 60), 0.1f))
            //{
            //    e.Graphics.DrawEllipse(pen, rect);
            //}


            using (Pen pen = new Pen(Color.FromArgb(60, 160, 60), border))
            {
                e.Graphics.DrawEllipse(pen, rect);
            }

        }
    }
}
