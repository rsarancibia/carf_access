#include <string.h>
#include <stdio.h>

#include <Tag_ICAO.h>

/*
#include "SecLogger.h"
#include "SmartCard.h"
#include "Bcr250bt.h"
#include "BioPin.h"
#include "misc.h"
#include "Nfc.h"
*/
#include <utils.h>
#include <Coder.h>
#include <openssl/rand.h>
#include <eac/eac.h>
#include <eac/pace.h>
#include <openssl/bio.h>


#include <misc.h>
#include <BIoGetData.h>



//#define _FOR_BECH		// Workaround for BECH. The don't expect SOLEMBIO_ERROR_MOC_LOCKED in MOC, so it returns ok if card is bloqued.

int		iIsOld = 0;
EAC_CTX* pcd_ctx = NULL;

static unsigned char parity(unsigned char in){
	unsigned char uc = in;
	int k = 0;
	
	if((in&0x01) != 0) k++;
	if((in&0x02) != 0) k++;
	if((in&0x04) != 0) k++;
	if((in&0x08) != 0) k++;
	if((in&0x10) != 0) k++;
	if((in&0x20) != 0) k++;
	if((in&0x40) != 0) k++;
	if((in&0x80) != 0) k++;
	
	if(k%2 == 0){
		if((uc&0x01) != 0){
			return (uc&0xFE);
		}else{
			return (uc|0x01);
		}
	}else{
		return uc;
	}
}

static int increment8bitCounter(unsigned char* ucpCount){
	int i;
	if(ucpCount == NULL)
		return -1;
	for(i=7;i>=0;i--){
		if(ucpCount[i] != 0xFF){
			ucpCount[i]++;
			return i;
		}else{
			ucpCount[i] = 0;
		}
	}
	return -2;
}

static char getDv(char *cpDigits){
	
	int i;
	int iTmp1 = 0;
	int iTmp2 = 7;
	int iTmp3 = 0;
	
	iTmp1 = strlen(cpDigits);
	
	for(i=0;i<iTmp1;i++){
		int convChar = 0;
		if (cpDigits[i] >= 0x30 && cpDigits[i] <= 0x39)
			convChar = cpDigits[i] - 0x30;
		else if(cpDigits[i] >= 65 || cpDigits[i] <= 90)
			convChar = cpDigits[i] - 55;
		else
			return 0;

		iTmp3 += convChar * iTmp2;
		switch(iTmp2){
		case 7:
			iTmp2=3;
			break;
		case 3:
			iTmp2=1;
			break;
		case 1:
			iTmp2=7;
			break;
		} 
	}
	
	return ((iTmp3%10)  + 0x30);
}

static int wrap(unsigned char* tag, int tagLen, unsigned char* inData, int inLen, unsigned char* outData, int outLen)
{
	int resultLen = -1;
	int Pos = 0;

	if (outLen >= tagLen)
	{
		memcpy_s(outData, outLen, tag, tagLen);
		Pos += tagLen;
		if (inLen > 0xFF)
		{
			if (outLen >= (Pos + 3))
			{
				outData[Pos++] = 0x82;
				outData[Pos++] = (unsigned char)((inLen >> 8) & 0xFF);
				outData[Pos++] = (unsigned char)(inLen & 0xFF);
			}
		}
		else if (inLen > 0x7F)
		{
			if (outLen >= (Pos + 2))
			{
				outData[Pos++] = 0x81;
				outData[Pos++] = (unsigned char)(inLen & 0xFF);
			}
		}
		else
		{
			if (outLen >= (Pos + 1))
			{
				outData[Pos++] = (unsigned char)inLen;
			}
		}
		if (outLen >= (Pos + inLen))
		{
			if (inLen > 0)
			{
				memcpy_s(outData + Pos, outLen, inData, inLen);
			}
			resultLen = Pos + inLen;
		}
	}
	return resultLen;
}

static int unWrap(unsigned char* tag, int tagLen, unsigned char* inData, int inLen, unsigned char* outData, int outLen)
{
	int resultLen = -1;

	if (inData > 0)
	{
		int Pos;
		for (Pos = 0; Pos < tagLen && Pos < inLen; Pos++)
		{
			if (tag[Pos] != inData[Pos])
				break;
		}
		if (Pos >= tagLen)
		{
			int Len = inData[Pos++];
			if (Len > 0x80)
			{
				int iTmp1 = Len & 0x0F;
				Len = 0;
				for (int i = 0; i < iTmp1; i++)
				{
					Len <<= 8;
					Len |= inData[Pos++];
				}
			}
			if (Len > 0 && Len == (inLen - Pos))
			{
				if (outLen >= Len)
				{
					memcpy_s(outData, outLen, inData + Pos, Len);
					resultLen = Len;
				}
			}
		}
	}
	return resultLen;
}

static int transmitPlainApdu(stSolemICAOPtr opSICAO, void *pDev, unsigned char* ucpHeader, unsigned char ucLc, unsigned char* ucpData, unsigned char ucLe, unsigned char* ucpSw1, unsigned char* ucpSw2, unsigned char* ucpApduRsp, int *ipApduRspLen)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;
int					iTmpApduRspLen = 0;
unsigned char		aApduMsg[255 + 6];
int					iApduMsgLen = 255 + 6;
unsigned char		aApduResponse[256];// +2];
int					iApduResponseLen = 256 + 2;

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;
	//	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemBio - SolemICAO - transmitPlainApdu - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	*ucpSw1 = 0;
	*ucpSw2 = 0;
	//iTmpApduRspLen = *ipApduRspLen;
	//*ipApduRspLen = 0;

	// Compose Apdu
	iApduMsgLen = sizeof(aApduMsg);
	memset(aApduMsg, 0, iApduMsgLen);
	memcpy_s(aApduMsg, sizeof(aApduMsg), ucpHeader, 4);
	iApduMsgLen = 4;
	if(ucLc != 0){
		aApduMsg[iApduMsgLen++] = ucLc;
		memcpy_s(aApduMsg + iApduMsgLen, sizeof(aApduMsg) - iApduMsgLen, ucpData, ucLc);
		iApduMsgLen += ucLc;
	}
	// if(ucLe != 0x00 || ucLe != 0)
		aApduMsg[iApduMsgLen++] = ucLe;


	//	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char *)"SolemBio - transmitPlainApdu Cmd:");
	//	DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduMsg, iApduMsgLen);
	//	iTmpApduRspLen = 2;
	//	if (ucLe != 0)
	//	{
	//		iTmpApduRspLen += ucLe;
	//		if (ucLe > 126)
	//			iTmpApduRspLen++;
	//	}
	
	// Transmit Apdu
	//iApduResponseLen = iTmpApduRspLen;	// sizeof(aApduResponse);
	iApduResponseLen = sizeof(aApduResponse);
	memset(aApduResponse, 0, sizeof(aApduResponse));
#ifdef _DEBUG
	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitApdu - Tx Frame [%d]", iApduMsgLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduMsg, iApduMsgLen);
#endif
	pSBIO->lReturn = opSICAO->fn_Transmit(pDev, aApduMsg, (long)iApduMsgLen, aApduResponse, (long*)&iApduResponseLen);
	if(pSBIO->lReturn != SMARTCARD_ERROR_NO_ERROR)
	{
		pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_PLAIN_APDU;
		//	PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - ERROR Transmit Apdu [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_transmitPlainApdu;
	}
#ifdef _DEBUG
	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitApdu - Rx Frame [%d]", iApduResponseLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduResponse, iApduResponseLen);
#endif
	//	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char *)"SolemBio - transmitPlainApdu Rsp:");
	//	DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduResponse, iApduResponseLen);

	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char *)"SolemBio - transmitApdu Le:%d RspLen:%d Dif:%d", ucLe, iApduResponseLen, iApduResponseLen - ucLe);

	if(iApduResponseLen < 2)
	{
		pSBIO->lReturn = iApduResponseLen;
		pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_PLAIN_APDU_RESP;
		//	PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - ERROR Transmit Apdu Resp [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_transmitPlainApdu;
	}

	*ucpSw1 = aApduResponse[iApduResponseLen - 2];
	*ucpSw2 = aApduResponse[iApduResponseLen - 1];

	if(iApduResponseLen > 2)
	{
		memcpy_s(ucpApduRsp, *ipApduRspLen, aApduResponse, iApduResponseLen - 2);
		*ipApduRspLen = iApduResponseLen - 2;
	}
	else
	{
		*ipApduRspLen = 0;
	}

	//	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - transmitPlainApdu - OUT");
JMP_transmitPlainApdu:

	return pSBIO->lError;
}

static int udateICAOkey(stSolemICAOPtr opSICAO)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;

