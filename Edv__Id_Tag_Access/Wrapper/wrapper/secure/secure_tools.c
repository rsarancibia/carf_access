#include <secure_tools.h>

#include <windows.h>
#include <winhttp.h>
#include <time.h>
#include <string.h>



#pragma comment(lib, "winhttp.lib")

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



#define LICENSE_OK 0
#define LICENSE_EXPIRED 1
#define LICENSE_ROLLBACK 2

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