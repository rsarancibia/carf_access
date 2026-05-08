#include <BIoGetData.h>
#include <stdio.h>
#include <utils.h>

static unsigned char	g_Template_ISO19794[256];
static int				g_Template_ISO19794_Len;

static unsigned char	g_FaceJP2[1024*30];
static int				g_FaceJP2_Len;

static unsigned char	g_SignatureJP2[1024 * 30];
static int				g_Signature_Len;



void	sBioSetTemplate(unsigned char *template, int template_len)
{
    if (template_len > sizeof(g_Template_ISO19794))
    {
        template_len = sizeof(g_Template_ISO19794);
    }

    memcpy(g_Template_ISO19794, template, template_len);
    
    g_Template_ISO19794_Len = template_len; 
}

int sBioGetData(stHndBioPtr handle, int info_id, int info_type, unsigned char *buffer_out, int *buffer_out_len)
{
    unsigned char   *ptrFrom;
	int             lenFrom;

        switch (info_type)
        {
            case BIO_DATAFIELDPROP_MINUTIAE_SOLISOC:

                ptrFrom = g_Template_ISO19794;
			    lenFrom = g_Template_ISO19794_Len;
                break;
        
            case BIO_DATAFIELD_DOC_DG_2:
        
                ptrFrom = ((sHndICAOPtr)(handle->HndICAO))->aDoc_DG_2;
                lenFrom = ((sHndICAOPtr)(handle->HndICAO))->iDoc_DG_2Len;
                break;
        
            case BIO_DATAFIELD_DOC_FACE:

                ptrFrom = g_FaceJP2;
                lenFrom = g_FaceJP2_Len;
                break;

            case BIO_DATAFIELD_DOC_SIGNATURE:

                ptrFrom = g_SignatureJP2;
                lenFrom = g_Signature_Len;
                break;


            default:
                return 1;
        }
    

        if (buffer_out == NULL || buffer_out_len == NULL) return 2;

        if (*buffer_out_len < lenFrom)
        {
            return 3;
        }
                
        memcpy(buffer_out, ptrFrom, lenFrom);
        *buffer_out_len = lenFrom;

    return 0;
}

int sBioPutData(void *handle, int info_id, int info_type, unsigned char *buffer_in, int buffer_len)
{
	int output_available_len;
    int *poutput_len;
    unsigned char* pOut;

    switch (info_id)
    {
        case BIO_DATAFIELD_DOC_FACE:
	
            pOut = g_FaceJP2;
            output_available_len = sizeof(g_FaceJP2);
			poutput_len = &g_FaceJP2_Len;
            break;

        case BIO_DATAFIELD_DOC_SIGNATURE:

            pOut = g_SignatureJP2;
            output_available_len = sizeof(g_SignatureJP2);
			poutput_len = &g_Signature_Len;
            break;

		default:
            return 1;
    }

    if (buffer_len > output_available_len)
        return 2;
  
    
    memcpy(pOut, buffer_in, buffer_len);
    *poutput_len = buffer_len;

    return 0;
}

int sBioCleanData(void *handle, int info_id, int info_type)
{
	return 0;
}

int scl_GetFnBitmap(void *handle, int tag)
{
    return  SCL_FNBITMAP_ON;
}