int					iTmp;
char				*cpTmp1;
int					iDocType;
char				sDocBarcode[256];
char					sDocSerial[32];
char					sDocBirthDate[16];
char					sDocExpityDate[16];

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - udateICAOkey - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;
	
	memset(sDocSerial, 0, sizeof(sDocSerial));
	memset(sDocBirthDate, 0, sizeof(sDocBirthDate));
	memset(sDocExpityDate, 0, sizeof(sDocExpityDate));

	// Get Doc MRZ from QR
	iTmp = sizeof(sDocBarcode);
	pSBIO->lReturn = sBioGetData(pSBIO, SOLEMBIO_DATAFIELD_DOC_BARCODE, SOLEMBIO_DATAFIELDPROP_NONE, sDocBarcode, &iTmp);
	if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR && pSBIO->lReturn != SOLEMBIO_ERROR_NO_DATA_AVAILABLE)
	{
		pSBIO->lError = SOLEMICAO_ERROR_UPDATEKEY_GETDOCBARCODE;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - udateICAOkey - ERROR Get Doc Barcode [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_udateICAOkey;
	}
	else if (pSBIO->lReturn == SOLEMBIO_ERROR_NO_ERROR)
	{
		//opSICAO->sICAOKey
		pSBIO->lReturn = sBioGetData(pSBIO, SOLEMBIO_DATAFIELD_DOC_TYPE, SOLEMBIO_DATAFIELDPROP_NONE, NULL, &iDocType);
		if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_UPDATEKEY_GETDOCTYPE;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - udateICAOkey - ERROR Get Doc Type [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_udateICAOkey;
		}
		if (iDocType != SOLEMBIO_DOCTYPE_CLID2013 && iDocType != SOLEMBIO_DOCTYPE_CLID2013_ESP)
		{
			pSBIO->lError = SOLEMICAO_ERROR_UPDATEKEY_INVALID_DOCTYPE;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - udateICAOkey - ERROR Invalid Doc Type [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_udateICAOkey;
		}

		if (iTmp > 0)
		{
			cpTmp1 = strstr((char*)sDocBarcode, (char*)"&mrz=");
			if (cpTmp1 != NULL)
			{
				cpTmp1 += 5;
				memset(opSICAO->sICAOKey, 0, sizeof(opSICAO->sICAOKey));
				memcpy_s(opSICAO->sICAOKey, sizeof(opSICAO->sICAOKey), cpTmp1, 24);
			}
		}
	}
	else if(pSBIO->lReturn == SOLEMBIO_ERROR_NO_DATA_AVAILABLE)
	{
		// Get Doc Serial
		iTmp = sizeof(sDocSerial);
		pSBIO->lReturn = sBioGetData(pSBIO, SOLEMBIO_DATAFIELD_DOC_SERIAL, SOLEMBIO_DATAFIELDPROP_NONE, sDocSerial, &iTmp);
		if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR && pSBIO->lReturn != SOLEMBIO_ERROR_NO_DATA_AVAILABLE)
		{
			pSBIO->lError = SOLEMICAO_ERROR_UPDATEKEY_GETDOCSERIAL;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - udateICAOkey - ERROR Get Doc Serial [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_udateICAOkey;
		}
		else if(pSBIO->lReturn == SOLEMBIO_ERROR_NO_DATA_AVAILABLE)
		{
			pSBIO->lError = SOLEMICAO_ERROR_UPDATEKEY_DOCSERIAL_UNAVAILABLE;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - udateICAOkey - ERROR Doc Serial Unavailable[%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_udateICAOkey;
		}

		// Get Doc BirthDate
		iTmp = sizeof(sDocBirthDate);
		pSBIO->lReturn = sBioGetData(pSBIO, SOLEMBIO_DATAFIELD_DOC_BIRTHDATE, SOLEMBIO_DATAFIELDPROP_NONE, sDocBirthDate, &iTmp);
		if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR && pSBIO->lReturn != SOLEMBIO_ERROR_NO_DATA_AVAILABLE)
		{
			pSBIO->lError = SOLEMICAO_ERROR_UPDATEKEY_GETDOCBIRTHDATE;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - udateICAOkey - ERROR Get Doc BirthDate [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_udateICAOkey;
		}
		else if(pSBIO->lReturn == SOLEMBIO_ERROR_NO_DATA_AVAILABLE)
		{
			pSBIO->lError = SOLEMICAO_ERROR_UPDATEKEY_DOCBIRTHDATE_UNAVAILABLE;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - udateICAOkey - ERROR Doc BirthDate Unavailable[%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_udateICAOkey;
		}

		// Get Doc ExpiryDate
		iTmp = sizeof(sDocExpityDate);
		pSBIO->lReturn = sBioGetData(pSBIO, SOLEMBIO_DATAFIELD_DOC_EXPIRYDATE, SOLEMBIO_DATAFIELDPROP_NONE, sDocExpityDate, &iTmp);
		if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR && pSBIO->lReturn != SOLEMBIO_ERROR_NO_DATA_AVAILABLE)
		{
			pSBIO->lError = SOLEMICAO_ERROR_UPDATEKEY_GETDOCEXPIRYDATE;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - udateICAOkey - ERROR Get Doc ExpiryDate [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_udateICAOkey;
		}
		else if(pSBIO->lReturn == SOLEMBIO_ERROR_NO_DATA_AVAILABLE)
		{
			pSBIO->lError = SOLEMICAO_ERROR_UPDATEKEY_DOCEXPIRYDATE_UNAVAILABLE;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - udateICAOkey - ERROR Doc ExpiryDate Unavailable[%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_udateICAOkey;
		}

		iTmp = strlen(sDocSerial);
		sDocSerial[iTmp] = getDv(sDocSerial);
		iTmp = strlen(sDocBirthDate);
		sDocBirthDate[iTmp] = getDv(sDocBirthDate);
		iTmp = strlen(sDocExpityDate);
		sDocExpityDate[iTmp] = getDv(sDocExpityDate);

		sprintf_s(opSICAO->sICAOKey, sizeof(opSICAO->sICAOKey), "%s%s%s", sDocSerial, sDocBirthDate, sDocExpityDate);
	}

	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - udateICAOkey - icaoKey [%s]", opSICAO->sICAOKey);

	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - udateICAOkey - OUT");

JMP_udateICAOkey:

	return pSBIO->lError;
}

static int getMac(stSolemICAOPtr opSICAO, unsigned char *ucpData, int iDataLen,unsigned char *ucpMac, int *ipMacLen)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;

unsigned char		aIvMac[8];
unsigned char		a8KeyMac[8];
unsigned char		aKeyMac8[8];

unsigned char		aDataEnc[256];
int					iDataEncLen = 0;
unsigned char		aDataDenc[8];
int					iDataDencLen = 0;

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - getMac - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	memset(aIvMac, 0, sizeof(aIvMac));
	memcpy_s(a8KeyMac, sizeof(a8KeyMac), opSICAO->aKeyMac, 8);
	memcpy_s(aKeyMac8, sizeof(aKeyMac8), opSICAO->aKeyMac + 8, 8);
	
	//	Encrypt data = aDataEnc
	iDataEncLen = sizeof(aDataEnc);
	memset(aDataEnc, 0, iDataEncLen);
	pSBIO->lReturn = getCrypt(NID_des_cbc, CODER_CRYPT_ENCRYPT, a8KeyMac, sizeof(a8KeyMac), aIvMac, sizeof(aIvMac), ucpData, iDataLen, aDataEnc, &iDataEncLen, 0);
	if (pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_GETMAC_ENCRYPT;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - getMac - ERROR Encrypt [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_getMac;
	}
	
	// Decrypt last 8 bytes of aDataEnc = aDataDenc
	iDataDencLen = sizeof(aDataDenc);
	memset(aDataDenc, 0, iDataDencLen);
	pSBIO->lReturn = getCrypt(NID_des_ecb, CODER_CRYPT_DECRYPT, aKeyMac8, sizeof(aKeyMac8), NULL, 0, aDataEnc + iDataEncLen - 8, 8, aDataDenc, &iDataDencLen, 0);
	if (pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_GETMAC_DECRYPT;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - getMac - ERROR Decrypt [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_getMac;
	}

	// Encrypt aDataDenc = Mac
	memset(ucpMac, 0, *ipMacLen);
	pSBIO->lReturn = getCrypt(NID_des_ecb, CODER_CRYPT_ENCRYPT, a8KeyMac, sizeof(a8KeyMac), NULL, 0, aDataDenc, iDataDencLen, ucpMac, ipMacLen, 0);
	if (pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_GETMAC_DECRYPT;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - getMac - ERROR Decrypt [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_getMac;
	}
	
	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - getMac - OUT");

JMP_getMac:
	return pSBIO->lError;
}

static int transmitSecureApdu(stSolemICAOPtr opSICAO, void *pDev, unsigned char* ucpHeader, unsigned char ucLc, unsigned char* ucpData, unsigned char ucLe, unsigned char* ucpSw1, unsigned char* ucpSw2, unsigned char* ucpApduRsp, int *ipApduRspLen)
{

stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;

unsigned char		aIv[8];

unsigned char		aSecureHeaderPadd[8];	// Secure header = 0x0C|HEAD_1-3|0x80|0x00|0x00|0x00
unsigned char		aDataPadd[300];			// DATA_PADD = DATA|PADDING  aDataPadd[232];
int					iDataPaddLen;				// DATA_PADD_LEN = Lc + 8 - Lc % 8
unsigned char		aDo87[300];					// DO87 = 0x87|LC+1|0x01|tDES(DATA_PADD)  aDo87[235];
int					iDo87Len;					// DO87_LEN = 3 + DATA_PADD_LEN
int					iDo87Offset;				// DO87_OFFSET = IF DO87_LEN < 0x81 THEN = 3 IF DO87_LEN >= 0x81 THEN = 4
unsigned char		aDo97[3];					// DO97 = 0x97|0x01|LE
unsigned char		aDo8EDataPadd[300];		//		SSC|HEAD_PADD|DO87|DO97|PADDING = 8|8|DO87|3|PADDING
int					iDo8EDataPaddLen;			//		 8 |    8    |235 | 3  |   2    = 256
unsigned char		aDo8E[10];					// DO8E = 0x8E|0x08|MAC(SSC|HEAD_PADD|DO87|DO97|PADDING) 
int					iDo8ELen;					// DO8E_LEN = 2 + 8
unsigned char		aApduData[300];			// APDU_DATA = DO87|DO97|DO8E
int					iApduDataLen = 255;		// APDU_DATA_LEN = DO87_LEN + 3 + DO8E_LEN = 13 + DO87_LEN = 16 + DATA_PADD_LEN = 24 + Lc - Lc % 8 < 255 => Lc_max = 224

unsigned char		ucSw1;
unsigned char		ucSw2;
unsigned char		aApduResponse[256 + 2];
int					iApduResponseLen = 256 + 2;

unsigned char		aDo99[4];
unsigned char		aDo8EMac[8];

int					iTmpApduRspLen;

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;
	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - transmitSecureApdu - IN");

#ifdef _DEBUG
	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Input Apdu Header [%ld]", 4); DisplayHex(pLogger, LOG_LEVEL_DEBUG, ucpHeader, 4);
	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Input Apdu Data [%ld][%ld]", ucLc, ucLe); DisplayHex(pLogger, LOG_LEVEL_DEBUG, ucpData, ucLc);
#endif

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	memset(aIv, 0, sizeof(aIv));

	*ucpSw1 = 0;
	*ucpSw2 = 0;
	//iTmpApduRspLen = *ipApduRspLen;
	//*ipApduRspLen = 0;
	
	if (!iIsOld)
	{
		if (pcd_ctx == NULL)
		{
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR EAC Context NULL");
			pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDU_DO87_ENC;
			goto JMP_transmitSecureApdu;
		}
	}
	
	// Increment session counter
	if(iIsOld)
		increment8bitCounter(opSICAO->aSessCount);
	else
	{
		if (EAC_increment_ssc(pcd_ctx) == 0)
		{
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR EAC_increment_ssc");
			pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDU_DO87_ENC;
			goto JMP_transmitSecureApdu;
		}
	}
	
	//APDU = HEAD|lc|DO87|DO97|DO8E|le
	//APDU =    4| 1| 0/X| 0/3|  10| 1
	//X    = (Y + 8 - Y % 8) + 3

	// Secure header = 0x0C|HEAD_1-3|0x80|0x00|0x00|0x00
	memset(aSecureHeaderPadd, 0, sizeof(aSecureHeaderPadd));
	memcpy_s(aSecureHeaderPadd, sizeof(aSecureHeaderPadd), ucpHeader, 4);
	aSecureHeaderPadd[0] = 0x0C;
	aSecureHeaderPadd[4] = 0x80;

	// DO87 = 0x87|LC+1|0x01|tDES(DATA_PADD)
	iDo87Len = 0;
	if(ucLc != 0){
		if (iIsOld)
		{
			memset(aDataPadd, 0, sizeof(aDataPadd));
			memcpy_s(aDataPadd, sizeof(aDataPadd), ucpData, ucLc);
			iDataPaddLen = ucLc + 8 - ucLc % 8;
			aDataPadd[ucLc] = 0x80;

			aDo87[0] = 0x87;
			aDo87[1] = iDataPaddLen + 1;
			aDo87[2] = 0x01;

			iDo87Len = sizeof(aDo87) - 3;
			memset(aDo87 + 3, 0, iDo87Len);

			pSBIO->lReturn = getCrypt(NID_des_ede_cbc, CODER_CRYPT_ENCRYPT, opSICAO->aKeyEnc, sizeof(opSICAO->aKeyEnc), aIv, sizeof(aIv), aDataPadd, iDataPaddLen, aDo87 + 3, &iDo87Len, 0);
			iDo87Len += 3;
		}
		else
		{
			BUF_MEM* bmDataToPad = BUF_MEM_create_init(ucpData, ucLc);
			BUF_MEM* bmDataPadded = EAC_add_iso_pad(pcd_ctx, bmDataToPad);
			if (bmDataPadded != NULL)
			{
#ifdef _DEBUG
				PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - EAC_add_iso_pad [%ld]", bmDataPadded->length); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bmDataPadded->data, bmDataPadded->length);
#endif
				memset(aDataPadd, 0, sizeof(aDataPadd));
				memcpy_s(aDataPadd, sizeof(aDataPadd), bmDataPadded->data, bmDataPadded->length);
				iDataPaddLen = bmDataPadded->length;

				memset(aDo87, 0, sizeof(aDo87));
				int iDo87Offset = 0;
				if (aSecureHeaderPadd[1] == 0xB1 || aSecureHeaderPadd[1] == 0xCB || aSecureHeaderPadd[1] == 0x21)
				{
					aDo87[0] = 0x85;
					if (iDataPaddLen > 255)
					{
						PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Tag 0x85 DataPadLen > 255 [%ld]", iDataPaddLen);
						aDo87[1] = 0x82;
						aDo87[2] = iDataPaddLen >> 8;
						aDo87[3] = iDataPaddLen & 0xFF;
						iDo87Offset = 4;
					}
					else if (iDataPaddLen > 127)
					{
						aDo87[1] = 0x81;
						aDo87[2] = iDataPaddLen;
						iDo87Offset = 3;
					}
					else
					{
						aDo87[1] = iDataPaddLen;
						iDo87Offset = 2;
					}
				}
				else
				{
					aDo87[0] = 0x87;
					if (iDataPaddLen > 255)
					{
						PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Tag 0x87 DataPadLen > 255 [%ld]", iDataPaddLen);
						aDo87[1] = 0x82;
						aDo87[2] = iDataPaddLen >> 8;
						aDo87[3] = iDataPaddLen & 0xFF;
						aDo87[4] = 0x01;
						iDo87Offset = 5;
					}
					else if (iDataPaddLen > 127)
					{
						aDo87[1] = 0x81;
						aDo87[2] = iDataPaddLen + 1;
						aDo87[3] = 0x01;
						iDo87Offset = 4;
					}
					else
					{
						aDo87[1] = iDataPaddLen + 1;
						aDo87[2] = 0x01;
						iDo87Offset = 3;
					}
				}

				BUF_MEM* bmDataIn = BUF_MEM_create_init(aDataPadd, iDataPaddLen);
#ifdef _DEBUG
				PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - EAC_encrypt Data To Encript [%ld]", iDataPaddLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aDataPadd, iDataPaddLen);
				PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - EAC_encrypt [%ld]", bmDataIn->length);
#endif
				BUF_MEM* bmDataOut = EAC_encrypt(pcd_ctx, bmDataIn);
				if (bmDataOut != NULL)
				{
					memcpy_s(aDo87 + iDo87Offset, sizeof(aDo87) - iDo87Offset, bmDataOut->data, bmDataOut->length);
					iDo87Len = bmDataOut->length + iDo87Offset;
					pSBIO->lReturn = 0;
#ifdef _DEBUG
					PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - aDo87 [%ld]", iDo87Len); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aDo87, iDo87Len);
#endif
				}
				else
				{
					PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - EAC_encrypt NULL");
					pSBIO->lReturn = CODER_ERROR_CRYPT_DECRYPTPAD;
				}

				if (bmDataIn)
					BUF_MEM_free(bmDataIn);
				if (bmDataOut)
					BUF_MEM_free(bmDataOut);
			}
			else
			{
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - EAC_add_iso_pad NULL");
				pSBIO->lReturn = CODER_ERROR_CRYPT_DECRYPTPAD;
			}
			
			// Free BUF MEM
			if (bmDataToPad)
				BUF_MEM_free(bmDataToPad);
			if (bmDataPadded)
				BUF_MEM_free(bmDataPadded);
		}

		if(pSBIO->lReturn != 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDU_DO87_ENC;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - transmitSecureApdu - ERROR Do87 Data Enc [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_transmitSecureApdu;
		}
	}
	// DO97 = 0x97|0x01|LE
	if(ucLe != 0){
		aDo97[0] = 0x97;
		aDo97[1] = 0x01;
		aDo97[2] = ucLe;
	}

	// DO8E = 0x8E|0x08|MAC(SSC|HEAD_PADD|DO87|DO97|PADDING)
	if (iIsOld)
	{
		memset(aDo8EDataPadd, 0, sizeof(aDo8EDataPadd));
		iDo8EDataPaddLen = 0;
		memcpy_s(aDo8EDataPadd + iDo8EDataPaddLen, sizeof(aDo8EDataPadd) - iDo8EDataPaddLen, opSICAO->aSessCount, sizeof(opSICAO->aSessCount));
		iDo8EDataPaddLen += sizeof(opSICAO->aSessCount);
		memcpy_s(aDo8EDataPadd + iDo8EDataPaddLen, sizeof(aDo8EDataPadd) - iDo8EDataPaddLen, aSecureHeaderPadd, sizeof(aSecureHeaderPadd));
		iDo8EDataPaddLen += sizeof(aSecureHeaderPadd);
		if (ucLc != 0)
		{
			memcpy_s(aDo8EDataPadd + iDo8EDataPaddLen, sizeof(aDo8EDataPadd) - iDo8EDataPaddLen, aDo87, iDo87Len);
			iDo8EDataPaddLen += iDo87Len;
		}
		if (ucLe != 0)
		{
			memcpy_s(aDo8EDataPadd + iDo8EDataPaddLen, sizeof(aDo8EDataPadd) - iDo8EDataPaddLen, aDo97, 3);
			iDo8EDataPaddLen += 3;
		}
		aDo8EDataPadd[iDo8EDataPaddLen] = 0x80;
		iDo8EDataPaddLen += 8 - iDo8EDataPaddLen % 8;
		iDo8ELen = 8;
		memset(aDo8E, 0, sizeof(aDo8E));

		if (getMac(opSICAO, aDo8EDataPadd, iDo8EDataPaddLen, aDo8E + 2, &iDo8ELen) != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDU_DO8E_MAC;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR Do8E Data Mac [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_transmitSecureApdu;
		}
	}
	else
	{
		unsigned char aDo8797[300];
		int iPos = 0;
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - aSecureHeaderPadd [%ld]", 4); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aSecureHeaderPadd, 4);
#endif
		BUF_MEM* bmHeaderToPad = BUF_MEM_create_init(aSecureHeaderPadd, 4);
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - bmHeaderToPad [%ld]", bmHeaderToPad->length); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bmHeaderToPad->data, bmHeaderToPad->length);
#endif
		BUF_MEM* bmHeaderPadded = EAC_add_iso_pad(pcd_ctx, bmHeaderToPad);
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - bmHeaderPadded [%ld]", bmHeaderPadded->length); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bmHeaderPadded->data, bmHeaderPadded->length);
#endif
		memcpy_s(aDo8797, sizeof(aDo8797), bmHeaderPadded->data, bmHeaderPadded->length);
		iPos += bmHeaderPadded->length;
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Header Padded [%ld]", iPos); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aDo8797, iPos);
#endif
		BUF_MEM* bmDataIn = NULL;
		memcpy_s(aDo8797 + iPos, sizeof(aDo8797) - iPos, aDo87, iDo87Len);
		iPos += iDo87Len;
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Header+87 Padded [%ld]", iPos); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aDo8797, iPos);
#endif
		if (ucLe != 0)
		{
			memcpy_s(aDo8797 + iPos, sizeof(aDo8797) - iPos, aDo97, 3);
			iPos += 3;
			bmDataIn = BUF_MEM_create_init(aDo8797, iPos);
		}
		else
		{
			bmDataIn = BUF_MEM_create_init(aDo8797, iPos);
		}
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Header+87+97 Padded [%ld]", iPos); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aDo8797, iPos);
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Header+87+97 Padded BUFF [%ld]", bmDataIn->length); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bmDataIn->data, bmDataIn->length);
#endif
		BUF_MEM* bmDataPadded = EAC_add_iso_pad(pcd_ctx, bmDataIn);
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Header+87+97 Padded BUFF Padded [%ld]", bmDataPadded->length); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bmDataPadded->data, bmDataPadded->length);
#endif
		BUF_MEM* bnMac = EAC_authenticate(pcd_ctx, bmDataPadded);
		if (bnMac != NULL)
		{
			memcpy_s(aDo8E + 2, sizeof(aDo8E) - 2, bnMac->data, 8);
			iDo8ELen = 8;
		}
		else
		{
			pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDU_DO8E_MAC;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR Do8E Data Mac [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			// Free all BUFF_MEM
			if (bmHeaderToPad)
				BUF_MEM_free(bmHeaderToPad);
			if (bmHeaderPadded)
				BUF_MEM_free(bmHeaderPadded);
			if (bmDataIn)
				BUF_MEM_free(bmDataIn);
			if (bmDataPadded)
				BUF_MEM_free(bmDataPadded);
			if (bnMac)
				BUF_MEM_free(bnMac);
			goto JMP_transmitSecureApdu;
		}
		// Free all BUFF_MEM
		if (bmHeaderToPad)
			BUF_MEM_free(bmHeaderToPad);
		if (bmHeaderPadded)
			BUF_MEM_free(bmHeaderPadded);
		if (bmDataIn)
			BUF_MEM_free(bmDataIn);
		if (bmDataPadded)
			BUF_MEM_free(bmDataPadded);
		if (bnMac)
			BUF_MEM_free(bnMac);
	}
	
	aDo8E[0] = 0x8E;
	aDo8E[1] = 0x08;
	iDo8ELen += 2;

	//APDU_DATA = DO87|DO97|DO8E
	iApduDataLen = 0;
	memset(aApduData, 0, sizeof(aApduData));
	if(ucLc != 0)
	{
		memcpy_s(aApduData, sizeof(aApduData), aDo87, iDo87Len);
		iApduDataLen += iDo87Len;
	}
	if(ucLe != 0)
	{
		memcpy_s(aApduData + iApduDataLen, sizeof(aApduData) - iApduDataLen, aDo97, 3);
		iApduDataLen += 3;
	}
	memcpy_s(aApduData + iApduDataLen, sizeof(aApduData) - iApduDataLen, aDo8E, iDo8ELen);
	iApduDataLen += iDo8ELen;

	iTmpApduRspLen = 0;
	//if (ucLe != 0)
	//{
	//	iTmpApduRspLen = 3 + ucLe + 8 - (ucLe % 8);
	//}
	//iTmpApduRspLen += 4;
	//iTmpApduRspLen += 10;

#ifdef _DEBUG
	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Data To Tx [%ld]", 4); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aSecureHeaderPadd, 4);
	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Data To Tx [%ld]", iApduDataLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduData, iApduDataLen);
#endif

	pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aSecureHeaderPadd, iApduDataLen, aApduData, (unsigned char)iTmpApduRspLen, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
	if(pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
	{
		pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDU;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - transmitSecureApdu - ERROR Transmit [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_transmitSecureApdu;
	}

	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Apdu Resp [%02X][%02X]", ucSw1, ucSw2);
#ifdef _DEBUG
	PutInLog(pLogger, LOG_LEVEL_DEBUG, (char *)"SolemBio - SecTransmitApdu Le:%d RspLen:%d Dif:%d", ucLe, iApduResponseLen, iApduResponseLen - ucLe); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduResponse, iApduResponseLen);
#endif

	// Increment session counter
	if (iIsOld)
		increment8bitCounter(opSICAO->aSessCount);
	else
	{
		if (EAC_increment_ssc(pcd_ctx) == 0)
		{
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR EAC_increment_ssc");
			pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDU_DO87_ENC;
			goto JMP_transmitSecureApdu;
		}
	}

	if(iApduResponseLen == 0)
	{
		*ucpSw1 = ucSw1;
		*ucpSw2 = ucSw2;
	}

	// APDU_RESPONSE = DO87|DO99|DO8E|SW1|SW2
	// APDU_RESPONSE =  0/X|   4|DO8E|SW1|SW2

	// DO87 = 0x87|LC+1|0x01|tDES(DATA_PADD)
	if(aApduResponse[0] == 0x87 || aApduResponse[0] == 0x85)
	{
		if (aApduResponse[1] == 0x81)
		{
			iDo87Len = aApduResponse[2] + 3;
			iDo87Offset = 4;
		}
		else
		{
			iDo87Len = aApduResponse[1] + 2;
			iDo87Offset = 3;
		}
		if (aApduResponse[0] == 0x85)
			iDo87Offset--;

		memset(aDo87, 0, sizeof(aDo87));
		memcpy_s(aDo87, sizeof(aDo87), aApduResponse, iDo87Len);
	}
	else
	{
		iDo87Len = 0;
	}

	// DO99 = 0x99|0x02|SW1|SW2
	if(aApduResponse[iDo87Len] == 0x99)
	{
		memcpy_s(aDo99, sizeof(aDo99), aApduResponse + iDo87Len, 4);
	}
	else
	{
		pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDURESP_DO99;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - transmitSecureApdu - ERROR Do99 Missed [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_transmitSecureApdu;
	}

	// DO8E = 0x8E|0x08|MAC(SSC|DO87|DO99|PADDING)
	if(aApduResponse[iDo87Len + 4] == 0x8E)
	{
		memcpy_s(aDo8E, sizeof(aDo8E), aApduResponse + iDo87Len + 4, 10);
	}
	else
	{
		pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDURESP_DO8E;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - transmitSecureApdu - ERROR Do8E Missed [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_transmitSecureApdu;
	}

	if (iIsOld)
	{
		// DO8E = 0x8E|0x08|MAC(SSC|DO87|DO99|PADDING)
		//MAC(SSC|DO87|DO99|PADDING)
		memset(aDo8EDataPadd, 0, sizeof(aDo8EDataPadd));
		iDo8EDataPaddLen = 0;
		//	SSC
		memcpy_s(aDo8EDataPadd + iDo8EDataPaddLen, sizeof(aDo8EDataPadd) - iDo8EDataPaddLen, opSICAO->aSessCount, sizeof(opSICAO->aSessCount));
		iDo8EDataPaddLen += sizeof(opSICAO->aSessCount);
		//	DO87
		if (iDo87Len > 0)
		{
			memcpy_s(aDo8EDataPadd + iDo8EDataPaddLen, sizeof(aDo8EDataPadd) - iDo8EDataPaddLen, aDo87, iDo87Len);
			iDo8EDataPaddLen += iDo87Len;
		}
		//	DO99
		memcpy_s(aDo8EDataPadd + iDo8EDataPaddLen, sizeof(aDo8EDataPadd) - iDo8EDataPaddLen, aDo99, sizeof(aDo99));
		iDo8EDataPaddLen += sizeof(aDo99);
		//	PADD
		aDo8EDataPadd[iDo8EDataPaddLen] = 0x80;
		iDo8EDataPaddLen += 8 - iDo8EDataPaddLen % 8;
		//	MAC
		iDo8ELen = 8;
		memset(aDo8EMac, 0, sizeof(aDo8EMac));

		if (getMac(opSICAO, aDo8EDataPadd, iDo8EDataPaddLen, aDo8EMac, &iDo8ELen) != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDURESP_DO8E_MAC;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR Do8E Data Mac [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_transmitSecureApdu;
		}
		//	VERIFY
		pSBIO->lReturn = memcmp(aDo8EMac, aDo8E + 2, 8);
		if (pSBIO->lReturn != 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDURESP_DO8E_MAC_MISMATCH;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR Do8E Data Mac Mismatch [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_transmitSecureApdu;
		}
	}
	else
	{
		// Data to Drecytp
		BUF_MEM* bmDataIn = BUF_MEM_create_init(aApduResponse, iApduResponseLen - 10);
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Data To Verify [%ld]", iApduDataLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bmDataIn->data, bmDataIn->length);
#endif
		BUF_MEM* bmDataPadded = EAC_add_iso_pad(pcd_ctx, bmDataIn);
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Data To Verify Padded [%ld]", bmDataPadded->length); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bmDataPadded->data, bmDataPadded->length);
#endif
		// Mac from response
		BUF_MEM* bnMac = BUF_MEM_create_init(aDo8E + 2, 8);
		//	VERIFY
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - MAC To Verify [%ld]", iApduDataLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bnMac->data, bnMac->length);
#endif
		if (EAC_verify_authentication(pcd_ctx, bmDataPadded, bnMac) == 0)
		{
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR EAC_verify_authentication");
			pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDU_DO87_ENC;
			// Free all BUFF_MEM
			if (bmDataIn)
				BUF_MEM_free(bmDataIn);
			if (bmDataPadded)
				BUF_MEM_free(bmDataPadded);
			if (bnMac)
				BUF_MEM_free(bnMac);
			goto JMP_transmitSecureApdu;
		}
		// Free all BUFF_MEM
		if (bmDataIn)
			BUF_MEM_free(bmDataIn);
		if (bmDataPadded)
			BUF_MEM_free(bmDataPadded);
		if (bnMac)
			BUF_MEM_free(bnMac);
	}

	if(iDo87Len > 0)
	{
		iDataPaddLen = sizeof(aDataPadd);
		memset(aDataPadd, 0, iDataPaddLen);
		
		if (iIsOld)
		{
			pSBIO->lReturn = getCrypt(NID_des_ede_cbc, CODER_CRYPT_DECRYPT, opSICAO->aKeyEnc, sizeof(opSICAO->aKeyEnc), aIv, sizeof(aIv), aDo87 + iDo87Offset, iDo87Len - iDo87Offset, aDataPadd, &iDataPaddLen, 0);
			if (pSBIO->lReturn != 0)
			{
				pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDURESP_DO87_DENC;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR Do87 Resp Data Denc [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
				goto JMP_transmitSecureApdu;
			}
		}
		else
		{
			BUF_MEM* bmDataIn = BUF_MEM_create_init(aDo87 + iDo87Offset, iDo87Len - iDo87Offset);
#ifdef _DEBUG
			PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Data To Decrypt [%ld]", bmDataIn->length); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bmDataIn->data, bmDataIn->length);
#endif
			BUF_MEM* bmDataOut = EAC_decrypt(pcd_ctx, bmDataIn);
			if (bmDataOut != NULL)
			{
#ifdef _DEBUG
				PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - transmitSecureApdu - Data Decrypted [%ld]", bmDataOut->length); DisplayHex(pLogger, LOG_LEVEL_DEBUG, bmDataOut->data, bmDataOut->length);
#endif
				memcpy_s(aDataPadd, sizeof(aDataPadd), bmDataOut->data, bmDataOut->length);
				iDataPaddLen = bmDataOut->length;
				pSBIO->lReturn = 0;
			}
			else
			{
				pSBIO->lError = SOLEMICAO_ERROR_TRANSMIT_SECURE_APDURESP_DO87_DENC;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - transmitSecureApdu - ERROR EAC_decrypt");
				// Free all BUFF_MEM
				if (bmDataIn)
					BUF_MEM_free(bmDataIn);
				if (bmDataOut)
					BUF_MEM_free(bmDataOut);
				goto JMP_transmitSecureApdu;
			}
			// Free all BUFF_MEM
			if (bmDataIn)
				BUF_MEM_free(bmDataIn);
			if (bmDataOut)
				BUF_MEM_free(bmDataOut);
		}

		while(iDataPaddLen > 0)
		{
			if(aDataPadd[iDataPaddLen] == 0x00 && aDataPadd[iDataPaddLen-1] == 0x00)
			{
				iDataPaddLen--;
			}
			else if(aDataPadd[iDataPaddLen] == 0x00 && aDataPadd[iDataPaddLen-1] == 0x80)
			{
				iDataPaddLen--;
				break;
			}
			else if(aDataPadd[iDataPaddLen] == 0x80)
			{
				break;
			}
		}

		if(iDataPaddLen > 0)
		{
			memcpy_s(ucpApduRsp, *ipApduRspLen, aDataPadd, iDataPaddLen);
			*ipApduRspLen = iDataPaddLen;
		}

	}

	*ucpSw1 = aDo99[2];
	*ucpSw2 = aDo99[3];

	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - transmitSecureApdu - OUT");
JMP_transmitSecureApdu:

	return pSBIO->lError;
}

static int authenticate(stSolemICAOPtr opSICAO, void *pDev, unsigned char *ucpChallenge, int iChallengeLen)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;

int					iTmp;
unsigned char		aTmpBuff[256];
int					iTmpBuffLen = 256;

unsigned char		aIv[8];
unsigned char		aKseed[20];
int					iKseedLen;

unsigned char		aApduHeader[4];
unsigned char		aApduData[255];
int					iApduDataLen = 255;
//	unsigned char		aApduMsg[255 + 6];
//	int					iApduMsgLen = 255 + 6;
unsigned char		ucSw1;
unsigned char		ucSw2;
unsigned char		aApduResponse[256 + 2];
int					iApduResponseLen = 256 + 2;

unsigned char		aLocalChallenge[8];
unsigned char		aSCardChallenge[8];
unsigned char		aLocalRand[16];
unsigned char		aSCardRand[16];

unsigned char		aAuthDataPlain[40];
int					iAuthDataPlainLen;
unsigned char		aAuthDataEnc[40];
int					iAuthDataEncLen;
unsigned char		aAuthDataMac[8];
int					iAuthDataMacLen;

unsigned char		aRespDataEnc[40];
int					iRespDataEncLen;
unsigned char		aRespDataMac[8];
int					iRespDataMacLen;
unsigned char		aRespDataPlain[40];
int					iRespDataPlainLen;

//unsigned char		ucTmp;
//int					iTmp;

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - authenticate - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	memset(aIv, 0, sizeof(aIv));

	//	Kseed
	iKseedLen = sizeof(aKseed);
	memset(aTmpBuff, 0, iTmpBuffLen);
	pSBIO->lReturn = getHash(NID_sha1, (unsigned char*)opSICAO->sICAOKey, strlen(opSICAO->sICAOKey), aKseed, &iKseedLen, 0);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_KSEED_HASH;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR get Kseed Hash [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	aKseed[16] = 0x00;
	aKseed[17] = 0x00;
	aKseed[18] = 0x00;
	//	Kenc
	aKseed[19] = 0x01;
	iTmpBuffLen = sizeof(aTmpBuff);
	memset(aTmpBuff, 0, iTmpBuffLen);
	pSBIO->lReturn = getHash(NID_sha1, aKseed, iKseedLen, aTmpBuff, &iTmpBuffLen, 0);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_KENC_HASH;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR get Kenc Hash [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	memcpy_s(opSICAO->aKeyEnc, sizeof(opSICAO->aKeyEnc), aTmpBuff, 16);
	for(iTmp = 0; iTmp < 16; iTmp++) opSICAO->aKeyEnc[iTmp] = parity(opSICAO->aKeyEnc[iTmp]);

	//	Kmac
	aKseed[19] = 0x02;
	iTmpBuffLen = sizeof(aTmpBuff);
	memset(aTmpBuff, 0, iTmpBuffLen);
	pSBIO->lReturn = getHash(NID_sha1, aKseed, iKseedLen, aTmpBuff, &iTmpBuffLen, 0);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_KMAC_HASH;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR get Kmac Hash [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	memcpy_s(opSICAO->aKeyMac, sizeof(opSICAO->aKeyMac), aTmpBuff, 16);
	for(iTmp = 0; iTmp < 16; iTmp++) opSICAO->aKeyMac[iTmp] = parity(opSICAO->aKeyMac[iTmp]);

	//	Local challenge RANDIFD
	pSBIO->lReturn = RAND_bytes(aLocalChallenge, sizeof(aLocalChallenge));
	if(pSBIO->lReturn != 1)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_LOCAL_CHALLENGE;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR get Local Challege [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	
	// SCard challenge RANDICC
	memcpy_s(aSCardChallenge, sizeof(aSCardChallenge), ucpChallenge, iChallengeLen);

	// Local random key seed KIFD
	pSBIO->lReturn = RAND_bytes(aLocalRand, sizeof(aLocalRand));
	if(pSBIO->lReturn != 1)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_LOCAL_RAND;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR get Local Challege [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	// Auth data plain S
	memset(aAuthDataPlain, 0, sizeof(aAuthDataPlain));
	memcpy_s(aAuthDataPlain, sizeof(aAuthDataPlain), aLocalChallenge, 8);
	memcpy_s(aAuthDataPlain + 8,sizeof(aAuthDataPlain) - 8, aSCardChallenge, 8);
	memcpy_s(aAuthDataPlain + 16,sizeof(aAuthDataPlain) - 16, aLocalRand, 16);
	iAuthDataPlainLen = 8 + 8 + 16;
	//	Auth data enc EIFD
	iAuthDataEncLen = sizeof(aAuthDataEnc);
	memset(aAuthDataEnc, 0, iAuthDataEncLen);
	pSBIO->lReturn = getCrypt(NID_des_ede_cbc, CODER_CRYPT_ENCRYPT, opSICAO->aKeyEnc, sizeof(opSICAO->aKeyEnc), aIv, sizeof(aIv), aAuthDataPlain, iAuthDataPlainLen, aAuthDataEnc, &iAuthDataEncLen, 0);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_AUTH_DATA_ENC;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR Auth Data Enc [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	
	//	Auth data mac MIFD
	memset(aAuthDataEnc + iAuthDataEncLen, 0, 8);
	aAuthDataEnc[iAuthDataEncLen] = 0x80;
	iAuthDataEncLen += 8;
	iAuthDataMacLen = 8;
	if(getMac(opSICAO, aAuthDataEnc, iAuthDataEncLen, aAuthDataMac, &iAuthDataMacLen) != SOLEMICAO_ERROR_NO_ERROR)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_AUTH_DATA_MAC;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR Auth Data Mac [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}

	// cmd EXTERNAL AUTHENTICATE 0x00, 0x82, 0x00, 0x00
	aApduHeader[0] = 0x00;
	aApduHeader[1] = 0x82;
	aApduHeader[2] = 0x00;
	aApduHeader[3] = 0x00;
	// DATA: EIFD|MIFD
	iApduDataLen = 40;
	memcpy_s(aApduData, sizeof(aApduData), aAuthDataEnc, 32);
	memcpy_s(aApduData + 32, sizeof(aApduData) - 32, aAuthDataMac, 8);

	iApduResponseLen = sizeof(aApduResponse);
	memset(aApduResponse, 0, iApduResponseLen);
	pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 40, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
	if(pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR External Authenticate [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	if(ucSw1 != 0x90 || ucSw2 != 0x00)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_RESP;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR External Authenticate Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
		goto JMP_authenticate;
	}
	if(iApduResponseLen != 40)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_RESP_LEN;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR External Authenticate Resp Len [%ld][%ld]", iApduResponseLen, pSBIO->lError);
		goto JMP_authenticate;
	}

	memset(aRespDataEnc, 0, sizeof(aRespDataEnc));
	memcpy_s(aRespDataEnc, sizeof(aRespDataEnc), aApduResponse, 32);
	
	// Calculate and verify Mac
	aRespDataEnc[32] = 0x80;
	iRespDataMacLen = sizeof(aRespDataMac);
	memset(aRespDataMac, 0, iRespDataMacLen);
	if(getMac(opSICAO, aRespDataEnc, sizeof(aRespDataEnc), aRespDataMac, &iRespDataMacLen) != SOLEMICAO_ERROR_NO_ERROR)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_RESP_DATA_MAC;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR Resp Data Mac [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}

	pSBIO->lReturn = memcmp(aRespDataMac, aApduResponse + 32, 8);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_RESP_MAC_VERIFICATION;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR Resp Mac Verification [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}

	// Decrypt response
	iRespDataEncLen = 32;
	iRespDataPlainLen = sizeof(aRespDataPlain);
	memset(aRespDataPlain, 0, iRespDataPlainLen);
	pSBIO->lReturn = getCrypt(NID_des_ede_cbc, CODER_CRYPT_DECRYPT, opSICAO->aKeyEnc, sizeof(opSICAO->aKeyEnc), aIv, sizeof(aIv), aRespDataEnc, iRespDataEncLen, aRespDataPlain, &iRespDataPlainLen, 0);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_RESP_DATA_ENC;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR Resp Data Enc [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}

	//Compare SmartCard challenge RANDICC
	pSBIO->lReturn = memcmp(aSCardChallenge, aRespDataPlain, 8);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_RESP_SCARD_CHALLENGE_VERIFICATION;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR Resp SmartCard Challenge Verification [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	//	Compare local challenge RANDIFD
	pSBIO->lReturn = memcmp(aLocalChallenge, aRespDataPlain + 8, 8);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_RESP_LOCAL_CHALLENGE_VERIFICATION;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR Resp Local Challenge Verification [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}

	//	Session count SSC
	memcpy_s(opSICAO->aSessCount,sizeof(opSICAO->aSessCount), aSCardChallenge + 4, 4);
	memcpy_s(opSICAO->aSessCount + 4,sizeof(opSICAO->aSessCount) - 4, aLocalChallenge + 4, 4);

	// SCard random key seed KICC
	memcpy_s(aSCardRand, sizeof(aSCardRand), aRespDataPlain + 16, 16);

	//	KsSeed
	memset(aKseed,0,sizeof(aKseed));
	//	for(iTmp = 0; iTmp < sizeof(aKseed); iTmp++)
	for(iTmp = 0; iTmp < sizeof(aKseed) && iTmp < sizeof(aSCardRand) && iTmp < sizeof(aLocalRand); iTmp++)
	{
		aKseed[iTmp] = aSCardRand[iTmp] ^ aLocalRand[iTmp];//aSCardRand XOR aLocalRand;
	}
	aKseed[16] = 0x00;
	aKseed[17] = 0x00;
	aKseed[18] = 0x00;
	//	Kenc
	aKseed[19] = 0x01;
	iTmpBuffLen = sizeof(aTmpBuff);
	memset(aTmpBuff, 0, iTmpBuffLen);
	pSBIO->lReturn = getHash(NID_sha1, aKseed, iKseedLen, aTmpBuff, &iTmpBuffLen, 0);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_KENC_HASH;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR get Kenc Hash [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	memcpy_s(opSICAO->aKeyEnc, sizeof(opSICAO->aKeyEnc), aTmpBuff, 16);
	for(iTmp = 0; iTmp < 16; iTmp++) opSICAO->aKeyEnc[iTmp] = parity(opSICAO->aKeyEnc[iTmp]);

	//	KSmac
	aKseed[19] = 2;
	iTmpBuffLen = sizeof(aTmpBuff);
	memset(aTmpBuff, 0, iTmpBuffLen);
	pSBIO->lReturn = getHash(NID_sha1, aKseed, iKseedLen, aTmpBuff, &iTmpBuffLen, 0);
	if(pSBIO->lReturn != 0)
	{
		pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_KMAC_HASH;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - authenticate - ERROR get Kmac Hash [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_authenticate;
	}
	memcpy_s(opSICAO->aKeyMac, sizeof(opSICAO->aKeyMac), aTmpBuff, 16);
	for(iTmp = 0; iTmp < 16; iTmp++) opSICAO->aKeyMac[iTmp] = parity(opSICAO->aKeyMac[iTmp]);

	// At this point it have all keys needed to establish
	// a secure communication with the smart card.

	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - authenticate - OUT");
JMP_authenticate:
	return pSBIO->lError;
}

static int updateICAOResults(stSolemICAOPtr opSICAO)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;
int					iTmp, iTmp1;
int					iDGxOffset;
unsigned char		aYoyqPatern[] = {0xff,0x4f,0xff,0x51}; // "ÿOÿQ"
unsigned char		aJP2Patern[] = { 0x00, 0x00, 0x00, 0x0C, 0x6A, 0x50, 0x20, 0x20}; // "....jP  "
char					sTmp[256];

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - updateICAOResults - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	for(iTmp = 0; iTmp < SOLEMBIO_DATAFIELD_MAX; iTmp++)
	{
		if(!opSICAO->aCaptureField[iTmp])
			continue;

      PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - updateICAOResults - Updating Item [%ld]", iTmp);

		switch(iTmp)
		{
		case SOLEMBIO_DATAFIELD_DOC_RUN:
			//SOLEMICAO_DATAGROUP_1
			iDGxOffset = 0;
			memset(sTmp, 0, sizeof(sTmp));
			while(opSICAO->aDoc_DG_1[iDGxOffset + 53] != '<')
			{
				sTmp[iDGxOffset] = opSICAO->aDoc_DG_1[iDGxOffset + 53];
				iDGxOffset++;
			}
			sTmp[iDGxOffset] = opSICAO->aDoc_DG_1[iDGxOffset + 54];

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_RUN, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_RUN;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData RUN [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_NAMES:
			//SOLEMICAO_DATAGROUP_1
			//SOLEMICAO_DATAGROUP_11
			memset(sTmp, 0, sizeof(sTmp));
			iDGxOffset = 0x41;
			iTmp1 = 0;
			while(opSICAO->aDoc_DG_1[iDGxOffset] != '<' || opSICAO->aDoc_DG_1[iDGxOffset - 1] != '<') iDGxOffset++;
			iDGxOffset++;
			while((opSICAO->aDoc_DG_1[iDGxOffset] != '<' || opSICAO->aDoc_DG_1[iDGxOffset - 1] != '<') && iDGxOffset < opSICAO->iDoc_DG_1Len)
			{
				if(opSICAO->aDoc_DG_1[iDGxOffset] != '<')
					sTmp[iTmp1++] = opSICAO->aDoc_DG_1[iDGxOffset];
				else
					sTmp[iTmp1++] = ' ';
				iDGxOffset++;
			}

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_NAMES, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_NAMES;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData NAMES [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_SURNAME_1: //SOLEMBIO_DATAFIELD_DOC_LASTNAME:
			//SOLEMICAO_DATAGROUP_1
			//SOLEMICAO_DATAGROUP_11
			memset(sTmp, 0, sizeof(sTmp));
			iDGxOffset = 0x41;
			iTmp1 = 0;
			while(opSICAO->aDoc_DG_1[iDGxOffset] != '<' || opSICAO->aDoc_DG_1[iDGxOffset + 1] != '<')
			{
				if(opSICAO->aDoc_DG_1[iDGxOffset] != '<')
					sTmp[iTmp1++] = opSICAO->aDoc_DG_1[iDGxOffset];
				else
					break;//sTmp[iTmp1++] = ' ';
				iDGxOffset++;
			}

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_SURNAME_1, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_SURNAME_PARENTAL;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData PATERNAL [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_SURNAME_2:
			//SOLEMICAO_DATAGROUP_1
			//SOLEMICAO_DATAGROUP_11
			memset(sTmp, 0, sizeof(sTmp));
			iDGxOffset = 0x41;
			iTmp1 = 0;
			while(opSICAO->aDoc_DG_1[iDGxOffset++] != '<');

			while(opSICAO->aDoc_DG_1[iDGxOffset] != '<' || opSICAO->aDoc_DG_1[iDGxOffset + 1] != '<')
			{
				if(opSICAO->aDoc_DG_1[iDGxOffset] != '<')
					sTmp[iTmp1++] = opSICAO->aDoc_DG_1[iDGxOffset];
				else
					break;//sTmp[iTmp1++] = ' ';
				iDGxOffset++;
			}

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_SURNAME_2, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_SURNAME_MATERNAL;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData MATERNAL [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_GENDER:
			//SOLEMICAO_DATAGROUP_1
			memset(sTmp, 0, sizeof(sTmp));
			
			sTmp[0] = opSICAO->aDoc_DG_1[42];
			
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_GENDER, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_GENDER;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData GENDER [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_NATIONALITY:
			//SOLEMICAO_DATAGROUP_1
			memset(sTmp, 0, sizeof(sTmp));
			memcpy_s(sTmp, sizeof(sTmp), opSICAO->aDoc_DG_1 + 50, 3);

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_NATIONALITY, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_NATIONALITY;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData NATIONALITY [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_BIRTHDATE:
			//SOLEMICAO_DATAGROUP_1
			memset(sTmp, 0, sizeof(sTmp));
			memcpy_s(sTmp, sizeof(sTmp), opSICAO->aDoc_DG_1 + 35, 6);

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_BIRTHDATE, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_BIRTHDATE;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData BIRTH [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_EXPIRYDATE:
			//SOLEMICAO_DATAGROUP_1
			memset(sTmp, 0, sizeof(sTmp));
			memcpy_s(sTmp, sizeof(sTmp), opSICAO->aDoc_DG_1 + 43, 6);

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_EXPIRYDATE, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_EXPIRYDATE;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData EXPIRY [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_MRZ:
			//SOLEMICAO_DATAGROUP_1
			memset(sTmp, 0, sizeof(sTmp));
			memcpy_s(sTmp, sizeof(sTmp), opSICAO->aDoc_DG_1 + 5, opSICAO->iDoc_DG_1Len - 5);

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_MRZ, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_MRZ;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData MRZ [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_SERIAL:
			//SOLEMICAO_DATAGROUP_1
			memset(sTmp, 0, sizeof(sTmp));
			memcpy_s(sTmp, sizeof(sTmp), opSICAO->aDoc_DG_1 + 10, 9);

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_SERIAL, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_SERIAL;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData SERIAL [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_COUNTRY_OF_ISSUE:
			//SOLEMICAO_DATAGROUP_1
			memset(sTmp, 0, sizeof(sTmp));
			memcpy_s(sTmp, sizeof(sTmp), opSICAO->aDoc_DG_1 + 7, 3);

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_COUNTRY_OF_ISSUE, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_COUNTRY_OF_ISSUE;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData COUNTRY [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		// TODO case SOLEMBIO_DATAFIELD_DOC_OFFICE_CODE:
		case SOLEMBIO_DATAFIELD_DOC_OCUPATION:
			//SOLEMICAO_DATAGROUP_11
			memset(sTmp, 0, sizeof(sTmp));
			
			iDGxOffset = 13;
			while (opSICAO->aDoc_DG_11[iDGxOffset - 3] != 0x5F || opSICAO->aDoc_DG_11[iDGxOffset - 2] != 0x13)
			{
				iDGxOffset += opSICAO->aDoc_DG_11[iDGxOffset - 1] + 3;
				if (iDGxOffset > opSICAO->aDoc_DG_11[1])
				{
					// error
					iDGxOffset = 0;
					PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - updateICAOResults - WARN PutData OCUPATION Not Found");
					break;
				}
			}
			
			if (iDGxOffset != 0)
			{
				memcpy_s(sTmp, sizeof(sTmp), opSICAO->aDoc_DG_11 + iDGxOffset, opSICAO->aDoc_DG_11[iDGxOffset - 1]);

				pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_OCUPATION, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
				if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
				{
					//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_OCUPATION;
					PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - updateICAOResults - ERROR PutData OCUPATION [%d][%d]", pSBIO->lReturn, pSBIO->lError);
					//goto JMP_updateICAOResults;
				}
			}
			else
			{
				// Try with tag 5F18. The new document 2022 comes with this tag
				iDGxOffset = 13;
				while (opSICAO->aDoc_DG_11[iDGxOffset - 3] != 0x5F || opSICAO->aDoc_DG_11[iDGxOffset - 2] != 0x18)
				{
					iDGxOffset += opSICAO->aDoc_DG_11[iDGxOffset - 1] + 3;
					if (iDGxOffset > opSICAO->aDoc_DG_11[1])
					{
						// error
						iDGxOffset = 0;
						PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - updateICAOResults - WARN PutData OCUPATION Not Found in second tag");
						break;
					}
				}

				if (iDGxOffset != 0)
				{
					memcpy_s(sTmp, sizeof(sTmp), opSICAO->aDoc_DG_11 + iDGxOffset, opSICAO->aDoc_DG_11[iDGxOffset - 1]);

					pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_OCUPATION, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
					if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
					{
						//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_OCUPATION;
						PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - updateICAOResults - ERROR PutData OCUPATION (second tag) [%d][%d]", pSBIO->lReturn, pSBIO->lError);
						//goto JMP_updateICAOResults;
					}
				}
				else
				{
					PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - updateICAOResults - WARN PutData OCUPATION - Setting Default Ocupation [%s][%d]", DEFAULT_ICAO_OPUPATION, strlen(DEFAULT_ICAO_OPUPATION));
					memcpy_s(sTmp, sizeof(sTmp), DEFAULT_ICAO_OPUPATION, strlen(DEFAULT_ICAO_OPUPATION));

					pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_OCUPATION, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
					if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
					{
						//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_OCUPATION;
						PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - updateICAOResults - ERROR PutData OCUPATION (second tag) [%d][%d]", pSBIO->lReturn, pSBIO->lError);
						//goto JMP_updateICAOResults;
					}
				}
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_BIRTHPLACE:
			//SOLEMICAO_DATAGROUP_11
			//SOLEMICAO_DATAGROUP_11
			memset(sTmp, 0, sizeof(sTmp));

			iDGxOffset = 13;
			while (opSICAO->aDoc_DG_11[iDGxOffset - 3] != 0x5F || opSICAO->aDoc_DG_11[iDGxOffset - 2] != 0x11)
			{
				iDGxOffset += opSICAO->aDoc_DG_11[iDGxOffset-1] + 3;
				if (iDGxOffset > opSICAO->aDoc_DG_11[1])
				{
					// error
					iDGxOffset = 0;
				}
			}

			memcpy_s(sTmp, sizeof(sTmp), opSICAO->aDoc_DG_11 + iDGxOffset, opSICAO->aDoc_DG_11[iDGxOffset-1]);

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_BIRTHPLACE, SOLEMBIO_DATAFIELDPROP_NONE, sTmp, strlen(sTmp));
			if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_BIRTHPLACE;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData BIRTHPLACE [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_ISSUEDATE:
			//SOLEMICAO_DATAGROUP_12

			iDGxOffset = opSICAO->iDoc_DG_12Len - 8;

			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_ISSUEDATE, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_12 + iDGxOffset, 8);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_ISSUEDATE;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData ISSUEDATE [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}

			break;
		case SOLEMBIO_DATAFIELD_DOC_FACE:
			//SOLEMICAO_DATAGROUP_2
			// TODO identificar inicio de foto segun estructura "Face Template" de NIST
			iDGxOffset = 0;
			//while(memcmp(aYoyqPatern, opSICAO->aDoc_DG_2 + iDGxOffset, sizeof(aYoyqPatern))) 
			while (memcmp(aJP2Patern, opSICAO->aDoc_DG_2 + iDGxOffset, sizeof(aJP2Patern)))
				iDGxOffset++;

			// Before put new jp2 image, claen all images formats of SOLEMBIO_DATAFIELD_DOC_FACE
			pSBIO->lReturn = sBioCleanData(pSBIO, SOLEMBIO_DATAFIELD_DOC_FACE, SOLEMBIO_DATAFIELDPROP_NONE);
			if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - updateICAOResults - ERROR PutData FACE [%d][%d]", pSBIO->lReturn, pSBIO->lError);
			}
			else
			{
				pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_FACE, SOLEMBIO_DATAFIELDPROP_IMAGEFORMAT_SOLJP2, opSICAO->aDoc_DG_2 + iDGxOffset, opSICAO->iDoc_DG_2Len - iDGxOffset);
				if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
				{
					//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_FACE;
					PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - updateICAOResults - ERROR PutData FACE [%d][%d]", pSBIO->lReturn, pSBIO->lError);
					//goto JMP_updateICAOResults;
				}
			}
			
			break;
		case SOLEMBIO_DATAFIELD_DOC_SIGNATURE:
			//SOLEMICAO_DATAGROUP_7
			// TODO identificar inicio de firma segun estructura "Face Template" de NIST
			iDGxOffset = 0;
			//while(memcmp(aYoyqPatern, opSICAO->aDoc_DG_7 + iDGxOffset, sizeof(aYoyqPatern))) 
			while (memcmp(aJP2Patern, opSICAO->aDoc_DG_7 + iDGxOffset, sizeof(aJP2Patern)))
				iDGxOffset++;

			// Before put new jp2 image, claen all images formats of SOLEMBIO_DATAFIELD_DOC_SIGNATURE
			pSBIO->lReturn = sBioCleanData(pSBIO, SOLEMBIO_DATAFIELD_DOC_SIGNATURE, SOLEMBIO_DATAFIELDPROP_NONE);
			if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - updateICAOResults - ERROR PutData FACE [%d][%d]", pSBIO->lReturn, pSBIO->lError);
			}
			else
			{
				pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_SIGNATURE, SOLEMBIO_DATAFIELDPROP_IMAGEFORMAT_SOLJP2, opSICAO->aDoc_DG_7 + iDGxOffset, opSICAO->iDoc_DG_7Len - iDGxOffset);
				if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
				{
					//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DOC_SIGNATURE;
					PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - updateICAOResults - ERROR PutData SIGNATURE [%d][%d]", pSBIO->lReturn, pSBIO->lError);
					//goto JMP_updateICAOResults;
				}
			}
			
			break;
		case SOLEMBIO_DATAFIELD_DOC_FINGERID_1:
		case SOLEMBIO_DATAFIELD_DOC_FINGERID_2:
		case SOLEMBIO_DATAFIELD_DOC_FINGERID_1_TRIES_LEFT:
		case SOLEMBIO_DATAFIELD_DOC_FINGERID_2_TRIES_LEFT:
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_COM:
			//SOLEMICAO_DATAGROUP_COM
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_COM, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_COM, opSICAO->iDoc_DG_COMLen);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_COM;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DGCOM [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_1:
			//SOLEMICAO_DATAGROUP_1
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_1, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_1, opSICAO->iDoc_DG_1Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_1;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG1 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_2:
			//SOLEMICAO_DATAGROUP_2
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_2, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_2, opSICAO->iDoc_DG_2Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_2;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG2 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_3:
			//SOLEMICAO_DATAGROUP_3
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_3, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_3, opSICAO->iDoc_DG_3Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_3;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG3 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_4:
			//SOLEMICAO_DATAGROUP_4
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_4, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_4, opSICAO->iDoc_DG_4Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_4;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG4 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_5:
			//SOLEMICAO_DATAGROUP_5
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_5, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_5, opSICAO->iDoc_DG_5Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_5;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG5 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_6:
			//SOLEMICAO_DATAGROUP_6
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_6, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_6, opSICAO->iDoc_DG_6Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_6;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG6 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_7:
			//SOLEMICAO_DATAGROUP_7
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_7, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_7, opSICAO->iDoc_DG_7Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_7;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG7 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_8:
			//SOLEMICAO_DATAGROUP_8
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_8, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_8, opSICAO->iDoc_DG_8Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_8;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG8 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_9:
			//SOLEMICAO_DATAGROUP_9
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_9, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_9, opSICAO->iDoc_DG_9Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_9;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG9 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_10:
			//SOLEMICAO_DATAGROUP_10
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_10, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_10, opSICAO->iDoc_DG_10Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_10;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG10 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_11:
			//SOLEMICAO_DATAGROUP_11
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_11, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_11, opSICAO->iDoc_DG_11Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_11;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG11 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_12:
			//SOLEMICAO_DATAGROUP_12
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_12, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_12, opSICAO->iDoc_DG_12Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_12;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG12 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_13:
			//SOLEMICAO_DATAGROUP_13
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_13, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_13, opSICAO->iDoc_DG_13Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_13;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG13 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_14:
			//SOLEMICAO_DATAGROUP_14
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_14, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_14, opSICAO->iDoc_DG_14Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_14;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG14 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		case SOLEMBIO_DATAFIELD_DOC_DG_15:
			//SOLEMICAO_DATAGROUP_15
			pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_DG_15, SOLEMBIO_DATAFIELDPROP_NONE, opSICAO->aDoc_DG_15, opSICAO->iDoc_DG_15Len);
			if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
			{
				//pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_PUTDATA_DG_15;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR PutData DG15 [%d][%d]", pSBIO->lReturn, pSBIO->lError);
				//goto JMP_updateICAOResults;
			}
			break;
		default:
			pSBIO->lError = SOLEMICAO_ERROR_UPDATEICAORESULTS_DATAFIELD_INALID;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - updateICAOResults - ERROR DataField Invalid [%d]", iTmp);
			goto JMP_updateICAOResults;
		}
	}

	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char *)"SolemICAO - updateICAOResults - OUT");
JMP_updateICAOResults:
	return pSBIO->lError;
}


long ConnectCard(stSolemICAOPtr opSICAO, void* pDev, int appICAO)
{
	stSolemBioPtr		pSBIO = NULL;
	stLoggerPtr			pLogger = NULL;

	unsigned char		aApduHeader[4];
	unsigned char		aApduData[255];
	int					iApduDataLen = 255;
	unsigned char		aApduResponse[256 + 2];
	int					iApduResponseLen = 256 + 2;
	unsigned char		ucSw1;
	unsigned char		ucSw2;

	unsigned char		aDataTmp1[255];
	int					iDataTmp1Len = 255;
	unsigned char		aDataTmp2[255];
	int					iDataTmp2Len = 255;
	unsigned char		aTag[1];

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;
	iIsOld = 0;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - ConnectCard - IN");

	


	BIO* bio = NULL;
	PACE_SEC* secret = NULL;
	BUF_MEM* enc_nonce = NULL, * pcd_mapping_data = NULL,
		* picc_mapping_data = NULL, * pcd_ephemeral_pubkey = NULL,
		* picc_ephemeral_pubkey = NULL, * pcd_token = NULL,
		* picc_token = NULL;

	unsigned char EF_CARDACCESS[256];
	int iCardAccessLen = sizeof(EF_CARDACCESS);

	if (appICAO)
	{
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - ConnectCard - Trying New Chip");
		// Select ID Instance in 2023 Chip
		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));
		// cmd SELECT 00A4040C
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xA4;
		aApduHeader[2] = 0x04;
		aApduHeader[3] = 0x0C;
		// DATA: INST_ID 	A0000000770308700701FE0000040001
		iApduDataLen = 16;
		aApduData[0] = 0xA0;
		aApduData[1] = 0x00;
		aApduData[2] = 0x00;
		aApduData[3] = 0x00;
		aApduData[4] = 0x77;
		aApduData[5] = 0x03;
		aApduData[6] = 0x08;
		aApduData[7] = 0x70;
		aApduData[8] = 0x07;
		aApduData[9] = 0x01;
		aApduData[10] = 0xFE;
		aApduData[11] = 0x00;
		aApduData[12] = 0x00;
		aApduData[13] = 0x04;
		aApduData[14] = 0x00;
		aApduData[15] = 0x01;
		iApduResponseLen = sizeof(aApduResponse);


		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select ID Instance in New Chip [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}

		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR_RESP;
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - ConnectCard - ERROR Select ID Instance in New Chip Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - ConnectCard - Trying Old Chip");
			// Select ID Instance in 2012 Chip
			memset(aApduHeader, 0, sizeof(aApduHeader));
			memset(aApduData, 0, sizeof(aApduData));
			// cmd SELECT 00A4040C
			aApduHeader[0] = 0x00;
			aApduHeader[1] = 0xA4;
			aApduHeader[2] = 0x04;
			aApduHeader[3] = 0x0C;
			// DATA: INST_ID 	0xA0000002471001
			iApduDataLen = 7;
			aApduData[0] = 0xA0;
			aApduData[1] = 0x00;
			aApduData[2] = 0x00;
			aApduData[3] = 0x02;
			aApduData[4] = 0x47;
			aApduData[5] = 0x10;
			aApduData[6] = 0x01;

			iApduResponseLen = sizeof(aApduResponse);
			pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select ID Instance in Old Chip [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
				goto JMP_connectcard;
			}
			if (ucSw1 != 0x90 || ucSw2 != 0x00)
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR_RESP;
				PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - ERROR Select ID Instance in Old Chip Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
				goto JMP_connectcard;
			}
			iIsOld = 1;
		}
	}
	else
	{
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - ConnectCard - Trying New Chip");
		// Select ID Instance in 2023 Chip
		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));
		// cmd SELECT 0x00, 0xA4, 0x02, 0x0C
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xA4;
		aApduHeader[2] = 0x04;
		aApduHeader[3] = 0x0C;
		// DATA: INST_ID 	0xA000000077030C60000000FE00000500
		iApduDataLen = 16;
		aApduData[0] = 0xA0;
		aApduData[1] = 0x00;
		aApduData[2] = 0x00;
		aApduData[3] = 0x00;
		aApduData[4] = 0x77;
		aApduData[5] = 0x03;
		aApduData[6] = 0x0C;
		aApduData[7] = 0x60;
		aApduData[8] = 0x00;
		aApduData[9] = 0x00;
		aApduData[10] = 0x00;
		aApduData[11] = 0xFE;
		aApduData[12] = 0x00;
		aApduData[13] = 0x00;
		aApduData[14] = 0x05;
		aApduData[15] = 0x00;
		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select ID Instance in New Chip [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR_RESP;
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - ConnectCard - ERROR Select ID Instance in New Chip Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - ConnectCard - Trying Old Chip");
			// Select ID Instance in 2013 Chip
			memset(aApduHeader, 0, sizeof(aApduHeader));
			memset(aApduData, 0, sizeof(aApduData));
			// cmd SELECT 0x00, 0xA4, 0x02, 0x0C
			aApduHeader[0] = 0x00;
			aApduHeader[1] = 0xA4;
			aApduHeader[2] = 0x04;
			aApduHeader[3] = 0x0C;
			// DATA: INST_ID 	0xE828BD080FD25043686C43432D654944
			iApduDataLen = 16;
			aApduData[0] = 0xE8;
			aApduData[1] = 0x28;
			aApduData[2] = 0xBD;
			aApduData[3] = 0x08;
			aApduData[4] = 0x0F;
			aApduData[5] = 0xD2;
			aApduData[6] = 0x50;
			aApduData[7] = 0x43;
			aApduData[8] = 0x68;
			aApduData[9] = 0x6C;
			aApduData[10] = 0x43;
			aApduData[11] = 0x43;
			aApduData[12] = 0x2D;
			aApduData[13] = 0x65;
			aApduData[14] = 0x49;
			aApduData[15] = 0x44;
			iApduResponseLen = sizeof(aApduResponse);
			pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select ID Instance in Old Chip [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
				goto JMP_connectcard;
			}
			if (ucSw1 != 0x90 || ucSw2 != 0x00)
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR_RESP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select ID Instance in Old Chip Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
				goto JMP_connectcard;
			}
			iIsOld = 1;
		}
	}

	if (iIsOld)
	{
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Select Master File for 2012 Chip");
		// Select Master File for 2012 Chip
		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xA4;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x0C;
		// DATA: INST_ID 	0x3F00
		iApduDataLen = 2;
		aApduData[0] = 0x3F;
		aApduData[1] = 0x00;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select Master File for 2012 Chip [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_SELECT_DIR_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select Master File for 2012 Chip Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_connectcard;
		}
	}

	// SELECT DIR
	memset(aApduHeader, 0, sizeof(aApduHeader));
	memset(aApduData, 0, sizeof(aApduData));
	// cmd SELECT 0x00, 0xA4, 0x02, 0x0C
	aApduHeader[0] = 0x00;
	aApduHeader[1] = 0xA4;
	aApduHeader[2] = 0x02;
	aApduHeader[3] = 0x0C;
	// DATA: FID DIR 	0x2F, 0x00
	iApduDataLen = 2;
	aApduData[0] = 0x2F;
	aApduData[1] = 0x00;

	iApduResponseLen = sizeof(aApduResponse);
	pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
	if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
	{
		PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - WARN Select App [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
	}

	if (ucSw1 != 0x90 || ucSw2 != 0x00)
	{
		PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - WARN Select App Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
	}
	else
	{
		// READBINARY on DIR
		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));
		// cmd SELECT 0x00, 0xB0, 0x00, 0x00
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xB0;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x00;
		// DATA: FID DIR 	0x2F, 0x00
		iApduDataLen = 0;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - WARN Read Binary on Dir [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - WARN Read Binary on Dir Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
		}
		else
		{
			// Decode Dir, not used for now
		}
	}

	// SELECT CARDACCESS
	memset(aApduHeader, 0, sizeof(aApduHeader));
	memset(aApduData, 0, sizeof(aApduData));
	// cmd SELECT 0x00, 0xA4, 0x02, 0x0C
	aApduHeader[0] = 0x00;
	aApduHeader[1] = 0xA4;
	aApduHeader[2] = 0x02;
	aApduHeader[3] = 0x0C;
	// DATA: FID CARDACCESS 	0x01 , 0x1C
	iApduDataLen = 2;
	aApduData[0] = 0x01;
	aApduData[1] = 0x1C;
	iApduResponseLen = sizeof(aApduResponse);
	pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
	if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
	{
		PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - WARN Select Card Access [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
	}
	if (ucSw1 != 0x90 || ucSw2 != 0x00)
	{
		PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - WARN Select Card Access Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
	}
	else
	{
		// READBINARY on DIR
		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));
		// cmd SELECT 0x00, 0xB0, 0x00, 0x00
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xB0;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x00;
		// DATA: FID DIR 	0x2F, 0x00
		iApduDataLen = 0;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - ERROR Read Binary on Dir CA [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - ERROR Read Binary on Dir CA Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
		}
		else
		{
			// Decode CardAccessData
#ifdef _DEBUG
			PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - OK Read Binary on Dir CA [%ld]", iApduResponseLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduResponse, iApduResponseLen);
#endif
			memcpy_s(EF_CARDACCESS, iCardAccessLen, aApduResponse, iApduResponseLen);
			iCardAccessLen = iApduResponseLen;
#ifdef _DEBUG
			PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - CardAccess [%ld]", iCardAccessLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, EF_CARDACCESS, iCardAccessLen);
#endif
		}
	}

	if (iIsOld == 0)
	{
		// PACE

		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"PASEEEEEEEEEEEEEEEEE 1!!!!!!");
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"PASEEEEEEEEEEEEEEEEE 1!!!!!!");
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"PASEEEEEEEEEEEEEEEEE 1!!!!!!");
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"PASEEEEEEEEEEEEEEEEE 1!!!!!!");

		//	Update ICAO key
		/////////////////////////////////////////////////////////////////////////
		// RAFA RAFA RAFA RAFA
		//if (udateICAOkey(opSICAO) != SOLEMICAO_ERROR_NO_ERROR)
		//{
		//	PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Update ICAO key [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		//	goto JMP_connectcard;
		//}
		/////////////////////////////////////////////////////////////////////////
		// RAFA RAFA RAFA RAFA


		//PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"POST Update ICAO key");
		//return 0;



#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - IcaoKey [%s]", opSICAO->sICAOKey);
#endif

		unsigned char		aKseed[24];
		int					iKseedLen;
		//	Kseed
		memset(aKseed, 0, sizeof(aKseed));
		iKseedLen = sizeof(aKseed);
		pSBIO->lReturn = getHash(NID_sha1, (unsigned char*)opSICAO->sICAOKey, strlen(opSICAO->sICAOKey), aKseed, &iKseedLen, 0);
		if (pSBIO->lReturn != 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_KSEED_HASH;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR get Kseed Hash [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Len Kseed Hash [%ld]", iKseedLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aKseed, iKseedLen);
#endif

		unsigned char		aKM[32];
		int					iKMLen;
		//	KM
		iKMLen = sizeof(aKM);
		aKseed[sizeof(aKseed) - 1] = 3;	// PACE
		pSBIO->lReturn = getHash(NID_sha256, (unsigned char*)aKseed, iKseedLen + 4, aKM, &iKMLen, 0);
		if (pSBIO->lReturn != 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_AUTHENTICATE_KSEED_HASH;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR get KM Hash [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Len KM Hash [%ld]", iKMLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aKM, iKMLen);
#endif

		// Prepare Security Enviroment Data
		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0x22;
		aApduHeader[2] = 0xC1;
		aApduHeader[3] = 0xA4;

		aDataTmp1[0] = 0x04;	// OID
		aDataTmp1[1] = 0x00;
		aDataTmp1[2] = 0x7F;
		aDataTmp1[3] = 0x00;
		aDataTmp1[4] = 0x07;
		aDataTmp1[5] = 0x02;
		aDataTmp1[6] = 0x02;
		aDataTmp1[7] = 0x04;
		aDataTmp1[8] = 0x02;
		aDataTmp1[9] = 0x04;

		aTag[0] = 0x80;
		int wrapLen1 = wrap(aTag, 1, aDataTmp1, 10, aApduData, sizeof(aApduData));
		if (wrapLen1 > 0)
		{
			aDataTmp1[0] = 0x01;	// Type MRZ
			aTag[0] = 0x83;
			int wrapLen2 = wrap(aTag, 1, aDataTmp1, 1, aApduData + wrapLen1, sizeof(aApduData) - wrapLen1);
			if (wrapLen2 > 0)
			{
				iApduDataLen = wrapLen2 + wrapLen1;
			}
			else
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_WRAP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x83");
				goto JMP_connectcard;
			}
		}
		else
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_WRAP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x80");
			goto JMP_connectcard;
		}

		// Request Secure Environemnt MSE
		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Secure Environemnt MSE [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Secure Environemnt MSE Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_connectcard;
		}


		// Get Doc MRZ from QR
		const char sMrz[91];
		memset((void *)sMrz, 0, sizeof(sMrz));
		memset((void *)sMrz, '<', sizeof(sMrz) - 1);
		memcpy_s((void *)(sMrz + 5), sizeof(sMrz) - 5, opSICAO->sICAOKey, 10);
		memcpy_s((void *)(sMrz + 30), sizeof(sMrz) - 30, opSICAO->sICAOKey + 10, 7);
		memcpy_s((void *)(sMrz + 38), sizeof(sMrz) - 38, opSICAO->sICAOKey + 17, 7);


		secret = PACE_SEC_new(sMrz, strlen(sMrz), PACE_MRZ);
		if (secret == NULL)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_PACE_SEC_NEW;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR PACE_SEC_new");
			goto JMP_connectcard;
		}

		if (pcd_ctx != NULL)
		{
			EAC_CTX_clear_free(pcd_ctx);
			pcd_ctx = NULL;
		}
		pcd_ctx = EAC_CTX_new();
		if (EAC_CTX_init_ef_cardaccess(EF_CARDACCESS, iCardAccessLen, pcd_ctx) == 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_EAC_INIT_CTX;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR init_ef_cardaccess");
			goto JMP_connectcard;
		}

		// Prepare Step 1 General Authentication Negotiation Data
		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));

		aApduData[0] = 0x7C;	// tag
		aApduData[1] = 0x00;	// 0

		iApduDataLen = 2;

		aApduHeader[0] = 0x10;
		aApduHeader[1] = 0x86;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x00;

		// Request Step 1 General Authentication Negotiation
		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Step 1 General Authentication Negotiation [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Step 1 General Authentication Negotiation Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_connectcard;
		}
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Resp Step 1 General Authentication Negotiation [%ld]", iApduResponseLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduResponse, iApduResponseLen);
#endif

		unsigned char uTmp[32];
		memcpy_s(uTmp, sizeof(uTmp), aApduResponse + 4, iApduResponseLen - 4);

		enc_nonce = BUF_MEM_create_init(uTmp, sizeof(uTmp));

		if (PACE_STEP2_dec_nonce(pcd_ctx, secret, enc_nonce) == 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_PACE_STEP2;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR PACE_STEP2_dec_nonce");
			goto JMP_connectcard;
		}

		// Prepare Step 2 General Authentication Negotiation Data
		pcd_mapping_data = PACE_STEP3A_generate_mapping_data(pcd_ctx);
		if (pcd_mapping_data == NULL)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_PACE_STEP3A;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR generate_mapping_data");
			goto JMP_connectcard;
		}

		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - OK generate_mapping_data len [%ld]", pcd_mapping_data->length);

		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));

		aApduData[0] = 0x7C;	// tag
		aApduData[1] = (unsigned char)pcd_mapping_data->length + 2;	// Len 
		aApduData[2] = 0x81;	// OID
		aApduData[3] = (unsigned char)pcd_mapping_data->length;	// Len mapping
		memcpy_s(aApduData + 4, sizeof(aApduData) - 4, pcd_mapping_data->data, pcd_mapping_data->length);

		iApduDataLen = pcd_mapping_data->length + 4;

		aApduHeader[0] = 0x10;
		aApduHeader[1] = 0x86;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x00;

		// Request Step 2 General Authentication Negotiation
		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Step 2 General Authentication Negotiation [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Step 2 General Authentication Negotiation Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_connectcard;
		}
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Resp Step 2 General Authentication Negotiation [%ld]", iApduResponseLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduResponse, iApduResponseLen);
#endif

		aTag[0] = 0x7C;
		iDataTmp1Len = unWrap(aTag, 1, aApduResponse, iApduResponseLen, aDataTmp1, sizeof(aDataTmp1));
		if (iDataTmp1Len > 0)
		{
			aTag[0] = 0x82;
			iDataTmp2Len = unWrap(aTag, 1, aDataTmp1, iDataTmp1Len, aDataTmp2, sizeof(aDataTmp2));
			if (iDataTmp2Len > 0)
			{
#ifdef _DEBUG
				PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Data Unwrap Step 2 [%ld]", iDataTmp2Len); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aDataTmp2, iDataTmp2Len);
#endif
			}
			else
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_UNWRAP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error unwrapping 0x82 Step 2");
				goto JMP_connectcard;
			}
		}
		else
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_UNWRAP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error unwrapping 0x7C Step 2");
			goto JMP_connectcard;
		}

		// Prepare Step 3 General Authentication Negotiation Data
		picc_mapping_data = BUF_MEM_create_init(aDataTmp2, iDataTmp2Len);
		if (PACE_STEP3A_map_generator(pcd_ctx, picc_mapping_data) == 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_PACE_STEP3A_MAP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR PACE_STEP3A_map_generator");
			goto JMP_connectcard;
		}


		pcd_ephemeral_pubkey = PACE_STEP3B_generate_ephemeral_key(pcd_ctx);
		if (pcd_ephemeral_pubkey == NULL)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_PACE_STEP3B;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR PACE_STEP3B_generate_ephemeral_key");
			goto JMP_connectcard;
		}

		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));
		aApduHeader[0] = 0x10;
		aApduHeader[1] = 0x86;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x00;

		aTag[0] = 0x83;
		iDataTmp1Len = wrap(aTag, 1, pcd_ephemeral_pubkey->data, pcd_ephemeral_pubkey->length, aDataTmp1, sizeof(aDataTmp1));
		if (iDataTmp1Len > 0)
		{
			aTag[0] = 0x7C;
			iApduDataLen = wrap(aTag, 1, aDataTmp1, iDataTmp1Len, aApduData, sizeof(aApduData));
			if (iApduDataLen <= 0)
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_WRAP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x7C");
				goto JMP_connectcard;
			}
		}
		else
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_WRAP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x83");
			goto JMP_connectcard;
		}

		// Request Step 3 General Authentication Negotiation
		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Step 3 General Authentication Negotiation [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Step 3 General Authentication Negotiation Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_connectcard;
		}
#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Resp Step 3 General Authentication Negotiation [%ld]", iApduResponseLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduResponse, iApduResponseLen);
#endif

		aTag[0] = 0x7C;
		iDataTmp1Len = unWrap(aTag, 1, aApduResponse, iApduResponseLen, aDataTmp1, sizeof(aDataTmp1));
		if (iDataTmp1Len > 0)
		{
			aTag[0] = 0x84;
			iDataTmp2Len = unWrap(aTag, 1, aDataTmp1, iDataTmp1Len, aDataTmp2, sizeof(aDataTmp2));
			if (iDataTmp2Len > 0)
			{
#ifdef _DEBUG
				PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Data Unwrap Step 3 [%ld]", iDataTmp2Len); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aDataTmp2, iDataTmp2Len);
#endif
			}
			else
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_UNWRAP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error unwrapping 0x84 Step 3");
				goto JMP_connectcard;
			}
		}
		else
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_UNWRAP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error unwrapping 0x7C Step 3");
			goto JMP_connectcard;
		}

		picc_ephemeral_pubkey = BUF_MEM_create_init(aDataTmp2, iDataTmp2Len);
		if (PACE_STEP3B_compute_shared_secret(pcd_ctx, picc_ephemeral_pubkey) == 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_BUFF_MEM_INIT;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR PACE_STEP3B_compute_shared_secret");
			goto JMP_connectcard;
		}

		// Prepare Step 4
		if (PACE_STEP3C_derive_keys(pcd_ctx) == 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_PACE_STEP3C;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR PACE_STEP3B_compute_shared_secret");
			goto JMP_connectcard;
		}
		pcd_token = PACE_STEP3D_compute_authentication_token(pcd_ctx, picc_ephemeral_pubkey);
		if (pcd_token == NULL)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_PACE_STEP3D;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR PACE_STEP3D_compute_authentication_token");
			goto JMP_connectcard;
		}

#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Data pcd_token [%ld]", pcd_token->length); DisplayHex(pLogger, LOG_LEVEL_DEBUG, pcd_token->data, pcd_token->length);
#endif

		// Prepare Step 4 General Authentication Negotiation Data
		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0x86;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x00;

		aTag[0] = 0x85;
		iDataTmp1Len = wrap(aTag, 1, pcd_token->data, pcd_token->length, aDataTmp1, sizeof(aDataTmp1));
		if (iDataTmp1Len > 0)
		{
			aTag[0] = 0x7C;
			iApduDataLen = wrap(aTag, 1, aDataTmp1, iDataTmp1Len, aApduData, sizeof(aApduData));
			if (iApduDataLen <= 0)
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_WRAP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x7C step 4");
				goto JMP_connectcard;
			}
		}
		else
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_WRAP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x83 step 4");
			goto JMP_connectcard;
		}

