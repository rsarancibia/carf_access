#include <secure_tools.h>

//#include <windows.h>
//#include <winhttp.h>
//#include <time.h>
//#include <string.h>
//#include <stdio.h>




//🔐 Tamaño de salida(esto es lo importante)
//
//Si tu clave es :
//
//2048 bits → 256 bytes
//3072 bits → 384 bytes
//4096 bits → 512 bytes
//
//👉 Entonces :
//
//Firma RSA = tamaño fijo = tamaño de la clave

//🔍 Ejemplo real
//
//Con una clave de 2048 bits:
//
//data(cualquier tamaño)
//→ SHA256(interno)
//→ firma RSA
//
//resultado = 256 bytes
//
//👉 da lo mismo si DATA1 tiene 10 bytes o 10 KB
//👉 la firma será siempre 256 bytes



#pragma comment(lib, "winhttp.lib")


#define XOR_KEY 0x5A

int save_state(const char* path, license_state_t* st)
{
    FILE* f = fopen(path, "wb");
    if (!f) return 0;

    license_state_t tmp = *st;

    // ofuscación simple
    tmp.last_valid_time ^= XOR_KEY;
    tmp.expiry_time ^= XOR_KEY;

    fwrite(&tmp, sizeof(tmp), 1, f);
    fclose(f);
    return 1;
}

int load_state(const char* path, license_state_t* st)
{
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        memset(st, 0, sizeof(*st));
        return 0;
    }

    license_state_t tmp;

    fread(&tmp, sizeof(tmp), 1, f);
    fclose(f);

    // desofuscar
    tmp.last_valid_time ^= XOR_KEY;
    tmp.expiry_time ^= XOR_KEY;

    *st = tmp;
    return 1;
}



int get_http_date(char* buffer, DWORD size)
{
    HINTERNET hSession = WinHttpOpen(L"Lic/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) return 0;

    HINTERNET hConnect = WinHttpConnect(hSession, L"google.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);

    if (!hConnect) return 0;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"HEAD", NULL,
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest) return 0;

    if (!WinHttpSendRequest(hRequest, 0, 0, 0, 0, 0, 0))
        return 0;

    if (!WinHttpReceiveResponse(hRequest, NULL))
        return 0;

    DWORD len = size;

    int ok = WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_DATE,
        WINHTTP_HEADER_NAME_BY_INDEX,
        buffer,
        &len,
        WINHTTP_NO_HEADER_INDEX);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return ok;
}




time_t parse_http_date(const char* date_str)
{
    struct tm tm = { 0 };

    char wkday[4], month[4];
    int day, year, hour, min, sec;

    sscanf(date_str, "%3s, %d %3s %d %d:%d:%d",
        wkday, &day, month, &year, &hour, &min, &sec);

    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char* m = strstr(months, month);
    if (!m) return 0;

    int month_index = (int)((m - months) / 3);

    tm.tm_year = year - 1900;
    tm.tm_mon = month_index;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;

    return _mkgmtime(&tm); // importante: GMT
}


#define LICENSE_OK           0
#define LICENSE_INVALID      1
#define LICENSE_EXPIRED      2
#define LICENSE_ROLLBACK     3
#define LICENSE_NO_TIME      4

int validate_license(const char* state_path, uint64_t expiry_time)
{
    license_state_t st = { 0 };
    load_state(state_path, &st);

    char date[128] = { 0 };
    uint64_t current_time = 0;

    if (get_http_date(date, sizeof(date)))
    {
        time_t t = parse_http_date(date);
        if (t > 0)
            current_time = (uint64_t)t;
    }

    // fallback si no hay internet
    if (current_time == 0)
        current_time = st.last_valid_time;

    // primera ejecución
    if (st.last_valid_time == 0)
        st.last_valid_time = current_time;

    // 🔴 rollback detection
    if (current_time < st.last_valid_time)
        return LICENSE_ROLLBACK;

    // actualizar tiempo confiable
    if (current_time > st.last_valid_time)
        st.last_valid_time = current_time;

    // ⏳ expiración
    if (current_time > expiry_time)
        return LICENSE_EXPIRED;

    save_state(state_path, &st);

    return LICENSE_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////

//#include <stdint.h>
//#include <string.h>
//#include <time.h>

// ============================
// ESTRUCTURA INTERNA
// ============================

typedef struct
{
    uint64_t expiry_time;
    uint8_t  hwid_hash[32];   // opcional
    uint32_t flags;           // opcional
} license_info_t;


// ============================
// HOOKS QUE TÚ IMPLEMENTAS
// ============================

// 🔐 1. Base64 decode
int decode_base64(const char* input, uint8_t* output, int* out_len)
{
    // TODO: implementar
    return 0;
}

// 🔐 2. Verificación de firma
int verify_signature(const uint8_t* data, int data_len,
    const uint8_t* sig, int sig_len)
{
    // TODO: implementar (OpenSSL / propia)
    return 0;
}

// 🔐 3. Parseo de licencia
int parse_license(const uint8_t* data, int data_len, license_info_t* info)
{
    // TODO: implementar
    return 0;
}


// ============================
// TIEMPO INTERNET (ya lo tienes)
// ============================

extern int get_http_date(char* buffer, DWORD size);
extern time_t parse_http_date(const char* date_str);


// ============================
// FALLBACK OFFLINE (sin archivo visible)
// ============================

int load_cached_time(uint64_t* t)
{
    // TODO: puedes usar:
    // - registry
    // - memoria ofuscada
    // - múltiples ubicaciones
    return 0;
}

int save_cached_time(uint64_t t)
{
    // TODO: idem
    return 0;
}


// ============================
// FUNCIÓN PRINCIPAL
// ============================

int ValidateLicenseString(const char* license_b64)
{
    uint8_t decoded[2048];
    int decoded_len = 0;

    // 1. Base64 decode
    if (!decode_base64(license_b64, decoded, &decoded_len))
        return LICENSE_INVALID;

    // ----------------------------
    // 2. Separar DATA y SIGNATURE
    // ----------------------------
    // 👉 EJEMPLO: últimos 256 bytes = firma RSA
    int sig_len = 256;
    if (decoded_len <= sig_len)
        return LICENSE_INVALID;

    uint8_t* data = decoded;
    int data_len = decoded_len - sig_len;

    uint8_t* sig = decoded + data_len;

    // 3. Verificar firma
    if (!verify_signature(data, data_len, sig, sig_len))
        return LICENSE_INVALID;

    // 4. Parsear licencia
    license_info_t lic = { 0 };

    if (!parse_license(data, data_len, &lic))
        return LICENSE_INVALID;

    // ----------------------------
    // 5. Obtener tiempo confiable
    // ----------------------------
    char date[128] = { 0 };
    uint64_t current_time = 0;

    if (get_http_date(date, sizeof(date)))
    {
        time_t t = parse_http_date(date);
        if (t > 0)
            current_time = (uint64_t)t;
    }

    // ----------------------------
    // 6. Fallback offline
    // ----------------------------
    if (current_time == 0)
    {
        uint64_t cached = 0;

        if (load_cached_time(&cached) && cached > 0)
        {
            current_time = cached;
        }
        else
        {
            return LICENSE_NO_TIME;
        }
    }
    else
    {
        // guardar último tiempo válido
        save_cached_time(current_time);
    }

    // ----------------------------
    // 7. Validar expiración
    // ----------------------------
    if (current_time > lic.expiry_time)
        return LICENSE_EXPIRED;

    return LICENSE_OK;
}


