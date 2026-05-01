#ifndef SECURE_TOOLS_H
#define SECURE_TOOLS_H

#include <stdint.h>
#include <windows.h>
#include <winhttp.h>
#include <time.h>
#include <string.h>
#include <stdio.h>



typedef struct
{
    uint64_t last_valid_time;
    uint64_t expiry_time;
} license_state_t;

int load_state(const char* path, license_state_t* st);
int save_state(const char* path, license_state_t* st);


int get_http_date(char* buffer, DWORD size);


#endif