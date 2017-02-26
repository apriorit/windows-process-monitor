/*
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Copyright (c) 1999 
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted 
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */
#ifndef _STLP_ALLOC_C
#define _STLP_ALLOC_C

#ifndef _STLP_INTERNAL_ALLOC_H
#  include <stl/_alloc.h>
#endif

# if defined (_STLP_EXPOSE_GLOBALS_IMPLEMENTATION)

_STLP_BEGIN_NAMESPACE

template <int __inst>
void *  _STLP_CALL __malloc_alloc<__inst>::_S_oom_malloc(size_t __n)
{
  __oom_handler_type __my_malloc_handler;
  void * __result;

  for (;;) {
    __my_malloc_handler = __oom_handler;
    if (0 == __my_malloc_handler) { __THROW_BAD_ALLOC; }
    (*__my_malloc_handler)();
    __result = malloc(__n);
    if (__result) return(__result);
  }
#if defined(_STLP_NEED_UNREACHABLE_RETURN)
  return 0;
#endif

}

template <class _Alloc>
void *  _STLP_CALL __debug_alloc<_Alloc>::allocate(size_t __n) {
  size_t __real_n = __n + __extra_before_chunk() + __extra_after_chunk();
  __alloc_header *__result = (__alloc_header *)__allocator_type::allocate(__real_n);
  memset((char*)__result, __shred_byte, __real_n*sizeof(value_type));
  __result->__magic = __magic;
  __result->__type_size = sizeof(value_type);
  __result->_M_size = (_STLP_UINT32_T)__n;
  return ((char*)__result) + (long)__extra_before;
}

# if ( _STLP_STATIC_TEMPLATE_DATA > 0 )
// malloc_alloc out-of-memory handling
template <int __inst>
__oom_handler_type __malloc_alloc<__inst>::__oom_handler=(__oom_handler_type)0 ;
# else /* ( _STLP_STATIC_TEMPLATE_DATA > 0 ) */
__DECLARE_INSTANCE(__oom_handler_type, __malloc_alloc<0>::__oom_handler, =0);
#  endif /* _STLP_STATIC_TEMPLATE_DATA */

_STLP_END_NAMESPACE

# undef _S_FREELIST_INDEX

# endif /* _STLP_EXPOSE_GLOBALS_IMPLEMENTATION */

#endif /*  _STLP_ALLOC_C */

// Local Variables:
// mode:C++
// End:
