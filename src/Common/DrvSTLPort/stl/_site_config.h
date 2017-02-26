// 
// This file defines site configuration.
//
//

// compatibility section

#define _STLP_DRIVERSTUDIO_NO_THROW
#define _STLP_NO_EXCEPTIONS
#define _STLP_NO_NATIVE_MBSTATE_T
#define _STLP_NO_CSTD_FUNCTION_IMPORTS
#define _STLP_NO_IOSTREAMS
#define _STLP_NO_OWN_NAMESPACE        // required for AMD64
#define _STLP_GLOBAL_NEW_HANDLER
#define _STLP_USE_STATIC_LIB
#define _NOTHREADS 
#define _STLP_USE_NEWALLOC
#define _STLP_USE_RAW_SGI_ALLOCATORS 

#ifdef _IA64_
#define _STLP_NESTED_TYPE_PARAM_BUG
#endif


# if defined (_STLP_NO_IOSTREAMS) || defined (_STLP_NO_NEW_IOSTREAMS) && ! defined ( _STLP_NO_OWN_IOSTREAMS )
#  define _STLP_NO_OWN_IOSTREAMS
# endif

# if !defined (_STLP_NO_OWN_IOSTREAMS) &&  ! defined (_STLP_OWN_IOSTREAMS)
#  define _STLP_OWN_IOSTREAMS
# endif

# if (defined (_STLP_NOTHREADS) || defined (_STLP_NO_THREADS) || defined (NOTHREADS))
#  if ! defined (_NOTHREADS)
#   define _NOTHREADS
#  endif
#  if ! defined (_STLP_NO_THREADS)
#   define _STLP_NO_THREADS
#  endif
# endif

/* 
 * _STLP_USE_OWN_NAMESPACE/_STLP_NO_OWN_NAMESPACE
 * If defined, STLport uses _STL:: namespace, else std::
 * The reason you have to use separate namespace in wrapper mode is that new-style IO
 * compiled library may have its own idea about STL stuff (string, vector, etc.),
 * so redefining them in the same namespace would break ODR and may cause
 * undefined behaviour. Rule of thumb is - if new-style iostreams are
 * available, there WILL be a conflict. Otherwise you should be OK.
 * In STLport iostreams mode, there is no need for this flag other than to facilitate
 * link with third-part libraries compiled with different standard library implementation.
 */
// #  define _STLP_USE_OWN_NAMESPACE 1
// #  define _STLP_NO_OWN_NAMESPACE  1

#define   _STLP_NO_DEBUG_EXCEPTIONS 1

/* 
 * Uncomment that to disable exception handling code 
 */
// #define   _STLP_NO_EXCEPTIONS 1

/*
 * _STLP_NO_NAMESPACES: if defined, don't put the library in namespace
 * stlport:: or std::, even if the compiler supports namespaces
 */
// #define   _STLP_NO_NAMESPACES 1

# define _STLP_INCOMPLETE_EXCEPTION_HEADER