#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Data Wrap pcd_token [%ld]", iApduDataLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduData, iApduDataLen);
#endif

		// Request Step 4 General Authentication Negotiation
		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Step 4 General Authentication Negotiation [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_READ_BIN_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Request Step 4 General Authentication Negotiation Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_connectcard;
		}

#ifdef _DEBUG
		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Resp Request Step 4 General Authentication Negotiation [%ld]", iApduResponseLen); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aApduResponse, iApduResponseLen);
#endif

		aTag[0] = 0x7C;
		iDataTmp1Len = unWrap(aTag, 1, aApduResponse, iApduResponseLen, aDataTmp1, sizeof(aDataTmp1));
		if (iDataTmp1Len > 0)
		{
			aTag[0] = 0x86;
			iDataTmp2Len = unWrap(aTag, 1, aDataTmp1, iDataTmp1Len, aDataTmp2, sizeof(aDataTmp2));
			if (iDataTmp2Len > 0)
			{
#ifdef _DEBUG
				PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - ConnectCard - Data Unwrap Step 4 [%ld]", iDataTmp2Len); DisplayHex(pLogger, LOG_LEVEL_DEBUG, aDataTmp2, iDataTmp2Len);
#endif
			}
			else
			{
				pSBIO->lError = SOLEMICAO_ERROR_CONNETC_UNWRAP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error unwrapping 0x86 Step 4");
				goto JMP_connectcard;
			}
		}
		else
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_UNWRAP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error unwrapping 0x7C Step 4");
			goto JMP_connectcard;
		}

		picc_token = BUF_MEM_create_init(aDataTmp2, iDataTmp2Len);
		if (PACE_STEP3D_verify_authentication_token(pcd_ctx, picc_token) != 1)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_PACE_STEP3D_VERIF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR PACE_STEP3D_verify_authentication_token");
			goto JMP_connectcard;
		}

		// Set encryption
		if (EAC_CTX_set_encryption_ctx(pcd_ctx, EAC_ID_PACE) == 0)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CONNETC_EAC_SET_CTX;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR EAC_CTX_set_encryption_ctx");
			goto JMP_connectcard;
		}

		if (appICAO)
		{
			// // Select ICAO Application 0x00A4040C
			aApduHeader[0] = 0x00;
			aApduHeader[1] = 0xA4;
			aApduHeader[2] = 0x04;
			aApduHeader[3] = 0x0C;
			// DATA: AID_ICAO 0xA0000002471001
			iApduDataLen = 7;
			aApduData[0] = 0xA0;
			aApduData[1] = 0x00;
			aApduData[2] = 0x00;
			aApduData[3] = 0x02;
			aApduData[4] = 0x47;
			aApduData[5] = 0x10;
			aApduData[6] = 0x01;

			iApduResponseLen = sizeof(aApduResponse);
			pSBIO->lReturn = transmitSecureApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_SELECT_APP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select ICAO App [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
				goto JMP_connectcard;
			}
			if (ucSw1 != 0x90 || ucSw2 != 0x00)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_SELECT_APP_RESP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select ICAO App Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
				goto JMP_connectcard;
			}
		}
	}

	if (appICAO && iIsOld)
	{
		//	Update ICAO key
		///////////////////////////////////////////////////////////////////////
		// RAFA RAFA RAFA 
		//if (udateICAOkey(opSICAO) != SOLEMICAO_ERROR_NO_ERROR)
		//{
		//	PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Update ICAO key [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		//	goto JMP_connectcard; poto
		//}
		///////////////////////////////////////////////////////////////////////
		// RAFA RAFA RAFA 


		// cmd SELECT 0x00, 0xA4, 0x04, 0x0C
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xA4;
		aApduHeader[2] = 0x04;
		aApduHeader[3] = 0x0C;
		// DATA: File ID (ICAO) 0xA0,0x00,0x00,0x02,0x47,0x10,0x01
		iApduDataLen = 7;
		aApduData[0] = 0xA0;
		aApduData[1] = 0x00;
		aApduData[2] = 0x00;
		aApduData[3] = 0x02;
		aApduData[4] = 0x47;
		aApduData[5] = 0x10;
		aApduData[6] = 0x01;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_SELECT_APP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select App [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_SELECT_APP_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Select App Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_connectcard;
		}

		if (iApduResponseLen > 0)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - ConnectCard - WARNING Select App Resp [%ld]", iApduResponseLen);
		}

		// cmd GET CHALLENGE 0x00, 0x84, 0x00, 0x00
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0x84;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x00;
		// NO DATA
		iApduDataLen = 0;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 8, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_GETCHALLENGE;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Get Challenge [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_GETCHALLENGE_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Get Challenge Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_connectcard;
		}
		if (iApduResponseLen != 8)
		{
			pSBIO->lReturn = iApduResponseLen;
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_GETCHALLENGE_RESP_LEN;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Get Challenge Resp Length [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}

		// Authenticate With ICAO SmartCard
		pSBIO->lReturn = authenticate(opSICAO, pDev, aApduResponse, iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_AUTHENTICATE;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - ERROR Authenticate [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_connectcard;
		}
	}

JMP_connectcard:

	// Clean EAC
	if (secret)
		PACE_SEC_clear_free(secret);
	if (enc_nonce)
		BUF_MEM_free(enc_nonce);
	if (pcd_mapping_data)
		BUF_MEM_free(pcd_mapping_data);
	if (picc_mapping_data)
		BUF_MEM_free(picc_mapping_data);
	if (pcd_ephemeral_pubkey)
		BUF_MEM_free(pcd_ephemeral_pubkey);
	if (picc_ephemeral_pubkey)
		BUF_MEM_free(picc_ephemeral_pubkey);
	if (pcd_token)
		BUF_MEM_free(pcd_token);
	if (picc_token)
		BUF_MEM_free(picc_token);

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - ConnectCard - OUT [%ld]", pSBIO->lError);

	return pSBIO->lError;
}

