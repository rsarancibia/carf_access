@echo off

REM set OPENSSL_DIR=I:\RProteus\Cedula\OpenPace\openssl_1_0_2

REM Verificar exports !!!
REM dumpbin /exports openpace_wrapper.dll

REM SOLEMBIO_DATAFIELD_MAX ??

set home=0

if "%home%"=="1" (

	echo Ejecutando en HOME

	set OPENSSL_DIR=I:\RProteus\Cedula\Cedula_Access\Edv__Id_Tag_Access\Wrapper\openssl_1_0_2
	set OPENSSL_LIB=%OPENSSL_DIR%\static\lib\libeay32.lib
	set OPENPACE_SRC_DIR=I:\RProteus\Cedula\Cedula_Access\Edv__Id_Tag_Access\Wrapper\openpace\src
	set NFC_DIR=I:\RProteus\Cedula\Cedula_Access\Edv__Id_Tag_Access\Wrapper\wrapper\nfc_access
	set DEST_DIR=I:\RProteus\Cedula\Cedula_Access\Edv__Id_Tag_Access\lib

) else (
    echo Ejecutando en ACER

	set OPENSSL_DIR=d:\RProteus\Cedula\carf_access\Edv__Id_Tag_Access\Wrapper\openssl_1_0_2
	set OPENSSL_LIB=%OPENSSL_DIR%\static\lib\libeay32.lib
	set OPENPACE_SRC_DIR=d:\RProteus\Cedula\carf_access\Edv__Id_Tag_Access\Wrapper\openpace\src
	set NFC_DIR=d:\RProteus\Cedula\carf_access\Edv__Id_Tag_Access\Wrapper\wrapper\nfc_access
	set DEST_DIR=d:\RProteus\Cedula\carf_access\Edv__Id_Tag_Access\lib

)

echo OPENSSL_DIR      : %OPENSSL_DIR%
echo OPENSSL_LIB      : %OPENSSL_LIB%
echo OPENPACE_SRC_DIR : %OPENPACE_SRC_DIR%
echo NFC_DIR          : %NFC_DIR%
echo DEST_DIR         : %DEST_DIR%


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












