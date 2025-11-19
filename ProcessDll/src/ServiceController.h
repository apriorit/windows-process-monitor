#pragma once

#include "Utils.h"

class ServiceController
{
public:
    static void Install(std::wstring_view serviceName,
                        std::wstring_view sourceBinaryPath,
                        DWORD serviceType = SERVICE_KERNEL_DRIVER,
                        DWORD startType = SERVICE_DEMAND_START,
                        std::wstring_view dependencies = {});

    static void Remove(std::wstring_view serviceName);

    static void Start(std::wstring_view serviceName);

    static void Stop(std::wstring_view serviceName);

    static void QueryStatus(std::wstring_view serviceName, bool& installed, bool& running);
};