static int getIcao(stSolemICAOPtr opSICAO, void *pDev)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;

unsigned char		aApduHeader[4];
unsigned char		aApduData[255];
int					iApduDataLen = 255;
unsigned char		ucSw1;
unsigned char		ucSw2;
unsigned char		aApduResponse[256 + 2];
int					iApduResponseLen = 256 + 2;
//unsigned char		ucTmp;
int					iTmp;
int					iDGxSize;
int					iDGxLen;
unsigned char		*ucpDGxData = NULL;
int					*ipDGxOffset = NULL;
int					iAuthBugRetryCount = 0;
	
	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, "%s", (char*)"SolemICAO - getIcao - IN");

JMP_AuthBugRetry:

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	//mutex_lock(opSICAO->oMutex);

	if (!opSICAO->iGetICAO)
	{
		PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - getIcao - WARNING No ICAO Data Requested");
		goto JMP_getIcao;
	}

	// Connect Card
	if (ConnectCard(opSICAO, pDev, 1) != SOLEMICAO_ERROR_NO_ERROR)
	{
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - Capture - ERROR Connect to Card [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		// Check for ICAO Authentication errors:
		switch (pSBIO->lError)
		{
		case SOLEMICAO_ERROR_GETICAO_SELECT_APP:
		case SOLEMICAO_ERROR_GETICAO_SELECT_APP_RESP:
		case SOLEMICAO_ERROR_GETICAO_GETCHALLENGE:
		case SOLEMICAO_ERROR_GETICAO_GETCHALLENGE_RESP:
		case SOLEMICAO_ERROR_GETICAO_GETCHALLENGE_RESP_LEN:
		case SOLEMICAO_ERROR_GETICAO_AUTHENTICATE:
			if (iAuthBugRetryCount < ICAO_AUTH_BUG_RETRY)
			{
				iAuthBugRetryCount++;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR ICAO Auth Connect [%ld][%ld]... Retry [%ld]", pSBIO->lReturn, pSBIO->lError, iAuthBugRetryCount);
				goto JMP_AuthBugRetry;
			}
			break;
		default:
			break;
		}
		goto JMP_getIcao;
	}


	for (iTmp = 0; iTmp < SOLEMICAO_DATAGROUP_MAX; iTmp++)
	{
		if (!opSICAO->aCaptureDG[iTmp])
		{
			continue;
		}

		// cmd SELECT 0x00, 0xA4, 0x02, 0x0C
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xA4;
		aApduHeader[2] = 0x02;
		aApduHeader[3] = 0x0C;
		// DATA: DG Indx 0x01,0xXX
		iApduDataLen = 2;
		aApduData[0] = 0x01;
		//aApduData[1] = 0x1E;
		switch (iTmp)
		{
		case SOLEMICAO_DATAGROUP_COM:
			aApduData[1] = 0x1E;
			iDGxSize = sizeof(opSICAO->aDoc_DG_COM);
			ucpDGxData = opSICAO->aDoc_DG_COM;
			ipDGxOffset = &opSICAO->iDoc_DG_COMLen;
			break;
		case SOLEMICAO_DATAGROUP_1:
			aApduData[1] = 0x01;
			iDGxSize = sizeof(opSICAO->aDoc_DG_1);
			ucpDGxData = opSICAO->aDoc_DG_1;
			ipDGxOffset = &opSICAO->iDoc_DG_1Len;
			break;
		case SOLEMICAO_DATAGROUP_2:
			aApduData[1] = 0x02;
			iDGxSize = sizeof(opSICAO->aDoc_DG_2);
			ucpDGxData = opSICAO->aDoc_DG_2;
			ipDGxOffset = &opSICAO->iDoc_DG_2Len;
			break;
		case SOLEMICAO_DATAGROUP_3:
			aApduData[1] = 0x03;
			iDGxSize = sizeof(opSICAO->aDoc_DG_3);
			ucpDGxData = opSICAO->aDoc_DG_3;
			ipDGxOffset = &opSICAO->iDoc_DG_3Len;
			break;
		case SOLEMICAO_DATAGROUP_4:
			aApduData[1] = 0x04;
			iDGxSize = sizeof(opSICAO->aDoc_DG_4);
			ucpDGxData = opSICAO->aDoc_DG_4;
			ipDGxOffset = &opSICAO->iDoc_DG_4Len;
			break;
		case SOLEMICAO_DATAGROUP_5:
			aApduData[1] = 0x05;
			iDGxSize = sizeof(opSICAO->aDoc_DG_5);
			ucpDGxData = opSICAO->aDoc_DG_5;
			ipDGxOffset = &opSICAO->iDoc_DG_5Len;
			break;
		case SOLEMICAO_DATAGROUP_6:
			aApduData[1] = 0x06;
			iDGxSize = sizeof(opSICAO->aDoc_DG_6);
			ucpDGxData = opSICAO->aDoc_DG_6;
			ipDGxOffset = &opSICAO->iDoc_DG_6Len;
			break;
		case SOLEMICAO_DATAGROUP_7:
			aApduData[1] = 0x07;
			iDGxSize = sizeof(opSICAO->aDoc_DG_7);
			ucpDGxData = opSICAO->aDoc_DG_7;
			ipDGxOffset = &opSICAO->iDoc_DG_7Len;
			break;
		case SOLEMICAO_DATAGROUP_8:
			aApduData[1] = 0x08;
			iDGxSize = sizeof(opSICAO->aDoc_DG_8);
			ucpDGxData = opSICAO->aDoc_DG_8;
			ipDGxOffset = &opSICAO->iDoc_DG_8Len;
			break;
		case SOLEMICAO_DATAGROUP_9:
			aApduData[1] = 0x09;
			iDGxSize = sizeof(opSICAO->aDoc_DG_9);
			ucpDGxData = opSICAO->aDoc_DG_9;
			ipDGxOffset = &opSICAO->iDoc_DG_9Len;
			break;
		case SOLEMICAO_DATAGROUP_10:
			aApduData[1] = 0x0A;
			iDGxSize = sizeof(opSICAO->aDoc_DG_10);
			ucpDGxData = opSICAO->aDoc_DG_10;
			ipDGxOffset = &opSICAO->iDoc_DG_10Len;
			break;
		case SOLEMICAO_DATAGROUP_11:
			aApduData[1] = 0x0B;
			iDGxSize = sizeof(opSICAO->aDoc_DG_11);
			ucpDGxData = opSICAO->aDoc_DG_11;
			ipDGxOffset = &opSICAO->iDoc_DG_11Len;
			break;
		case SOLEMICAO_DATAGROUP_12:
			aApduData[1] = 0x0C;
			iDGxSize = sizeof(opSICAO->aDoc_DG_12);
			ucpDGxData = opSICAO->aDoc_DG_12;
			ipDGxOffset = &opSICAO->iDoc_DG_12Len;
			break;
		case SOLEMICAO_DATAGROUP_13:
			aApduData[1] = 0x0D;
			iDGxSize = sizeof(opSICAO->aDoc_DG_13);
			ucpDGxData = opSICAO->aDoc_DG_13;
			ipDGxOffset = &opSICAO->iDoc_DG_13Len;
			break;
		case SOLEMICAO_DATAGROUP_14:
			aApduData[1] = 0x0E;
			iDGxSize = sizeof(opSICAO->aDoc_DG_14);
			ucpDGxData = opSICAO->aDoc_DG_14;
			ipDGxOffset = &opSICAO->iDoc_DG_14Len;
			break;
		case SOLEMICAO_DATAGROUP_15:
			aApduData[1] = 0x0F;
			iDGxSize = sizeof(opSICAO->aDoc_DG_15);
			ucpDGxData = opSICAO->aDoc_DG_15;
			ipDGxOffset = &opSICAO->iDoc_DG_15Len;
			break;
		default:
			iDGxSize = 0;
			ucpDGxData = NULL;
			ipDGxOffset = NULL;
		}

		iApduResponseLen = sizeof(aApduResponse);
		memset(aApduResponse, 0, iApduResponseLen);
		pSBIO->lReturn = transmitSecureApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_SELECTDG;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Select DG [%ld][%ld][%ld]", iTmp, pSBIO->lReturn, pSBIO->lError);
			goto JMP_getIcao;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_SELECTDG_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Select DG Resp [%ld][%02x:%02x][%ld]", iTmp, ucSw1, ucSw2, pSBIO->lError);
			goto JMP_getIcao;
		}

		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - getIcao - Select DG OK [%ld][%02x:%02x][%ld]", iTmp, ucSw1, ucSw2, pSBIO->lError);

		// cmd Binary READ 0x00, 0xB0, 0x00, 0x00
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xB0;
		aApduHeader[2] = 0x00; //Off
		aApduHeader[3] = 0x00; //Set
		// NO DATA

		iApduResponseLen = sizeof(aApduResponse);
		memset(aApduResponse, 0, iApduResponseLen);
		pSBIO->lReturn = transmitSecureApdu(opSICAO, pDev, aApduHeader, 0, NULL, 4, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG [%ld][%ld][%ld]", iTmp, pSBIO->lReturn, pSBIO->lError);
			goto JMP_getIcao;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG Resp [%ld][%02x:%02x][%ld]", iTmp, ucSw1, ucSw2, pSBIO->lError);
			goto JMP_getIcao;
		}
		if (iApduResponseLen != 4)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG_RESP_LEN;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG Resp Length (init) [%ld][%ld][%ld]", iTmp, iApduResponseLen, pSBIO->lError);
			goto JMP_getIcao;
		}

		PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - getIcao - Read DG OK [%ld][%02x:%02x][%ld]", iTmp, ucSw1, ucSw2, pSBIO->lError);

		if ((aApduResponse[1] & 0xF0) == 0x80)
		{
			iDGxLen = ((int)aApduResponse[2]);
			iDGxLen = iDGxLen * 256 + ((int)aApduResponse[3]) + 4;
		}
		else
		{
			iDGxLen = (int)aApduResponse[1] + 2;
		}

		if (iDGxLen > iDGxSize)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG_OVERFLOW;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG Length Too Large [%ld][%ld/%ld][%ld]", iTmp, iDGxLen, iDGxSize, pSBIO->lError);
			goto JMP_getIcao;
		}

		*ipDGxOffset = 0;
		memset(ucpDGxData, 0, iDGxSize);
		while (*ipDGxOffset + ICAO_READ_CHUNK <= iDGxLen)
		{
			//PutInLog(pLogger, LOG_LEVEL_WARNING, (char *)"SolemICAO - getIcao - IN WHILE Read DG [%ld][%ld][%ld]", *ipDGxOffset, *ipDGxOffset + ICAO_READ_CHUNK, iDGxLen);
			// cmd Binary READ 0x00, 0xB0, 0x00, 0x00
			aApduHeader[0] = 0x00;
			aApduHeader[1] = 0xB0;
			aApduHeader[2] = ((*ipDGxOffset >> 8) & 0xFF);	//0x00; //Off
			aApduHeader[3] = (*ipDGxOffset & 0xFF);			//0x00; //Set
			// NO DATA

			iApduResponseLen = sizeof(aApduResponse);
			memset(aApduResponse, 0, iApduResponseLen);
			pSBIO->lReturn = transmitSecureApdu(opSICAO, pDev, aApduHeader, 0, NULL, ICAO_READ_CHUNK, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG [%ld][%ld][%ld]", iTmp, pSBIO->lReturn, pSBIO->lError);
				goto JMP_getIcao;
			}
			if (ucSw1 != 0x90 || ucSw2 != 0x00)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG_RESP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG Resp [%ld][%02x:%02x][%ld]", iTmp, ucSw1, ucSw2, pSBIO->lError);
				goto JMP_getIcao;
			}
			if (iApduResponseLen != ICAO_READ_CHUNK)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG_RESP_LEN;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG Resp Length (chunk) [%ld][%ld/%ld][%ld]", iTmp, ICAO_READ_CHUNK, iApduResponseLen, pSBIO->lError);
				goto JMP_getIcao;
			}

			memcpy_s(ucpDGxData + *ipDGxOffset, iDGxSize - *ipDGxOffset, aApduResponse, iApduResponseLen);
			*ipDGxOffset += iApduResponseLen;

		}

		if (iDGxLen % ICAO_READ_CHUNK > 0)
		{
			//PutInLog(pLogger, LOG_LEVEL_WARNING, (char *)"SolemICAO - OUT WHILE Read DG [%ld][%ld][%ld]", *ipDGxOffset, *ipDGxOffset + ICAO_READ_CHUNK, iDGxLen);
			// cmd Binary READ 0x00, 0xB0, 0x00, 0x00
			aApduHeader[0] = 0x00;
			aApduHeader[1] = 0xB0;
			aApduHeader[2] = ((*ipDGxOffset >> 8) & 0xFF);	//0x00; //Off
			aApduHeader[3] = (*ipDGxOffset & 0xFF);			//0x00; //Set
			// NO DATA

			iApduResponseLen = sizeof(aApduResponse);
			memset(aApduResponse, 0, iApduResponseLen);
			pSBIO->lReturn = transmitSecureApdu(opSICAO, pDev, aApduHeader, 0, NULL, iDGxLen % ICAO_READ_CHUNK, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG [%ld][%ld][%ld]", iTmp, pSBIO->lReturn, pSBIO->lError);
				goto JMP_getIcao;
			}
			if (ucSw1 != 0x90 || ucSw2 != 0x00)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG_RESP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG Resp [%ld][%02x:%02x][%ld]", iTmp, ucSw1, ucSw2, pSBIO->lError);
				goto JMP_getIcao;
			}
			if (iApduResponseLen != iDGxLen % ICAO_READ_CHUNK)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG_RESP_LEN;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Read DG Resp Length (last chunk) [%ld][%ld/%ld][%ld]", iTmp, iApduResponseLen, iDGxLen % ICAO_READ_CHUNK, pSBIO->lError);
				goto JMP_getIcao;
			}

			memcpy_s(ucpDGxData + *ipDGxOffset, iDGxSize - *ipDGxOffset, aApduResponse, iApduResponseLen);
			*ipDGxOffset += iApduResponseLen;
		}
	}

	// cmd GET CHALLENGE 0x00, 0x84, 0x00, 0x00
	aApduHeader[0] = 0x00;
	aApduHeader[1] = 0x84;
	aApduHeader[2] = 0x00;
	aApduHeader[3] = 0x00;
	// NO DATA
	iApduDataLen = 0;

	iApduResponseLen = sizeof(aApduResponse);
	pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 8, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
	if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
	{
		//pSBIO->lError = SOLEMICAO_ERROR_GETICAO_GETCHALLENGE;
		//PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - getIcao - ERROR Get Challenge [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - getIcao - DEBUG Get Challenge [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		//goto JMP_getIcao;
	}
	if (ucSw1 != 0x90 || ucSw2 != 0x00)
	{
		//pSBIO->lError = SOLEMICAO_ERROR_GETICAO_GETCHALLENGE_RESP;
		//PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - getIcao - ERROR Get Challenge Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - getIcao - DEBUG Get Challenge Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
		//goto JMP_getIcao;
	}
	if (iApduResponseLen != 8)
	{
		pSBIO->lReturn = iApduResponseLen;
		//pSBIO->lError = SOLEMICAO_ERROR_GETICAO_GETCHALLENGE_RESP_LEN;
		//PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - getIcao - ERROR Get Challenge Resp Length [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - getIcao - DEBUG Get Challenge Resp Length [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		//goto JMP_getIcao;
	}

	pSBIO->lReturn = updateICAOResults(opSICAO);
	if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
	{
		pSBIO->lError = SOLEMICAO_ERROR_GETICAO_READDG;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getIcao - ERROR Update ICAO Results [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_getIcao;
	}

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - getIcao - OUT");

JMP_getIcao:
	//mutex_unlock(opSICAO->oMutex);

	return pSBIO->lError;
}

