#ifndef __SERVICE_CONTROLLER_H__
#define __SERVICE_CONTROLLER_H__

#include "utils.h"

class ServiceController
{
public:
    static void Install(const std::wstring& ServiceName, 
                        const std::wstring& SourceBinaryPath, 
                        DWORD ServiceType = SERVICE_KERNEL_DRIVER, 
                        DWORD StartType = SERVICE_DEMAND_START, 
                        LPCTSTR lpDependencies = NULL);
    
    
    static void Remove(const std::wstring& ServiceName);

    static void Start(const std::wstring& ServiceName);
    
    static void Stop(const std::wstring& ServiceName);

    static void QueryStatus(const std::wstring& ServiceName, bool& Installed, bool& Running);
};

#endif // __SERVICE_CONTROLLER_H__