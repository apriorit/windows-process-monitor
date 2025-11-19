#pragma once

#include "Processdll.h"

namespace Callbacks
{
    void __stdcall ListCallback(
        PCHECK_LIST_ENTRY checkList[],
        ULONG checkListCount
        );
}
