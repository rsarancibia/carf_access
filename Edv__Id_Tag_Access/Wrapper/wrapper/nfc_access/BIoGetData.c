#ifndef __BIO_GET_DATA_H__
#define __BIO_GET_DATA_H__

#include <BIoGetData.h>
#include <stdio.h>
#include <utils.h>

unsigned char	g_Template_ISO19794[256];
int				g_Template_ISO19794_Len;

void	sBioSetTemplate(unsigned char *template, int template_len)
{
    if (template_len > sizeof(g_Template_ISO19794))
    {
        template_len = sizeof(g_Template_ISO19794);
    }

    memcpy(g_Template_ISO19794, template, template_len);
    
    g_Template_ISO19794_Len = template_len; 

    PutInLog(NULL, LOG_LEVEL_NOTICE, "TEMPLATE RECIBIDO -> Len %d!!!!!", g_Template_ISO19794_Len); 

    PutInLog(NULL, LOG_LEVEL_NOTICE, "TEMPLATE RECIBIDO!!!!!");

    //DisplayHex(LOG_LEVEL_NOTICE, g_Template_ISO19794, 16); 


}

int sBioGetData(void *handle, int info_id, int info_type, unsigned char *buffer_out, int *buffer_out_len)
{
    memcpy(buffer_out, g_Template_ISO19794, g_Template_ISO19794_Len);
    *buffer_out_len = g_Template_ISO19794_Len;
    return 0;
}

int sBioPutData(void *handle, int info_id, int info_type, unsigned char *buffer_in, int buffer_len)
{
	return 0;
}

int sBioCleanData(void *handle, int info_id, int info_type)
{
	return 0;
}

int scl_GetFnBitmap(void *handle, int tag)
{
	return 0;
}


#endif