static int getFingerRef(stSolemICAOPtr opSICAO, void *pDev)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;

unsigned char		aApduHeader[4];
unsigned char		aApduData[255];
int					iApduDataLen = 255;
//	unsigned char		aApduMsg[255 + 6];
//	int					iApduMsgLen = 255 + 6;
unsigned char		ucSw1;
unsigned char		ucSw2;
unsigned char		aApduResponse[256 + 2];
int					iApduResponseLen = 256 + 2;

int					iIndx1, iIndx2, iIndx3;
unsigned char		ucTmp;
int					iTmp;
int					iFingerRef;

int					iFingerRef1 = SOLEMBIO_FINGER_NONE;
int					iFingerRef2 = SOLEMBIO_FINGER_NONE;

int					iFingerRef1TriesLeft = -1;
int					iFingerRef2TriesLeft = -1;

int					iNoFingerBugAttempts = 3;
	
	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - getFingerRef - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	//mutex_lock(opSICAO->oMutex);

	if(!opSICAO->iGetFinger)
	{
		PutInLog(pLogger, LOG_LEVEL_WARNING, (char *)"SolemICAO - getFingerRef - WARNING No Finger Requested");
		goto JMP_getFingerRef;
	}


	// Connect Card
	if (ConnectCard(opSICAO, pDev, 0) != SOLEMICAO_ERROR_NO_ERROR)
	{
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - Capture - ERROR Connect to Card [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_getFingerRef;
	}

JMP_NO_FINGER_BUG:

	if (iIsOld)
	{
		// cmd SELECT 0x00,0xA4,0x04,0x0C
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xA4;
		aApduHeader[2] = 0x04;
		aApduHeader[3] = 0x0C;
		// DATA: File ID (GET_FINGER) 0xE8,0x28,0xBD,0x08,0x0F,0xD2,0x50,0x43,0x68,0x6C,0x43,0x43,0x2D,0x65,0x49,0x44
		iApduDataLen = 16;
		aApduData[0] = 0xE8;
		aApduData[1] = 0x28;
		aApduData[2] = 0xBD;
		aApduData[3] = 0x08;
		aApduData[4] = 0x0F;
		aApduData[5] = 0xD2;
		aApduData[6] = 0x50;
		aApduData[7] = 0x43;
		aApduData[8] = 0x68;
		aApduData[9] = 0x6C;
		aApduData[10] = 0x43;
		aApduData[11] = 0x43;
		aApduData[12] = 0x2D;
		aApduData[13] = 0x65;
		aApduData[14] = 0x49;
		aApduData[15] = 0x44;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_SELECT_APP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR Select App [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_getFingerRef;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_SELECT_APP_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR Select App Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_getFingerRef;
		}

		if (iApduResponseLen > 0)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - getFingerRef - WARNING Select App Resp [%ld]", iApduResponseLen);
		}

		// {0x00, 0xCB, 0x3F, 0xFF, 0x0E, 0x4D, 0x0C, 0x70, 0x0A, 0xBF, 0x82, 0xXX, 0x06, 0x7F, 0x50, 0x03, 0x7F, 0x60, 0x80} 0xXX / 0x11 / 0x12
		// cmd INTERNAL(GET_FINGER) 0x00,0xCB,0x3F,0xFF
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xCB;
		aApduHeader[2] = 0x3F;
		aApduHeader[3] = 0xFF;
		// DATA: INTERNAL(GET_FINGER) 0x4D,0x0C,0x70,0x0A,0xBF,0x82,0xXX,0x06,0x7F,0x50,0x03,0x7F,0x60,0x80 /0xXX / 0x11 / 0x12
		iApduDataLen = 14;
		aApduData[0] = 0x4D;
		aApduData[1] = 0x0C;
		aApduData[2] = 0x70;
		aApduData[3] = 0x0A;
		aApduData[4] = 0xBF;
		aApduData[5] = 0x82;
		aApduData[6] = 0x11; //0x11//0x12
		aApduData[7] = 0x06;
		aApduData[8] = 0x7F;
		aApduData[9] = 0x50;
		aApduData[10] = 0x03;
		aApduData[11] = 0x7F;
		aApduData[12] = 0x60;
		aApduData[13] = 0x80;

		//if(opSICAO->aCaptureField[SOLEMBIO_DATAFIELD_DOC_FINGERID_1])
		for (iFingerRef = 1; iFingerRef <= 2; iFingerRef++)
		{
			switch (iFingerRef)
			{
			case 1:
				aApduData[6] = 0x11; //0x11//0x12
				break;
			case 2:
				aApduData[6] = 0x12; //0x11//0x12
				break;
			}

			iApduResponseLen = sizeof(aApduResponse);
			pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_GETDATA1;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR Get Finger %d [%ld][%ld]", iFingerRef, pSBIO->lReturn, pSBIO->lError);
				goto JMP_getFingerRef;
			}

			if (ucSw1 == 0x6a && ucSw2 == 0x88) //0x6A88 = referenced data not found
			{
				switch (iFingerRef)
				{
				case 1:
					iFingerRef1 = SOLEMBIO_FINGER_NONE;
					break;
				case 2:
					iFingerRef2 = SOLEMBIO_FINGER_NONE;
					break;
				}
				PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - getFingerRef - DEBUG Finger %d None", iFingerRef);
			}
			else if (ucSw1 != 0x90 && ucSw2 != 0x00)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_GETDATA1_RESP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR Get Finger %d Response [%02x][%02x][%ld][%ld]", iFingerRef, ucSw1, ucSw2, iApduResponseLen, pSBIO->lError);
				//goto JMP_getFingerRef;
			}
			else //if(ucSw1 == 0x90 && ucSw2 == 0x00)
			{
				for (iIndx1 = 0; iIndx1 < iApduResponseLen - 2; iIndx1++)
				{
					if (aApduResponse[iIndx1] == 0x7F && aApduResponse[iIndx1 + 1] == 0x60)
					{
						for (iIndx2 = iIndx1 + 3; iIndx2 < iApduResponseLen - 1; iIndx2++)
						{
							if (aApduResponse[iIndx2] == 0xA1)
							{
								for (iIndx3 = iIndx2 + 2; iIndx3 < iApduResponseLen - 1; iIndx3++)
								{
									if (aApduResponse[iIndx3] == 0x82)
									{
										ucTmp = aApduResponse[iIndx3 + 2];
										iTmp = (ucTmp >> 2) & 0x07;
										if ((ucTmp & 0x03) == 2)
											iTmp += 5;

										switch (iFingerRef)
										{
										case 1:
											iFingerRef1 = iTmp;
											break;
										case 2:
											iFingerRef2 = iTmp;
											break;
										}
										PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - getFingerRef - DEBUG Finger %d found [%ld]", iFingerRef, iTmp);
										//	SOLEMBIO_FINGER_RT = 1,	//5,
										//	SOLEMBIO_FINGER_RI = 2,	//9,
										//	SOLEMBIO_FINGER_RM = 3,	//13,
										//	SOLEMBIO_FINGER_RR = 4,	//17,
										//	SOLEMBIO_FINGER_RL = 5,	//21,
										//	SOLEMBIO_FINGER_LT = 6,	//6,
										//	SOLEMBIO_FINGER_LI = 7,	//10,
										//	SOLEMBIO_FINGER_LM = 8,	//14,
										//	SOLEMBIO_FINGER_LR = 9,	//18,
										//	SOLEMBIO_FINGER_LL = 10,//22,
										//	iFingerRef = aApduResponse[19];
									}
								}
							}
						}
					}
				}
			}
		}

		// {0x00, 0x21, 0x00, 0xXX} 0xXX / 0x91 / 0x92
		// cmd INTERNAL(VERIFY) 0x00,0x21,0x00,0x91
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0x21;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x91; //0x91//0x92
		for (iFingerRef = 1; iFingerRef <= 2; iFingerRef++)
		{
			switch (iFingerRef)
			{
			case 1:
				aApduHeader[3] = 0x91; //0x91//0x92
				break;
			case 2:
				aApduHeader[3] = 0x92; //0x91//0x92
				break;
			}

			iApduResponseLen = sizeof(aApduResponse);
			pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)0, NULL, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_GETDATA1;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR Get Finger %d [%ld][%ld]", iFingerRef, pSBIO->lReturn, pSBIO->lError);
				goto JMP_getFingerRef;
			}

			else if (ucSw1 != 0x63 || (ucSw2 & 0xF0) != 0xC0)
			{
				pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_GETDATA1_RESP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR Get Finger %d Response [%02x][%02x][%ld][%ld]", iFingerRef, ucSw1, ucSw2, iApduResponseLen, pSBIO->lError);
				//goto JMP_getFingerRef;
			}
			else
			{
				iTmp = ucSw2 & 0x0F;
				switch (iFingerRef)
				{
				case 1:
					iFingerRef1TriesLeft = iTmp;
					pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_FINGERID_1_TRIES_LEFT, SOLEMBIO_DATAFIELDPROP_NONE, NULL, iFingerRef1TriesLeft);
					if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
					{
						pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_PUTDATA_FINGER1;
						PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR PutData Finger 1 tries left [%ld][%ld][%ld]", pSBIO->lReturn, pSBIO->lError, iFingerRef1TriesLeft);
						pSBIO->lReturn = SOLEMBIO_ERROR_NO_ERROR;
						pSBIO->lError = SOLEMBIO_ERROR_NO_ERROR;
						//goto JMP_getFingerRef;
					}
					break;
				case 2:
					iFingerRef2TriesLeft = iTmp;
					pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_FINGERID_2_TRIES_LEFT, SOLEMBIO_DATAFIELDPROP_NONE, NULL, iFingerRef2TriesLeft);
					if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
					{
						pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_PUTDATA_FINGER2;
						PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR PutData Finger 2 tries left [%ld][%ld][%ld]", pSBIO->lReturn, pSBIO->lError, iFingerRef2TriesLeft);
						pSBIO->lReturn = SOLEMBIO_ERROR_NO_ERROR;
						pSBIO->lError = SOLEMBIO_ERROR_NO_ERROR;
						//goto JMP_getFingerRef;
					}
					break;
				}
				PutInLog(pLogger, LOG_LEVEL_DEBUG, (char*)"SolemICAO - getFingerRef - DEBUG Finger %d tries left [%ld]", iFingerRef, iTmp);
			}
		}
	}
	else
	{
		// cmd SELECT 0x00CB3FFF
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xCB;
		aApduHeader[2] = 0x3F;
		aApduHeader[3] = 0xFF;
		// DATA: FiID TEMPLATE 0x7F61
		iApduDataLen = 2;
		aApduData[0] = 0x7F;
		aApduData[1] = 0x61;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitSecureApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_SELECT_APP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR Read Bit [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_getFingerRef;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_SELECT_APP_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR Read Bit Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_getFingerRef;
		}

		char seachTemplate = 0;
		int Template1 = 0;
		int Template2 = 0;
		int iFinger1 = SOLEMBIO_FINGER_NONE;
		int iFinger2 = SOLEMBIO_FINGER_NONE;
		for (int Index = 0; Index < iApduResponseLen; Index++)
		{
			if (aApduResponse[Index] == 0x7F)
			{
				seachTemplate = 1;
			}
			else if (aApduResponse[Index] == 0x60 && seachTemplate == 1)
			{
				if (Template1 != 0)
				{
					Template2 = Index;
					break;
				}
				else
					Template1 = Index;
				seachTemplate = 0;
			}
			else
			{
				seachTemplate = 0;
			}
		}
		if (Template1 + 12 < iApduResponseLen)
		{
			if (aApduResponse[Template1 + 9] == 8)
			{
				iFinger1 = aApduResponse[Template1 + 12];
			}
		}
		if (Template2 + 12 < iApduResponseLen)
		{
			if (aApduResponse[Template2 + 9] == 8)
			{
				iFinger2 = aApduResponse[Template2 + 12];
			}
		}

		if ((iFinger2 & 0x01) != 0)
		{
			iFinger2 >>= 2;
			if (iFinger2 < 1 || iFinger2 > 5)
				iFinger2 = SOLEMBIO_FINGER_NONE;
		}
		else if ((iFinger2 & 0x02) != 0)
		{
			iFinger2 >>= 2;
			iFinger2 += 5;
			if (iFinger2 < 6 || iFinger2 > 10)
				iFinger2 = SOLEMBIO_FINGER_NONE;
		}

		if ((iFinger1 & 0x01) != 0)
		{
			iFinger1 >>= 2;
			if (iFinger1 < 1 || iFinger1 > 5)
				iFinger1 = SOLEMBIO_FINGER_NONE;
		}
		else if ((iFinger1 & 0x02) != 0)
		{
			iFinger1 >>= 2;
			iFinger1 += 5;
			if (iFinger1 < 6 || iFinger1 > 10)
				iFinger1 = SOLEMBIO_FINGER_NONE;
		}

		iFingerRef1 = iFinger1;
		iFingerRef2 = iFinger2;
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - getFingerRef - Fingers found [%ld][%ld]", iFingerRef1, iFingerRef2);

		}
	

	if (iFingerRef1 == SOLEMBIO_FINGER_NONE && iFingerRef2 == SOLEMBIO_FINGER_NONE && iNoFingerBugAttempts > 0)
	{
		iNoFingerBugAttempts--;
		goto JMP_NO_FINGER_BUG;
	}
	else if (iFingerRef1 == SOLEMBIO_FINGER_NONE && iFingerRef2 == SOLEMBIO_FINGER_NONE)
	{
		pSBIO->lError = SOLEMBIO_ERROR_NO_DATA_AVAILABLE;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR No data available [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_getFingerRef;
	}
	else
	{
		pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_FINGERID_1, SOLEMBIO_DATAFIELDPROP_NONE, NULL, iFingerRef1);
		if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_PUTDATA_FINGER1;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR PutData Finger 1 [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			pSBIO->lReturn = SOLEMBIO_ERROR_NO_ERROR;
			pSBIO->lError = SOLEMBIO_ERROR_NO_ERROR;
			//goto JMP_getFingerRef;
		}
		pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_FINGERID_2, SOLEMBIO_DATAFIELDPROP_NONE, NULL, iFingerRef2);
		if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETFINGERREF_PUTDATA_FINGER1;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getFingerRef - ERROR PutData Finger 2 [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			pSBIO->lReturn = SOLEMBIO_ERROR_NO_ERROR;
			pSBIO->lError = SOLEMBIO_ERROR_NO_ERROR;
			//goto JMP_getFingerRef;
		}
	}

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - getFingerRef - OUT");

