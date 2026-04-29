#ifndef __BIO_GET_DATA_H__
#define __BIO_GET_DATA_H__

#include <BIoGetData.h>


int sBioGetData(void *handle, int info_id, int info_type, unsigned char *buffer_out, int *buffer_out_len)
{
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