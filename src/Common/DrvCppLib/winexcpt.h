/*
    This is a C++ run-time library for Windows kernel-mode drivers.
    Copyright (C) 2004 Bo Brantén.
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#if !defined(WINEXCPT_H)
#define WINEXCPT_H

/*
 * Class to translate from Windows structured exception handling (SEH) to C++ exceptions.
 *
 * Example:
 *
 * #include <winexcpt.h>
 *
 * WinException::install();
 *
 * try {
 *     volatile int *x = 0, y = *x; // dereference NULL pointer
 * }
 * catch (WinException e) {
 *     DbgPrint("Caught WinException %#x\n", e.ExceptionCode());
 * }
 *
 */

typedef void (__cdecl *_se_translator_function)(unsigned int, struct _EXCEPTION_POINTERS*);

extern "C" _se_translator_function __cdecl _set_se_translator(_se_translator_function);

class WinException
{
public:
    WinException(unsigned int ec, PEXCEPTION_POINTERS ep) : m_ExceptionCode(ec), m_ExceptionInformation(ep) {}
    unsigned int ExceptionCode() { return m_ExceptionCode; }
    PEXCEPTION_POINTERS ExceptionInformation() { return m_ExceptionInformation; }
    static _se_translator_function install() { return _set_se_translator(translator); }
    static void __cdecl translator(unsigned int ec, PEXCEPTION_POINTERS ep) { throw WinException(ec, ep); }
private:
    unsigned int        m_ExceptionCode;
    PEXCEPTION_POINTERS m_ExceptionInformation;
};

#endif // !defined(WINEXCPT_H)
