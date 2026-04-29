#ifndef __TAG_ICAO_H__
#define __TAG_ICAO_H__

#include <generictypedefs.h>
#include <BioDefs.h>


#define ICAO_READ_CHUNK	215

#define ICAO_AUTH_BUG_RETRY	3

#define DEFAULT_ICAO_OPUPATION			(char*)"No informada"

#ifdef _WIN32
typedef int	(__cdecl *GETCARD)		(IN void *pDev);
typedef int	(__cdecl *TRANSMIT)		(IN void *pDev, IN unsigned char *ucpApduCmd, IN long lApduCmdLen, OUT unsigned char *ucpApduRsp, IN OUT long *lpApduRspLen);
typedef int	(__cdecl *DISCONNECT)	(IN void *pDev);
#endif


// typedef int (__cdecl *GETCARD)(void *pDev);

// typedef int (__cdecl *TRANSMIT)(
//     void *pDev,
//     unsigned char *ucpApduCmd,
//     long lApduCmdLen,
//     unsigned char *ucpApduRsp,
//     long *lpApduRspLen
// );

// typedef int (__cdecl *DISCONNECT)(void *pDev);


enum
{
	SOLEMICAO_DATAGROUP_COM = 0,
	SOLEMICAO_DATAGROUP_1,
	SOLEMICAO_DATAGROUP_2,
	SOLEMICAO_DATAGROUP_3,
	SOLEMICAO_DATAGROUP_4,
	SOLEMICAO_DATAGROUP_5,
	SOLEMICAO_DATAGROUP_6,
	SOLEMICAO_DATAGROUP_7,
	SOLEMICAO_DATAGROUP_8,
	SOLEMICAO_DATAGROUP_9,
	SOLEMICAO_DATAGROUP_10,
	SOLEMICAO_DATAGROUP_11,
	SOLEMICAO_DATAGROUP_12,
	SOLEMICAO_DATAGROUP_13,
	SOLEMICAO_DATAGROUP_14,
	SOLEMICAO_DATAGROUP_15,
	SOLEMICAO_DATAGROUP_MAX,
};


/*

stSolemBioPtr		pSBIO = NULL;
stLoggerPtr			pLogger = NULL;
int					iTmpApduRspLen = 0;
unsigned char		aApduMsg[255 + 6];
int					iApduMsgLen = 255 + 6;
unsigned char		aApduResponse[256];// +2];
int					iApduResponseLen = 256 + 2;

	pSBIO = opSICAO->pSBIO;
	pLogger = (stLoggerPtr)pSBIO->pLogger;
*/

#define SOLEMBIO_DATAFIELD_MAX 32 // !!!! ???????


typedef struct ststSolemBioPtr{

	void 	*pLogger;
	int		lReturn;
	int 	lError;
	void 	*hSolemICAO;
	void 	*hSCL;
	void 	**vpDeviceHandler;
	
	//stSolemICAOPtr			*opSICAO:


}stSolemBio, *stSolemBioPtr;




typedef struct ststSolemICAO{
	
	stSolemBioPtr	pSBIO;

	// SolemICAO Capture Fields
	int				aCaptureField[SOLEMBIO_DATAFIELD_MAX];
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

}stSolemICAO, *stSolemICAOPtr;


int SolemICAOInit(stSolemBioPtr	opSBIO);
void SolemICAOClean(stSolemICAOPtr opSICAO);
int SolemICAOAddCaptureField(stSolemICAOPtr opSICAO, int iFieldID);
int SolemICAOCleanCaptureFields(stSolemICAOPtr opSICAO);
int SolemICAOCapture(stSolemICAOPtr opSICAO, int iDeviceID, int iTimeout);
int SolemICAOMOC(stSolemICAOPtr opSICAO, int iDeviceID, int iFingerRef, int iFieldID, int *ipMatchResult);

#endif

