#include "Callbacks.h"
#include "CallBackList.h"
#include "ProcMonGUI.h"

namespace Callbacks
{
    void __stdcall ListCallback(
        PCHECK_LIST_ENTRY checkList[],
        ULONG checkListCount
        )
    {
        if (checkList && checkListCount > 0)
        {
            CheckedProcesses listedProcesses{};

            for (ULONG i = 0; i < checkListCount; ++i)
            {
                CheckProcessInfo checkProcessInfo{};
                checkProcessInfo.isDeny = checkList[i]->AddToBlacklist;
                checkProcessInfo.process.pid = checkList[i]->ProcessInfo->ProcessId;
                checkProcessInfo.process.imageName = checkList[i]->ProcessInfo->ImagePath;

                listedProcesses.push_back(checkProcessInfo);
            }

            CCallBackListForm callBackGreyDlg(listedProcesses, ::theApp.GetMainWnd());
            INT_PTR nResponse = callBackGreyDlg.DoModal();

            listedProcesses = callBackGreyDlg.GetCheckedProcessesList();

            for (ULONG i = 0; i < checkListCount; ++i)
            {
                checkList[i]->AddToBlacklist = listedProcesses.at(i).isDeny;
            }
        }
    }
}