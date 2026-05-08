#ifndef __BIO_GET_DATA_H__
#define __BIO_GET_DATA_H__

#include <stdio.h>
#include <Tag_ICAO.h>


typedef int enDataField;

void	sBioSetTemplate(unsigned char* template, int template_len);

int sBioGetData(stHndBioPtr handle, int info_id, int info_type, unsigned char *buffer_out, int *buffer_out_len);

int sBioPutData(void *handle, int info_id, int info_type, unsigned char *buffer_in, int buffer_len);

int sBioCleanData(void *handle, int info_id, int info_type);

int scl_GetFnBitmap(void *handle, int tag);

#endif