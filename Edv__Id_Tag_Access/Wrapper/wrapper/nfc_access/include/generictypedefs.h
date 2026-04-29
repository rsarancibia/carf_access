#ifndef	__GENERICTYPEDEFS_H
#define	__GENERICTYPEDEFS_H

#ifdef _WIN32
#ifndef _WIN32_WINNT					// Specifies that the minimum required platform is Windows Vista.
		#define _WIN32_WINNT 0x0600	// Change this to the appropriate value to target other versions of Windows.
	#endif
	#include <winsock2.h>
	#include <Windows.h>


#define mutex_destroy(h)						CloseHandle(h)
#define mutex_lock(h)							WaitForSingleObject(h, INFINITE)
#define mutex_unlock(h)							ReleaseMutex(h)

#else
#if defined(ANDROID_JNI)
#include <jni.h>
#endif
#include <pthread.h>
#endif
#include <stdio.h>


#include <openssl/evp.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#define EVP_MD_CTX_new()        EVP_MD_CTX_create()
#define EVP_MD_CTX_free(ctx)    EVP_MD_CTX_destroy(ctx)

#endif



//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
typedef int						Int1;
typedef char					tCHAR;
typedef int						tINT;
typedef short					tSHORT;
typedef long					tLONG;
typedef double					tDOUBLE;
typedef float					tFLOAT;

typedef signed char				sInt8;
typedef unsigned char			uInt8;
typedef signed short			sInt16;
typedef unsigned short			uInt16;
typedef signed int				sInt32;
typedef unsigned int			uInt32;
typedef signed long				sLong;
typedef unsigned long			uLong;
typedef unsigned long long		uInt64;
typedef signed long long		sInt64;

#endif

