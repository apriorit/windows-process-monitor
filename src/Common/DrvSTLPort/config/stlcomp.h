/*
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

/*
 * Purpose of this file :
 *
 * To hold COMPILER-SPECIFIC portion of STLport settings.
 * In general, user should not edit this file unless 
 * using the compiler not recognized below.
 *
 * If your compiler is not being recognized yet, 
 * please look for definitions of macros in stl_mycomp.h,
 * copy stl_mycomp.h to stl_YOUR_COMPILER_NAME, 
 * adjust flags for your compiler, and add  <include config/stl_YOUR_COMPILER_NAME>
 * to the secton controlled by unique macro defined internaly by your compiler.
 *
 * To change user-definable settings, please edit <../stl_user_config.h> 
 *
 */

#ifndef _STLP_COMP_H
# define _STLP_COMP_H

/* distinguish real MSC from Metrowerks and Intel */
# if defined(_MSC_VER) && !defined(__MWERKS__) && !defined (__ICL) && !defined (__COMO__)
#  define _STLP_MSVC _MSC_VER
# endif

# if defined(_STLP_MSVC)
/* Microsoft Visual C++ 4.0, 4.1, 4.2, 5.0 */
#  include <config/stl_msvc.h>
# else
/* Unable to identify the compiler, issue error diagnostic.
 * Edit <config/stl_mycomp.h> to set STLport up for your compiler. */
//#  include <config/stl_mycomp.h>
# endif /* end of compiler choice */

#endif

