using System.Drawing.Imaging;

namespace UI_Demo
{
    internal class Helper
    {

        public static Bitmap RawToBitmap(byte[] raw, int width, int height)
        {
            Bitmap bmp = new Bitmap(width, height, PixelFormat.Format8bppIndexed);

            // 1. Configurar paleta de grises
            ColorPalette palette = bmp.Palette;
            for (int i = 0; i < 256; i++)
            {
                palette.Entries[i] = Color.FromArgb(i, i, i);
            }
            bmp.Palette = palette;

            // 2. Copiar datos al bitmap
            BitmapData bmpData = bmp.LockBits(
                new Rectangle(0, 0, width, height),
                ImageLockMode.WriteOnly,
                PixelFormat.Format8bppIndexed);

            IntPtr ptr = bmpData.Scan0;
            int stride = bmpData.Stride;

            // Ojo: stride puede ser mayor que width (padding)
            for (int y = 0; y < height; y++)
            {
                System.Runtime.InteropServices.Marshal.Copy(
                    raw,
                    y * width,
                    ptr + y * stride,
                    width);
            }

            bmp.UnlockBits(bmpData);

            return bmp;
        }
    }
}
