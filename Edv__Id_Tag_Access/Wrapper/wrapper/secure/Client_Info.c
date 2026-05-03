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

    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return hr;

    hr = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);

    if (FAILED(hr) && hr != RPC_E_TOO_LATE)
        return hr;

    IWbemLocator* pLoc = NULL;

    hr = CoCreateInstance(
        &CLSID_WbemLocator, 0,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hr)) return hr;

    hr = pLoc->lpVtbl->ConnectServer(
        pLoc,
        L"ROOT\\CIMV2",
        NULL, NULL, 0,
        0, 0, 0,
        pSvc);

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

    return hr;
}

// ------------------------------------------------------------
// CPU ID
// ------------------------------------------------------------
int get_cpu_id(char* out, int max_len)
{
    int status = 0;
    
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;

    while (TRUE)
    {
        if (FAILED(init_wmi(&pSvc)))
        {
            status = 1;
            break;
        }

        HRESULT hr = pSvc->lpVtbl->ExecQuery(
            pSvc,
            L"WQL",
            L"SELECT ProcessorId FROM Win32_Processor",
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);

        if (FAILED(hr))
        {
            status = 2;
            break;
        }

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

        if (pEnumerator) pEnumerator->lpVtbl->Release(pEnumerator);
        if (pSvc) pSvc->lpVtbl->Release(pSvc);
    
        break;
    }


    return status;
}

// ------------------------------------------------------------
// DISK SERIAL
// ------------------------------------------------------------
int get_disk_serial(char* out, int max_len)
{
    int status = 0;
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;

    while(TRUE)
	{

        if (FAILED(init_wmi(&pSvc)))
        {
            status = 1;
            break;
        }

        HRESULT hr = pSvc->lpVtbl->ExecQuery(
            pSvc,
            L"WQL",
            L"SELECT VolumeSerialNumber FROM Win32_LogicalDisk WHERE DeviceID='C:'",
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);

        if (FAILED(hr))
        {
            status = 2;
            break;
        }


        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;

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

        break;
    }

    return status;
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
        
        if (get_cpu_id(cpu, sizeof(cpu)) != 0)
        {
			status = 2;
            break;
        }
        
        
        if(get_disk_serial(disk, sizeof(disk)) != 0)
        {
			status = 3;
            break;
        }

        _strupr(cpu);
        _strupr(disk);

        sprintf(out, "%s|%s", cpu, disk);
        break;
    }

	return status;
}