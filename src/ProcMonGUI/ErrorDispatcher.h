#pragma once

class ErrorDispatcher
{
public:
	ErrorDispatcher(void);
	~ErrorDispatcher(void);

	static bool Dispatch(int error)
	{
		TCHAR *message = TEXT("");
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
};
