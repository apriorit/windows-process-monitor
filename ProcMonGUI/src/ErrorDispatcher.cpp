#include "ErrorDispatcher.h"

bool ErrorDispatcher::Dispatch(int error)
{
    const TCHAR* message = TEXT("");
    switch (error)
    {
    case ERROR_SUCCESS:
        return true;
    default:
        message = _T("Unknown error");
        break;
    }

    message = _T("Error happened");
    MessageBox(0, message, _T("Error"), 0);

    return false;
}