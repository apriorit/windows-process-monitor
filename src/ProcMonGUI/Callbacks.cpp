#include "stdafx.h"
#include "Callbacks.h"
#include "CallBackList.h"
#include "ProcMonGUI.h"

namespace Callbacks
{
	void __stdcall ListCallback(
		PCHECK_LIST_ENTRY CheckList[],
		ULONG CheckListCount
		)
	{
		if(CheckList != NULL && CheckListCount > 0)
		{
			CheckedProcesses listedProcesses;

			for (UINT i = 0; i < CheckListCount; i++)
			{
				CheckProcessInfo checkProcessInfo;
				checkProcessInfo.isDeny = CheckList[i]->AddToBlacklist != FALSE;
				checkProcessInfo.process.pid = CheckList[i]->ProcessInfo->ProcessId;
				checkProcessInfo.process.imageName = CheckList[i]->ProcessInfo->ImagePath;

				listedProcesses.push_back(checkProcessInfo);
			}
			CCallBackListForm callBackGreyDlg(listedProcesses, ::theApp.GetMainWnd());
			INT_PTR nResponse = callBackGreyDlg.DoModal();        

			listedProcesses = callBackGreyDlg.GetCheckedProcessesList();

			for (UINT i = 0; i < CheckListCount; i++)
			{
				CheckList[i]->AddToBlacklist = listedProcesses.at(i).isDeny ? TRUE : FALSE;
			}        
		}
	}
}