JMP_getFingerRef:
	//mutex_unlock(opSICAO->oMutex);

	return pSBIO->lError;
}

static int getDocumentNumber(stSolemICAOPtr opSICAO, void *pDev)
{
	stSolemBioPtr		pSBIO = NULL;
	stLoggerPtr			pLogger = NULL;

	unsigned char		aApduHeader[4];
	unsigned char		aApduData[255];
	int					iApduDataLen = 255;
	unsigned char		ucSw1;
	unsigned char		ucSw2;
	unsigned char		aApduResponse[256 + 2];
	int					iApduResponseLen = 256 + 2;

	int					iTmp;
	int					iTmp2;
	char				sDocNum[16];
	
	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - getDocumentNumber - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	//mutex_lock(opSICAO->oMutex);

	if (!opSICAO->aCaptureField[SOLEMBIO_DATAFIELD_DOC_SERIAL])	//opSICAO->aCaptureField[SOLEMBIO_DATAFIELD_DOC_DOCUMENT]
	{
		PutInLog(pLogger, LOG_LEVEL_WARNING, (char *)"SolemICAO - getDocumentNumber - WARNING No Serial Number requested");
		goto JMP_getDocumentNumber;
	}

	// Connect Card
	if (ConnectCard(opSICAO, pDev, 0) != SOLEMICAO_ERROR_NO_ERROR)
	{
		pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_SELECT_APP;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR Connect to Card [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_getDocumentNumber;
	}

	if (iIsOld)
	{
		// cmd SELECT 0x00,0xA4,0x04,0x0C
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xA4;
		aApduHeader[2] = 0x04;
		aApduHeader[3] = 0x0C;
		// DATA: File ID (GET_FINGER) 0xE8,0x28,0xBD,0x08,0x0F,0xD2,0x50,0x43,0x68,0x6C,0x43,0x43,0x2D,0x65,0x49,0x44
		iApduDataLen = 16;
		aApduData[0] = 0xE8;
		aApduData[1] = 0x28;
		aApduData[2] = 0xBD;
		aApduData[3] = 0x08;
		aApduData[4] = 0x0F;
		aApduData[5] = 0xD2;
		aApduData[6] = 0x50;
		aApduData[7] = 0x43;
		aApduData[8] = 0x68;
		aApduData[9] = 0x6C;
		aApduData[10] = 0x43;
		aApduData[11] = 0x43;
		aApduData[12] = 0x2D;
		aApduData[13] = 0x65;
		aApduData[14] = 0x49;
		aApduData[15] = 0x44;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_SELECT_APP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR Select App [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_getDocumentNumber;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_SELECT_APP_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR Select App Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_getDocumentNumber;
		}

		if (iApduResponseLen > 0)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - getDocumentNumber - WARNING Select App Resp [%ld]", iApduResponseLen);
		}

		// cmd SELECT 0x00,0xA4,0x00,0x0C
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xA4;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x0C;
		// DATA: File ID (MF) 0x3F,0x00
		iApduDataLen = 2;
		aApduData[0] = 0x3F;
		aApduData[1] = 0x00;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_SELECT_MF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR Select MF [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_getDocumentNumber;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_SELECT_MF_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR Select MF Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_getDocumentNumber;
		}

		if (iApduResponseLen > 0)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - getDocumentNumber - WARNING Select MF Resp [%ld]", iApduResponseLen);
		}

		// cmd SELECT 0x00,0xA4,0x02,0x0C
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xA4;
		aApduHeader[2] = 0x02;
		aApduHeader[3] = 0x0C;
		// DATA: File ID (FILE) 0xD0,0x03
		iApduDataLen = 2;
		aApduData[0] = 0xD0;
		aApduData[1] = 0x03;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_SELECT;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR Select File [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_getDocumentNumber;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_SELECT_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR Select File Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_getDocumentNumber;
		}

		if (iApduResponseLen > 0)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - getDocumentNumber - WARNING Select File Resp [%ld]", iApduResponseLen);
		}

		// cmd READ BINARY 0x00,0xB0,0x**,0x**
		aApduHeader[0] = 0x00;
		aApduHeader[1] = 0xB0;
		aApduHeader[2] = 0x00;
		aApduHeader[3] = 0x05;
		// DATA: EMPTY
		iApduDataLen = 0;
		//aApduData[0] = 0x00;

		iApduResponseLen = sizeof(aApduResponse);
		pSBIO->lReturn = transmitPlainApdu(opSICAO, pDev, aApduHeader, (unsigned char)0, NULL, 5, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
		if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_READ;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR Read Binary [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_getDocumentNumber;
		}
		if (ucSw1 != 0x90 || ucSw2 != 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_READ_RESP;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR Read Binary Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			goto JMP_getDocumentNumber;
		}

		if (iApduResponseLen != 5)
		{
			PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - getDocumentNumber - WARNING Read Binary Resp [%ld]", iApduResponseLen);
		}

		memset(sDocNum, 0, sizeof(sDocNum));
		for (iTmp = 0; iTmp < (2 * iApduResponseLen) - 1; iTmp++)
		{
			iTmp2 = (aApduResponse[iTmp / 2] >> ((iTmp + 1) % 2) * 4) & 0x0F;
			sprintf_s(sDocNum, sizeof(sDocNum), "%s%d", sDocNum, iTmp2);
		}

		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - getDocumentNumber - Doc Number [%s]", sDocNum);

		pSBIO->lReturn = sBioPutData(pSBIO, SOLEMBIO_DATAFIELD_DOC_SERIAL, SOLEMBIO_DATAFIELDPROP_NONE, sDocNum, strlen(sDocNum));
		if (pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_GETDOCUMENTNUMBER_PUTDATA_DOC_SERIAL;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - getDocumentNumber - ERROR PutData Document Number [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			goto JMP_getDocumentNumber;
		}
	}
	else
	{
		PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - getDocumentNumber - New Doc - Skip Document Number");
	}

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - getDocumentNumber - OUT");

JMP_getDocumentNumber:
	//mutex_unlock(opSICAO->oMutex);

	return pSBIO->lError;
}


