#include <utils.h>
#include <Tag_ICAO.h>
#include <string.h>

stSolemBio	g_SBIO;


__declspec(dllexport)
int Edv_Init()
{
    memset((void *)&g_SBIO, 0, sizeof(stSolemBio));

    return SolemICAOInit(&g_SBIO);
}