#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include "processdll.h"

namespace Callbacks
{
	void __stdcall ListCallback(
		PCHECK_LIST_ENTRY CheckList[],
		ULONG CheckListCount
		);

}

#endif // __CALLBACKS_H__