int SolemICAOInit(stSolemBioPtr	opSBIO)
{	
	stSolemICAOPtr		pTmpSICAO = NULL;
	stLoggerPtr			pLogger = NULL;

	pLogger = opSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - Init - IN");

	//return 107;

	pTmpSICAO = (stSolemICAOPtr)calloc(1,sizeof(stSolemICAO));
	if(pTmpSICAO == NULL)
	{
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Init - ERROR Creating Engine Handle");
		return SOLEMICAO_ERROR_INIT_ENGINEHANDLE_MEM;
	}
	opSBIO->hSolemICAO = pTmpSICAO;
	pTmpSICAO->pSBIO = opSBIO;
	
	opSBIO->lReturn = 0;
	opSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	memset(pTmpSICAO->aCaptureField, 0, sizeof(pTmpSICAO->aCaptureField));
	memset(pTmpSICAO->aCaptureDG, 0, sizeof(pTmpSICAO->aCaptureDG));
	memset(pTmpSICAO->sICAOKey, 0, sizeof(pTmpSICAO->sICAOKey));
	memset(pTmpSICAO->aKeyEnc, 0, sizeof(pTmpSICAO->aKeyEnc));
	memset(pTmpSICAO->aKeyMac, 0, sizeof(pTmpSICAO->aKeyMac));

	// Engine/Device Mutex Definition
	pTmpSICAO->iMutexFlag = -1;
#ifdef _WIN32
	pTmpSICAO->iMutexFlag = ((pTmpSICAO->oMutex = CreateMutex(NULL, 0, NULL)) == NULL);
	if (pTmpSICAO->iMutexFlag != 0)
	{
		opSBIO->lReturn = GetLastError();
		opSBIO->lError = SOLEMICAO_ERROR_CREATE_MUTEX;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Init - ERROR Creating Mutex [%ld][%ld]", opSBIO->lReturn, opSBIO->lError);
		goto JMP_SolemICAOInit;
	}
#else
	if ((pTmpSICAO->iMutexFlag = pthread_mutex_init(&pTmpSICAO->oMutex, NULL)) != 0)
	{
		opSBIO->lReturn = pTmpSICAO->iMutexFlag;
		opSBIO->lError = SOLEMICAO_ERROR_CREATE_MUTEX;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Init - ERROR Creating Mutex [%ld][%ld]", opSBIO->lReturn, opSBIO->lError);
		goto JMP_SolemICAOInit;
	}
#endif

	// INIT EAC-PACE
	EAC_init();

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - Init - OUT");

JMP_SolemICAOInit:
	return opSBIO->lError;
}

void SolemICAOClean(stSolemICAOPtr opSICAO)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;

	if(opSICAO == NULL)
	{
		//	PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemBio - ERROR Engine Handle NULL");
		return;
	}

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;
	
	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - Clean - IN");
	
	// Clean Mutex
	mutex_destroy(opSICAO->oMutex);
	opSICAO->iMutexFlag = -1;

	pSBIO->hSolemICAO = NULL;
	free(opSICAO);

	// Clean EAC-PACE
	EAC_cleanup();

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - Clean - OUT");

	return;
}

int SolemICAOAddCaptureField(stSolemICAOPtr opSICAO, int iFieldID)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;
	
	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - AddCaptureField - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	mutex_lock(opSICAO->oMutex);

	switch(iFieldID)
	{
	case SOLEMBIO_DATAFIELD_DOC_RUN:
	case SOLEMBIO_DATAFIELD_DOC_NAMES:
	case SOLEMBIO_DATAFIELD_DOC_SURNAME_1:
	case SOLEMBIO_DATAFIELD_DOC_SURNAME_2:
	case SOLEMBIO_DATAFIELD_DOC_GENDER:
	case SOLEMBIO_DATAFIELD_DOC_NATIONALITY:
	case SOLEMBIO_DATAFIELD_DOC_BIRTHDATE:
	case SOLEMBIO_DATAFIELD_DOC_EXPIRYDATE:
	case SOLEMBIO_DATAFIELD_DOC_MRZ:
	//case SOLEMBIO_DATAFIELD_DOC_SERIAL:
	case SOLEMBIO_DATAFIELD_DOC_COUNTRY_OF_ISSUE:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATA);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO Data [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATA, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_1] = TRUE;
		break;
	//	case SOLEMBIO_DATAFIELD_DOC_LASTNAME:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATA);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO Data (Lastname) [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATA, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[SOLEMBIO_DATAFIELD_DOC_SURNAME_1] = TRUE;
		opSICAO->aCaptureField[SOLEMBIO_DATAFIELD_DOC_SURNAME_2] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_1] = TRUE;
		break;
	//case SOLEMBIO_DATAFIELD_DOC_NAMES:
	//case SOLEMBIO_DATAFIELD_DOC_SURNAME_1:
	//case SOLEMBIO_DATAFIELD_DOC_SURNAME_2:
	case SOLEMBIO_DATAFIELD_DOC_OCUPATION:
	case SOLEMBIO_DATAFIELD_DOC_BIRTHPLACE:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATA);
		if (pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO Data (Issuedate) [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATA, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_11] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_ISSUEDATE:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATA);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO Data (Issuedate) [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATA, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_12] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_FACE:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_FACE);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO Face [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_FACE, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_2] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_SIGNATURE:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_SIGNATURE);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO Signature [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_SIGNATURE, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_7] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_SERIAL:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DOCNUM);
		if (pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO Doc Number [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DOCNUM, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetFinger = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_FINGERID_1:
	case SOLEMBIO_DATAFIELD_DOC_FINGERID_2:
	case SOLEMBIO_DATAFIELD_DOC_FINGERID_1_TRIES_LEFT:
	case SOLEMBIO_DATAFIELD_DOC_FINGERID_2_TRIES_LEFT:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_FINGER);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO Finger [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_FINGER, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetFinger = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_COM:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup COM [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_COM] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_1:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 1 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_1] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_2:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 2 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_2] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_3:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 3 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_3] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_4:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 4 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_4] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_5:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 5 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_5] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_6:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 6 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_6] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_7:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 7 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_7] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_8:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 8 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_8] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_9:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 9 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_9] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_10:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 10 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_10] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_11:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 11 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_11] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_12:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 12 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_12] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_13:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 13 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_13] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_14:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 14 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_14] = TRUE;
		break;
	case SOLEMBIO_DATAFIELD_DOC_DG_15:
		pSBIO->lReturn = scl_GetFnBitmap(pSBIO->hSCL, SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP);
		if(pSBIO->lReturn != SCL_FNBITMAP_ON)
		{
			pSBIO->lError = SOLEMBIO_ERROR_LICENSE_FNBITMAP_OFF;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR License FnBitmap Not Allow Capture ICAO DataGroup 15 [%ld][%ld][%ld]", SOLEMBIO_FEATUREBITMAP_CAPT_ICAO_DATAGROUP, pSBIO->lReturn, pSBIO->lError);
			goto JMP_SolemICAOAddCaptureField;
		}
		opSICAO->iGetICAO = TRUE;
		opSICAO->aCaptureField[iFieldID] = TRUE;
		opSICAO->aCaptureDG[SOLEMICAO_DATAGROUP_15] = TRUE;
		break;
	default:
		pSBIO->lError = SOLEMICAO_ERROR_ADDCAPTUREFIELD_DATAFIELD_INALID;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - AddCaptureField - ERROR DataField Invalid [%d]", iFieldID);
		goto JMP_SolemICAOAddCaptureField;
	}

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - AddCaptureField - OUT");

JMP_SolemICAOAddCaptureField:
	mutex_unlock(opSICAO->oMutex);

	return pSBIO->lError;
}

int SolemICAOCleanCaptureFields(stSolemICAOPtr opSICAO)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;
	
	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - CleanCaptureField - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;

	mutex_lock(opSICAO->oMutex);

	memset(opSICAO->aCaptureField, 0, sizeof(opSICAO->aCaptureField));
	memset(opSICAO->aCaptureDG, 0, sizeof(opSICAO->aCaptureDG));
	opSICAO->iGetICAO = FALSE;
	opSICAO->iGetFinger = FALSE;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - CleanCaptureField - OUT");

	mutex_unlock(opSICAO->oMutex);

	return pSBIO->lError;
}

