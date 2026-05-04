#ifndef __TAG_ICAO_H__
#define __TAG_ICAO_H__

#include <generictypedefs.h>
#include <BioDefs.h>

#define ICAO_READ_CHUNK	215

#define ICAO_AUTH_BUG_RETRY	3

#define DEFAULT_ICAO_OPUPATION			(char*)"No informada"

#ifdef _WIN32
#include <windows.h>


typedef int	(__cdecl *GETCARD)		(IN void *pDev);
typedef int	(__cdecl *TRANSMIT)		(IN void *pDev, IN unsigned char *ucpApduCmd, IN long lApduCmdLen, OUT unsigned char *ucpApduRsp, IN OUT long *lpApduRspLen);
typedef int	(__cdecl *DISCONNECT)	(IN void *pDev);

#define	__delay_ms(ms) Sleep(ms)	

#endif

enum
{
	ICAO_DATAGROUP_COM = 0,
	ICAO_DATAGROUP_1,
	ICAO_DATAGROUP_2,
	ICAO_DATAGROUP_3,
	ICAO_DATAGROUP_4,
	ICAO_DATAGROUP_5,
	ICAO_DATAGROUP_6,
	ICAO_DATAGROUP_7,
	ICAO_DATAGROUP_8,
	ICAO_DATAGROUP_9,
	ICAO_DATAGROUP_10,
	ICAO_DATAGROUP_11,
	ICAO_DATAGROUP_12,
	ICAO_DATAGROUP_13,
	ICAO_DATAGROUP_14,
	ICAO_DATAGROUP_15,
	ICAO_DATAGROUP_MAX,
};


/*

stHndBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;
int					iTmpApduRspLen = 0;
unsigned char		aApduMsg[255 + 6];
int					iApduMsgLen = 255 + 6;
unsigned char		aApduResponse[256];// +2];
int					iApduResponseLen = 256 + 2;

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;
*/

#define BIO_DATAFIELD_MAX 32 // !!!! ???????


typedef struct ststHndBioPtr{

	void 	*pLogger;
	int		lReturn;
	int 	lError;
	void*	HndICAO;
	void*	hSCL;
	void 	**vpDeviceHandler;
	
	//sHndICAOPtr			*opSICAO:

}stHndBio, *stHndBioPtr;




typedef struct ststHndICAO{
	
	stHndBioPtr		pSBIO;

	int				aCaptureField[BIO_DATAFIELD_MAX];
	int				aCaptureDG[16];
	int				iGetICAO;
	int				iGetFinger;

	char				sICAOKey[32];
	unsigned char	aKeyEnc[16];
	unsigned char	aKeyMac[16];
	unsigned char	aSessCount[8];
	
	unsigned char	aDoc_DG_COM[32];	//	EF.COM	29		Common information for the application
	int				iDoc_DG_COMLen;
	unsigned char	aDoc_DG_1[128];	//	EF.DG1	93		Details recorded in MRZ
	int				iDoc_DG_1Len;
	unsigned char	aDoc_DG_2[16384];	//	EF.DG2	15360	Encoded face
	int				iDoc_DG_2Len;
	unsigned char	aDoc_DG_3[1];		
	int				iDoc_DG_3Len;
	unsigned char	aDoc_DG_4[1];		
	int				iDoc_DG_4Len;
	unsigned char	aDoc_DG_5[1];		
	int				iDoc_DG_5Len;
	unsigned char	aDoc_DG_6[1];		
	int				iDoc_DG_6Len;
	unsigned char	aDoc_DG_7[9216];	//	EF.DG7	8120	Displayed signature (8194)
	int				iDoc_DG_7Len;
	unsigned char	aDoc_DG_8[1];		
	int				iDoc_DG_8Len;
	unsigned char	aDoc_DG_9[1];		
	int				iDoc_DG_9Len;
	unsigned char	aDoc_DG_10[1];	
	int				iDoc_DG_10Len;
	unsigned char	aDoc_DG_11[128];	//	EF.DG11	40		Additional personal details
	int				iDoc_DG_11Len;
	unsigned char	aDoc_DG_12[128];	//	EF.DG12	60		Additional document details
	int				iDoc_DG_12Len;
	unsigned char	aDoc_DG_13[256];	//	EF.DG13	100		Optional details
	int				iDoc_DG_13Len;
	unsigned char	aDoc_DG_14[1];
	int				iDoc_DG_14Len;
	unsigned char	aDoc_DG_15[1024];	//	EF.DG15	512		Active Authentication Public Key Info
	int				iDoc_DG_15Len;
	
#ifdef _WIN32
	GETCARD		fn_GetCard;
	TRANSMIT	fn_Transmit;
	DISCONNECT	fn_Disconnect;
	HANDLE		oMutex;

#else
	int	(*fn_GetCard)(void *pDev);
	int	(*fn_Transmit)(void *pDev, unsigned char *ucpApduCmd, long lApduCmdLen, unsigned char *ucpApduRsp, long *lpApduRspLen);
	int	(*fn_Disconnect)(void *pDev);

	pthread_mutex_t		oMutex;
#endif

	int			iMutexFlag;

}stHndICAO, *sHndICAOPtr;


int ICAOInit(stHndBioPtr	opSBIO);
void ICAOClean(sHndICAOPtr opSICAO);
int ICAOAddCaptureField(sHndICAOPtr opSICAO, int iFieldID);
int ICAOCleanCaptureFields(sHndICAOPtr opSICAO);
int ICAOCapture(sHndICAOPtr opSICAO, int iDeviceID, int iTimeout);
int ICAOMOC(sHndICAOPtr opSICAO, int iDeviceID, int iFingerRef, int iFieldID, int *ipMatchResult);

#endif

