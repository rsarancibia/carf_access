#include <Client_Info.h>

#include <windows.h>
#include <wbemidl.h>
#include <stdio.h>

#pragma comment(lib, "wbemuuid.lib")

// ------------------------------------------------------------
// INIT WMI
// ------------------------------------------------------------
HRESULT init_wmi(IWbemServices** pSvc)
{
    HRESULT hr;

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    PutInLog(NULL, 0, "CoInitializeEx hr = 0x%lx", hr);

    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return hr;

    hr = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);

    PutInLog(NULL, 0, "CoInitializeSecurity hr = 0x%lx", hr);

    if (FAILED(hr) && hr != RPC_E_TOO_LATE)
        return hr;

    IWbemLocator* pLoc = NULL;

    hr = CoCreateInstance(
        &CLSID_WbemLocator, 0,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator, (LPVOID*)&pLoc);

    PutInLog(NULL, 0, "CoCreateInstance hr = 0x%lx", hr);

    if (FAILED(hr)) return hr;

    hr = pLoc->lpVtbl->ConnectServer(
        pLoc,
        L"ROOT\\CIMV2",
        NULL, NULL, 0,
        0, 0, 0,
        pSvc);

    PutInLog(NULL, 0, "ConnectServer hr = 0x%lx", hr);

    pLoc->lpVtbl->Release(pLoc);

    if (FAILED(hr)) return hr;

    hr = CoSetProxyBlanket(
        (IUnknown*)(*pSvc),
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE);

    PutInLog(NULL, 0, "CoSetProxyBlanket hr = 0x%lx", hr);

    return hr;
}

// ------------------------------------------------------------
// CPU ID
// ------------------------------------------------------------
int get_cpu_id(char* out, int max_len)
{
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;

    PutInLog(NULL, 0, "Get CPU ID - STP0");

    if (FAILED(init_wmi(&pSvc)))
        return 0;

    PutInLog(NULL, 0, "Get CPU ID - STP1");

    HRESULT hr = pSvc->lpVtbl->ExecQuery(
        pSvc,
        L"WQL",
        L"SELECT ProcessorId FROM Win32_Processor",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    PutInLog(NULL, 0, "Get CPU ID - STP2");

    if (FAILED(hr))
        return 0;

    PutInLog(NULL, 0, "Get CPU ID - STP3");

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    if (pEnumerator->lpVtbl->Next(pEnumerator, WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK)
    {
        VARIANT vtProp;
        VariantInit(&vtProp);

        hr = pclsObj->lpVtbl->Get(pclsObj, L"ProcessorId", 0, &vtProp, 0, 0);

        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR && vtProp.bstrVal != NULL)
        {
            WideCharToMultiByte(
                CP_UTF8,
                0,
                vtProp.bstrVal,
                -1,
                out,
                max_len,
                NULL,
                NULL);
        }

        VariantClear(&vtProp);
        pclsObj->lpVtbl->Release(pclsObj);
    }

    PutInLog(NULL, 0, "Get CPU ID - STP4");

    if (pEnumerator) pEnumerator->lpVtbl->Release(pEnumerator);
    if (pSvc) pSvc->lpVtbl->Release(pSvc);

    PutInLog(NULL, 0, "Get CPU ID - STP5");

    return 1;
}

// ------------------------------------------------------------
// DISK SERIAL
// ------------------------------------------------------------
int get_disk_serial(char* out, int max_len)
{
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;

    PutInLog(NULL, 0, "Get Disk Serial - STP0");

    if (FAILED(init_wmi(&pSvc)))
        return 0;

    PutInLog(NULL, 0, "Get Disk Serial - STP1");

    HRESULT hr = pSvc->lpVtbl->ExecQuery(
        pSvc,
        L"WQL",
        L"SELECT VolumeSerialNumber FROM Win32_LogicalDisk WHERE DeviceID='C:'",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    PutInLog(NULL, 0, "Get Disk Serial - STP2");

    if (FAILED(hr))
        return 0;

    PutInLog(NULL, 0, "Get Disk Serial - STP3");

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    PutInLog(NULL, 0, "Get Disk Serial - STP4");

    if (pEnumerator->lpVtbl->Next(pEnumerator, WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK)
    {
        VARIANT vtProp;
        VariantInit(&vtProp);

        hr = pclsObj->lpVtbl->Get(pclsObj, L"VolumeSerialNumber", 0, &vtProp, 0, 0);

        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR && vtProp.bstrVal != NULL)
        {
            WideCharToMultiByte(
                CP_UTF8,
                0,
                vtProp.bstrVal,
                -1,
                out,
                max_len,
                NULL,
                NULL);
        }

        VariantClear(&vtProp);
        pclsObj->lpVtbl->Release(pclsObj);
    }

    if (pEnumerator) pEnumerator->lpVtbl->Release(pEnumerator);
    if (pSvc) pSvc->lpVtbl->Release(pSvc);

    PutInLog(NULL, 0, "Get Disk Serial - STP5");

    return 1;
}

// ------------------------------------------------------------
// BUILD HWID
// ------------------------------------------------------------
int build_hwid(char* out)
{
    char cpu[128] = { 0 };
    char disk[128] = { 0 };
	int status = 0;

    while (TRUE)
    {
        if (out == NULL)
        {
            status = 1;
            break;
        }
        
        if (get_cpu_id(cpu, sizeof(cpu)) == 0)
        {
			status = 2;
            break;
        }
        
        
        if(get_disk_serial(disk, sizeof(disk)) == 0)
        {
			status = 3;
            break;
        }

        //PutInLog(NULL, 0, "Cpu : %d", strlen(cpu));
        //PutInLog(NULL, 0, "Disk : %d", strlen(disk));

        PutInLog(NULL, 0, "Cpu : %s", cpu);
        PutInLog(NULL, 0, "Disk : %s", disk);


        _strupr(cpu);
        _strupr(disk);

        sprintf(out, "%s|%s", cpu, disk);

        //unsigned char hash[32];

        //SHA256((unsigned char*) out, strlen(out), hash);

        //PutInLog(NULL, 0, "HASHHHH !!!!");
        //DisplayHex(0, hash, sizeof(hash));

        break;
    }

	return status;
}