int SolemICAOCapture(stSolemICAOPtr opSICAO, int iDeviceID, int iTimeout)
{
stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;
void				*pTmpDev = NULL;

long			lInitialTime;
long			lActualTime;

long			lTmpError;

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;
	
	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - Capture - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;
	
	//	Get and Check Device Handler
	pTmpDev = pSBIO->vpDeviceHandler[iDeviceID];
	if (pTmpDev == NULL)
	{
		pSBIO->lError = SOLEMICAO_ERROR_CAPTURE_DEVICEHANDLE_NULL;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Capture - ERROR SmartCard Reader Uninitialized [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_SolemICAOCapture;
	}
	
	if (iDeviceID == SOLEMBIO_DEVICE_BIOPIN)
	{
		// opSICAO->fn_GetCard = &BioPinGetCard;
		// opSICAO->fn_Transmit = &BioPinTransmit;
		// opSICAO->fn_Disconnect = &BioPinDisconnect;
	}
	else if (iDeviceID == SOLEMBIO_DEVICE_BCR250BT)
	{
		// opSICAO->fn_GetCard = &Bcr250btGetCard;
		// opSICAO->fn_Transmit = &Bcr250btTransmit;
		// opSICAO->fn_Disconnect = &Bcr250btDisconnect;
	}
	else if (iDeviceID == SOLEMBIO_DEVICE_NFC)
	{
		// opSICAO->fn_GetCard = &NfcGetCard;
		// opSICAO->fn_Transmit = &NfcTransmit;
		// opSICAO->fn_Disconnect = &NfcDisconnect;
	}
	else
	{
		// opSICAO->fn_GetCard = &SmartCardGetCard;
		// opSICAO->fn_Transmit = &SmartCardTransmit;
		// opSICAO->fn_Disconnect = &SmartCardDisconnect;
	}


	mutex_lock(opSICAO->oMutex);

	lInitialTime = GetTickCount();
	do
	{
		// Check presence and get SmartCard
		pSBIO->lReturn = opSICAO->fn_GetCard(pTmpDev);
		lActualTime = GetTickCount();
	} while (pSBIO->lReturn == SMARTCARD_ERROR_NO_CARD_AVAILABLE && (lActualTime - lInitialTime) < iTimeout);
	if(pSBIO->lReturn == SMARTCARD_ERROR_NO_CARD_AVAILABLE)
	{
		pSBIO->lError = SOLEMICAO_ERROR_NO_CARD_AVAILABLE;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Capture - ERROR SmartCard Not Available [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_SolemICAOCapture;
	}
	else if(pSBIO->lReturn != SMARTCARD_ERROR_NO_ERROR)
	{
		pSBIO->lError = SOLEMICAO_ERROR_CAPTURE_GETCARD;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Capture - ERROR Get SmartCard [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_SolemICAOCapture;
	}

	//	Get Finger Reference
	if (getFingerRef(opSICAO, pTmpDev) != SOLEMICAO_ERROR_NO_ERROR)
	{
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Capture - ERROR GetFingerRef [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_SolemICAOCapture;
	}

	// Get Document Number
	if (getDocumentNumber(opSICAO, pTmpDev) != SOLEMICAO_ERROR_NO_ERROR)
	{
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Capture - ERROR GetDocumentNumber [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_SolemICAOCapture;
	}
	
	//	Get ICAO Data
	if (getIcao(opSICAO, pTmpDev) != SOLEMICAO_ERROR_NO_ERROR)
	{
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Capture - ERROR GetIcao [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_SolemICAOCapture;
	}

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - Capture - OUT");
JMP_SolemICAOCapture:
	if (pSBIO->lError != SOLEMICAO_ERROR_NO_CARD_AVAILABLE)
	{
		lTmpError = pSBIO->lError;
		pSBIO->lReturn = opSICAO->fn_Disconnect(pTmpDev);
		if (pSBIO->lReturn != SMARTCARD_ERROR_NO_ERROR)
		{
			pSBIO->lError = SOLEMICAO_ERROR_CAPTURE_DISCONNECT;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - Capture - ERROR SmartCardDisconnect [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		}
		pSBIO->lError = lTmpError;
	}

	mutex_unlock(opSICAO->oMutex);

	return pSBIO->lError;
}

int SolemICAOMOC(stSolemICAOPtr opSICAO, int iDeviceID, int iFingerRef, int iFieldID, int *ipMatchResult)
{

stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;
void				*pTmpDev = NULL;

unsigned char		aIsoCTemplate[255];
int					iIsoCTemplateLen = 255;

unsigned char		aApduHeader[4];
unsigned char		aApduData[255];
int					iApduDataLen = 255;
unsigned char		aApduResponse[256 + 2];
int					iApduResponseLen = 256 + 2;
unsigned char		ucSw1;
unsigned char		ucSw2;

int					iPinNotFoundBugRetryCount = 0;
long				lTmpError = SOLEMBIO_ERROR_NO_ERROR;

long			lInitialTime;
long			lActualTime;
int				iTimeout = 10000;

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - MOC - IN");

	pSBIO->lReturn = 0;
	pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;


	///////////////////////////////////////////////////////////////////////////
	// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 
	// mutex_lock(opSICAO->oMutex);
	///////////////////////////////////////////////////////////////////////////
	// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 


	///////////////////////////////////////////////////////////////////////////
	// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 
	//	Get and Check Device Handler
	//pTmpDev = pSBIO->vpDeviceHandler[iDeviceID];
	//if (pTmpDev == NULL)
	//{
	//	pSBIO->lError = SOLEMICAO_ERROR_MOC_DEVICEHANDLE_NULL;
	//	PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR SmartCard Reader Uninitialized [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
	//	goto JMP_matchOnCard;
	//}
	///////////////////////////////////////////////////////////////////////////
	// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 




	if (iDeviceID == SOLEMBIO_DEVICE_BIOPIN)
	{
		// opSICAO->fn_GetCard = &BioPinGetCard;
		// opSICAO->fn_Transmit = &BioPinTransmit;
		// opSICAO->fn_Disconnect = &BioPinDisconnect;
	}
	else if (iDeviceID == SOLEMBIO_DEVICE_BCR250BT)
	{
		// opSICAO->fn_GetCard = &Bcr250btGetCard;
		// opSICAO->fn_Transmit = &Bcr250btTransmit;
		// opSICAO->fn_Disconnect = &Bcr250btDisconnect;
	}
	else if (iDeviceID == SOLEMBIO_DEVICE_NFC)
	{
		// opSICAO->fn_GetCard = &NfcGetCard;
		// opSICAO->fn_Transmit = &NfcTransmit;
		// opSICAO->fn_Disconnect = &NfcDisconnect;
	}
	else
	{
		// opSICAO->fn_GetCard = &SmartCardGetCard;
		// opSICAO->fn_Transmit = &SmartCardTransmit;
		// opSICAO->fn_Disconnect = &SmartCardDisconnect;
	}

	//	Check Finger Reference ID
	if(iFingerRef != 1 && iFingerRef != 2)
	{
		pSBIO->lReturn = iFingerRef;
		pSBIO->lError = SOLEMICAO_ERROR_MOC_FINGER_REF_INVALID;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Finger Ref Invalid [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_matchOnCard;
	}

	
	// Get Iso Compact Template for Matching
	pSBIO->lReturn = sBioGetData(pSBIO, (enDataField)iFieldID, SOLEMBIO_DATAFIELDPROP_MINUTIAE_SOLISOC, aIsoCTemplate, &iIsoCTemplateLen);
	if(pSBIO->lReturn != SOLEMBIO_ERROR_NO_ERROR)
	{
		pSBIO->lError = SOLEMICAO_ERROR_MOC_GET_TEMPLATE;
		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Get IsoCompact Template [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
		goto JMP_matchOnCard;
	}

	// Check presence and get SmartCard
	//pSBIO->lReturn = opSICAO->fn_GetCard(pTmpDev);

	//// TEST TEMPLATE - IN
	//unsigned char buffer[255];
	//FILE* ptr;
	////   20241227_014059_IsoCompact.min (240 bytes)
	//ptr = fopen("c:/Temp/bech/20241229_112624_IsoCompact.min", "rb");	// r for read, b for binary
	//fread(buffer, sizeof(unsigned char), 231, ptr);							// read 10 bytes to our buffer
	//memcpy_s(aIsoCTemplate, sizeof(aIsoCTemplate), buffer, 231);
	//iIsoCTemplateLen = 231;
	//PutInLog(pLogger, LOG_LEVEL_INFORMATIONAL, (char*)"SolemICAO - MOC - Using Test Template [%ld]", iIsoCTemplateLen);
	//// TEST TEMPLATE - OUT
	

	///////////////////////////////////////////////////////////////////////////
	// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 
	//lInitialTime = GetTickCount();
	//do
	//{
	//	// Check presence and get SmartCard
	//	pSBIO->lReturn = opSICAO->fn_GetCard(pTmpDev);
	//	lActualTime = GetTickCount();
	//} while (pSBIO->lReturn == SMARTCARD_ERROR_NO_CARD_AVAILABLE && (lActualTime - lInitialTime) < iTimeout);

	//if(pSBIO->lReturn == SMARTCARD_ERROR_NO_CARD_AVAILABLE)
	//{
	//	pSBIO->lError = SOLEMICAO_ERROR_NO_CARD_AVAILABLE;
	//	PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR SmartCard Not Available [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
	//	goto JMP_matchOnCard;
	//}
	//else if(pSBIO->lReturn != SMARTCARD_ERROR_NO_ERROR)
	//{
	//	pSBIO->lError = SOLEMICAO_ERROR_MOC_GETCARD;
	//	PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Get SmartCard [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
	//	goto JMP_matchOnCard;
	//}
	///////////////////////////////////////////////////////////////////////////
	// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 



	while(iPinNotFoundBugRetryCount < 3)
	{
		pSBIO->lError = SOLEMICAO_ERROR_NO_ERROR;
		iPinNotFoundBugRetryCount++;

		memset(aApduHeader, 0, sizeof(aApduHeader));
		memset(aApduData, 0, sizeof(aApduData));

		// Check if Card was no swapped making a ICAO connection first, to force authentication ('cause old cards don't requiere authentication for MOC)
		if (ConnectCard(opSICAO, pTmpDev, 1) != SOLEMICAO_ERROR_NO_ERROR)
		{
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - MOC - ERROR ICAO Connect to Card [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			continue;
		}

		// Connect Card
		if (ConnectCard(opSICAO, pTmpDev, 0) != SOLEMICAO_ERROR_NO_ERROR)
		{
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - MOC - ERROR Connect to Card [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
			continue;
		}

		if (iIsOld)
		{
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - MOC - MOC in Old Document [%ld]", iPinNotFoundBugRetryCount);

			// cmd SELECT 0x00,0xA4,0x04,0x0C
			aApduHeader[0] = 0x00;
			aApduHeader[1] = 0xA4;
			aApduHeader[2] = 0x04;
			aApduHeader[3] = 0x0C;
			// DATA: File ID (MOC)			0xE8,0x28,0xBD,0x08,0x0F,0xD2,0x50,0x43,0x68,0x6C,0x43,0x43,0x2D,0x65,0x49,0x44
			iApduDataLen = 16;
			aApduData[0] = 0xE8;
			aApduData[1] = 0x28;
			aApduData[2] = 0xBD;
			aApduData[3] = 0x08;
			aApduData[4] = 0x0F;
			aApduData[5] = 0xD2;
			aApduData[6] = 0x50;
			aApduData[7] = 0x43;
			aApduData[8] = 0x68;
			aApduData[9] = 0x6C;
			aApduData[10] = 0x43;
			aApduData[11] = 0x43;
			aApduData[12] = 0x2D;
			aApduData[13] = 0x65;
			aApduData[14] = 0x49;
			aApduData[15] = 0x44;

			iApduResponseLen = sizeof(aApduResponse);
			pSBIO->lReturn = transmitPlainApdu(opSICAO, pTmpDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_MOC_SELECT_APP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - MOC - ERROR Select App [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
				continue;
				//goto JMP_matchOnCard;
			}
			if (ucSw1 != 0x90 || ucSw2 != 0x00)
			{
				pSBIO->lError = SOLEMICAO_ERROR_MOC_SELECT_APP_RESP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - MOC - ERROR Select App Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
				continue;
				//goto JMP_matchOnCard;
			}


			if (iIsoCTemplateLen > 120)
			{
				PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - MOC - WARN Template too large [%d]", iIsoCTemplateLen);
				//iIsoCTemplateLen = 120;
			}

			memset(aApduHeader, 0, sizeof(aApduHeader));
			memset(aApduData, 0, sizeof(aApduData));
			// {0x00, 0x21, 0x00, 0xXX, 0x[Len Template + 5], 0x7F, 0x2E, 0x[Len Template + 2], 0x81, 0x[Len Template], 0x[Template]} 0xXX / 0x91 / 0x92
			// cmd INTERNAL(MOC) 0x00,0x21,0x00,0x9X //X = 1 or 2 (finger ref)
			aApduHeader[0] = 0x00;
			aApduHeader[1] = 0x21;
			aApduHeader[2] = 0x00;
			aApduHeader[3] = 0x90 + iFingerRef;
			// DATA: INTERNAL(MOC) 0x7F, 0x2E, 0x[Len Template + 2], 0x81, 0x[Len Template], {Template}
			//iApduDataLen = 0x05 + iIsoCTemplateLen;
			iApduDataLen = 0;
			aApduData[iApduDataLen++] = 0x7F;
			aApduData[iApduDataLen++] = 0x2E;
			if (iIsoCTemplateLen > 127)
			{
				aApduData[iApduDataLen++] = 0x81;
				aApduData[iApduDataLen++] = iIsoCTemplateLen + 0x02 + 0x01;
			}
			else
			{
				aApduData[iApduDataLen++] = iIsoCTemplateLen + 0x02;
			}
			aApduData[iApduDataLen++] = 0x81;
			if (iIsoCTemplateLen > 127)
			{
				aApduData[iApduDataLen++] = 0x81;
			}
			aApduData[iApduDataLen++] = iIsoCTemplateLen;

			memcpy_s(aApduData + iApduDataLen, sizeof(aApduData) - iApduDataLen, aIsoCTemplate, iIsoCTemplateLen);
			iApduDataLen += iIsoCTemplateLen;

			iApduResponseLen = sizeof(aApduResponse);
			pSBIO->lReturn = transmitPlainApdu(opSICAO, pTmpDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_MOC_MATCH;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - MOC - ERROR Match [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
				continue;
				//goto JMP_matchOnCard;
			}
		}
		else
		{
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char*)"SolemICAO - MOC - MOC in New Document [%ld]", iPinNotFoundBugRetryCount);

			if (iIsoCTemplateLen > 120)
			{
				PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - MOC - WARN Template too large [%d]", iIsoCTemplateLen);
				//iIsoCTemplateLen = 120;
				if (iIsoCTemplateLen > 180)
				{
					iIsoCTemplateLen = 180;
					PutInLog(pLogger, LOG_LEVEL_WARNING, (char*)"SolemICAO - MOC - WARN Truncating Template [%ld]", iIsoCTemplateLen);
				}
			}


			// Try new document
			//
			memset(aApduHeader, 0, sizeof(aApduHeader));
			memset(aApduData, 0, sizeof(aApduData));

			/*unsigned char aTag[2];
			unsigned char aDataTmp1[256];
			aTag[0] = 0x81;
			int wrapLen1 = wrap(&aTag, 1, aIsoCTemplate, iIsoCTemplateLen, aDataTmp1, sizeof(aDataTmp1));
			if (wrapLen1 > 0)
			{
				aTag[0] = 0x7F;
				aTag[1] = 0x2E;
				int wrapLen2 = wrap(&aTag, 2, aDataTmp1, wrapLen1, aApduData, sizeof(aApduData));
				if (wrapLen2 > 0)
				{
					iApduDataLen = wrapLen2;
				}
				else
				{
					PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x7F2E");
					goto JMP_matchOnCard;
				}
			}
			else
			{
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - ConnectCard - Error wrapping 0x81");
				goto JMP_matchOnCard;
			}*/


			// cmd SELECT 00210006
			aApduHeader[0] = 0x00;
			aApduHeader[1] = 0x21;
			aApduHeader[2] = 0x00;
			aApduHeader[3] = 0x06;
			// DATA: Template wrapped
			iApduDataLen = 0;
			aApduData[iApduDataLen++] = 0x7F;
			aApduData[iApduDataLen++] = 0x2E;
			if (iIsoCTemplateLen > 127)
			{
				aApduData[iApduDataLen++] = 0x81;
				aApduData[iApduDataLen++] = iIsoCTemplateLen + 0x02 + 0x01;
			}
			else
			{
				aApduData[iApduDataLen++] = iIsoCTemplateLen + 0x02;
			}
			aApduData[iApduDataLen++] = 0x81;
			if (iIsoCTemplateLen > 127)
			{
				aApduData[iApduDataLen++] = 0x81;
			}
			aApduData[iApduDataLen++] = iIsoCTemplateLen;

			memcpy_s(aApduData + iApduDataLen, sizeof(aApduData) - iApduDataLen, aIsoCTemplate, iIsoCTemplateLen);
			iApduDataLen += iIsoCTemplateLen;

			iApduResponseLen = sizeof(aApduResponse);

			iApduResponseLen = sizeof(aApduResponse);

			pSBIO->lReturn = transmitSecureApdu(opSICAO, pTmpDev, aApduHeader, (unsigned char)iApduDataLen, aApduData, 0, &ucSw1, &ucSw2, aApduResponse, &iApduResponseLen);
			if (pSBIO->lReturn != SOLEMICAO_ERROR_NO_ERROR)
			{
				pSBIO->lError = SOLEMICAO_ERROR_MOC_SELECT_APP;
				PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"SolemICAO - MOC - ERROR Select App [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
				continue;
				//goto JMP_matchOnCard;
			}


			//PutInLog(pLogger, LOG_LEVEL_ERROR, (char*)"POST TX iIsoCTemplateLen -> %d / 0x%02X%02X", iIsoCTemplateLen, ucSw1, ucSw2);
			//return;


		}

		
	
		//	if(ucSw1 != 0x90 || ucSw2 != 0x00)
		//	{
		//		pSBIO->lError = SOLEMICAO_ERROR_MOC_SELECT_APP_RESP;
		//		PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - ERROR Match Resp [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
		//		goto JMP_matchOnCard;
		//	}
	
		if( ucSw1 == 0x90 && ucSw2 == 0x00)
		{
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - MOC - Do Match");
			*ipMatchResult = SOLEMBIO_MOC_HIT;
		}
		else if( ucSw1 == 0x69 && ucSw2 == 0x83)
		{
#ifndef _FOR_BECH
			pSBIO->lError = SOLEMBIO_ERROR_MOC_LOCKED;
#endif
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - MOC - Don't Match (moc locked)");
			*ipMatchResult = SOLEMBIO_MOC_NOHIT;
		}
		else if( ucSw1 == 0x63 && ucSw2 == 0x00)
		{
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - MOC - Don't Match");
			*ipMatchResult = SOLEMBIO_MOC_NOHIT;
		}
		else if( ucSw1 == 0x63 && ucSw2 >= 0xC0 && ucSw2 <= 0xCF)
		{
			PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - MOC - Don't Match with %d retries left", ucSw2 & 0x0F);
			*ipMatchResult = SOLEMBIO_MOC_NOHIT;
		}
		else if( ucSw1 == 0x6A && ucSw2 == 0x86)
		{
			pSBIO->lError = SOLEMICAO_ERROR_MOC_MATCH_P1;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Match Resp (P1 != '00') [%ld]", pSBIO->lError);
			continue;
			//goto JMP_matchOnCard;
		}
		else if( ucSw1 == 0x67 && ucSw2 == 0x00)
		{
			pSBIO->lError = SOLEMICAO_ERROR_MOC_MATCH_WRONG_LENGTH;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Match Resp (Wrong length) [%ld][%ld]", pSBIO->lError, iIsoCTemplateLen);
			continue;
			//goto JMP_matchOnCard;
		}
		else if( ucSw1 == 0x6A && ucSw2 == 0x88)
		{
			pSBIO->lError = SOLEMICAO_ERROR_MOC_MATCH_PIN_NOT_FOUND;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Match Resp (Referenced PIN not found) [%ld][%02X]", pSBIO->lError, aApduHeader[3]);
			continue;
			//goto JMP_matchOnCard;
		}
		else if( ucSw1 == 0x69 && ucSw2 == 0x82)
		{
			pSBIO->lError = SOLEMICAO_ERROR_MOC_MATCH_SECURITY;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Match Resp (Security Status not satisfied) [%ld]", pSBIO->lError);
			continue;
			//goto JMP_matchOnCard;
		}
		else if( ucSw1 == 0x69 && ucSw2 == 0x84)
		{
			pSBIO->lError = SOLEMICAO_ERROR_MOC_MATCH_COUNTER_ZERO;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Match Resp (Usage counter reached 0) [%ld]", pSBIO->lError);
			continue;
			//goto JMP_matchOnCard;
		}
		else if( ucSw1 == 0x63 && ucSw2 == 0x63)
		{
			pSBIO->lError = SOLEMICAO_ERROR_MOC_MATCH_NEED_MORE_DATA;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Match Resp (Need more data) [%ld]", pSBIO->lError);
			continue;
			//goto JMP_matchOnCard;
		}
		else
		{
			pSBIO->lError = SOLEMICAO_ERROR_MOC_MATCH_UNKNOWN;
			PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Match Resp (Unknown error) [%02x:%02x][%ld]", ucSw1, ucSw2, pSBIO->lError);
			continue;
			//goto JMP_matchOnCard;
		}
		break;
	}

	if(pSBIO->lError != SOLEMICAO_ERROR_NO_ERROR)
		goto JMP_matchOnCard;

	PutInLog(pLogger, LOG_LEVEL_NOTICE, (char *)"SolemICAO - MOC - OUT");
JMP_matchOnCard:
	
	lTmpError = pSBIO->lError;

	///////////////////////////////////////////////////////////////////////////
	// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 
	//pSBIO->lReturn = opSICAO->fn_Disconnect(pTmpDev);
	//if(pSBIO->lReturn != SMARTCARD_ERROR_NO_ERROR)
	//{
	//	pSBIO->lError = SOLEMICAO_ERROR_MOC_DISCONNECT;
	//	PutInLog(pLogger, LOG_LEVEL_ERROR, (char *)"SolemICAO - MOC - ERROR Disconnect [%ld][%ld]", pSBIO->lReturn, pSBIO->lError);
	//}
	///////////////////////////////////////////////////////////////////////////
// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 


	pSBIO->lError = lTmpError;

	///////////////////////////////////////////////////////////////////////////
	// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 
	// mutex_unlock(opSICAO->oMutex);
	///////////////////////////////////////////////////////////////////////////
	// RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA RAFA 

	return pSBIO->lError;
}
