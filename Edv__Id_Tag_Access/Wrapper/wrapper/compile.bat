@echo off

REM set OPENSSL_DIR=I:\RProteus\Cedula\OpenPace\openssl_1_0_2

REM Verificar exports !!!
REM dumpbin /exports openpace_wrapper.dll

REM SOLEMBIO_DATAFIELD_MAX ??


REM /link ^
REM /OUT:openpace_wrapper.dll ^
REM /IMPLIB:openpace_wrapper.lib ^
REM /NODEFAULTLIB:LIBCMT

REM copy /Y openpace_wrapper.dll "%DEST_DIR%"
REM copy /Y openpace_wrapper.lib "%DEST_DIR%"


set home=1

if "%home%"=="1" (

	echo.
	echo			Ejecutando en HOME
	echo.

	set BASE_DIR=I:\RProteus\Cedula\Cedula_Access\Edv__Id_Tag_Access

) else (

	echo.
	echo			Ejecutando en ACER
	echo.

	set BASE_DIR=d:\RProteus\Cedula\carf_access\Edv__Id_Tag_Access
)

set OPENSSL_DIR=""
set OPENSSL_LIB=""
set OPENPACE_SRC_DIR=""
set NFC_DIR=""
set DEST_DIR=""

set OPENSSL_DIR=%BASE_DIR%\Wrapper\openssl_1_0_2
set OPENSSL_LIB=%OPENSSL_DIR%\static\lib\libeay32.lib
set OPENPACE_SRC_DIR=%BASE_DIR%\Wrapper\openpace\src
set NFC_DIR=%BASE_DIR%\Wrapper\wrapper\nfc_access
set DEST_DIR=%BASE_DIR%\lib

echo.

echo OPENSSL_DIR      : %OPENSSL_DIR%
echo OPENSSL_LIB      : %OPENSSL_LIB%
echo OPENPACE_SRC_DIR : %OPENPACE_SRC_DIR%
echo NFC_DIR          : %NFC_DIR%
echo DEST_DIR         : %DEST_DIR%

echo.


cl /LD /MD ^
/I %OPENSSL_DIR%\include ^
/I . ^
/I .\eac ^
/I %OPENPACE_SRC_DIR% ^
/I %NFC_DIR%\include ^
openpace_wrapper.c ^
%NFC_DIR%\utils.c ^
%NFC_DIR%\coder.c ^
%NFC_DIR%\BIoGetData.c ^
%NFC_DIR%\Tag_ICAO.c ^
%NFC_DIR%\all_smart_card.c ^
%NFC_DIR%\EdvBio_Wrapper.c ^
%OPENPACE_SRC_DIR%\iob_fix.c ^
%OPENPACE_SRC_DIR%\libeacMT.lib ^
%OPENSSL_LIB% ^
ws2_32.lib ^
crypt32.lib ^
advapi32.lib ^
user32.lib ^
gdi32.lib ^
legacy_stdio_definitions.lib ^
/link ^
/NODEFAULTLIB:LIBCMT



REM --- Verificar resultado ---
if %ERRORLEVEL% neq 0 (
    echo.
	echo ERROR: Fallo la compilacion
	echo.
    exit /b %ERRORLEVEL%
)

echo.
echo Compilacion OK !!!!!!!!
echo.


REM --- Copiar DLL ---
REM set DEST_DIR=i:\RProteus\Cedula\Cedula_Access\Edv__Id_Tag_Access\lib\

if not exist "%DEST_DIR%\" (
    mkdir "%DEST_DIR%"
)

copy /Y openpace_wrapper.dll "%DEST_DIR%\"

echo DLL copiada a %DEST_DIR%
echo.
echo.