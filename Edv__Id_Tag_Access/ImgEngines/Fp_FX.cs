using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Edv__Id_Tag_Access.ImgEngines
{
    internal class Fp_FX
    {
        public const uint FJFX_FMD_ISO_19794_2_2005 = 0x01010001;
        public const int FJFX_FMD_BUFFER_SIZE = 34 + (256 * 6);

        [DllImport("libFJFX.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int fjfx_create_fmd_from_raw(
        byte[] raw_image,
        ushort dpi,
        ushort height,
        ushort width,
        uint output_format,
        byte[] fmd,
        ref uint size_of_fmd
    );


    }
}
