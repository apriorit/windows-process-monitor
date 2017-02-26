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

#if !defined(NEW_H)
#define NEW_H

#include <new>
void* __cdecl operator new(size_t size);
void __cdecl operator delete(void* p);

void* __cdecl operator new(size_t size, void *place);
void __cdecl operator delete(void* p, void *place);

#if defined(_WDMDDK_) || defined(_NTDDK_) || defined(_NTIFS_)
void* __cdecl operator new(size_t size, POOL_TYPE type);
void __cdecl operator delete(void* p, POOL_TYPE type);
#endif

#endif // !defined(NEW_H)
