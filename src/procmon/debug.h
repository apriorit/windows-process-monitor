#ifndef __DEBUG_H__
#define __DEBUG_H__

#if DBG
//
// Debug levels: higher values indicate higher urgency
//
#define DBG_LEVEL_VERBOSE 1
#define DBG_LEVEL_TERSE   2
#define DBG_LEVEL_WARNING 3
#define DBG_LEVEL_ERROR   4

static INT DriverDbgLevel = 2;

#define DBGPRINTEX(Level, Prefix, Type, Fmt)                        \
{                                                                   \
    if (DriverDbgLevel <= (Level))                                  \
{                                                                   \
    DbgPrint("%s%s %s (%d): ", Prefix, Type, __FILE__, __LINE__);   \
    DbgPrint Fmt;                                                   \
}                                                                   \
}

#define DBGPRINT(Level, Prefix, Fmt)                                \
{                                                                   \
    if ((Level) >= DriverDbgLevel)                                  \
{                                                                   \
    DbgPrint("%s", Prefix);                                   \
    DbgPrint Fmt;                                                   \
}                                                                   \
}

#define VERBOSE_FN(Fmt) DBGPRINT(DBG_LEVEL_VERBOSE, __FUNCTION__"(): ", Fmt)
#define TERSE_FN(Fmt) DBGPRINT(DBG_LEVEL_TERSE, __FUNCTION__"(): ", Fmt)
#define WARNING_FN(Fmt) DBGPRINTEX(DBG_LEVEL_WARNING, __FUNCTION__"(): ", "WRN", Fmt)
#define ERR_FN(Fmt) DBGPRINTEX(DBG_LEVEL_ERROR, __FUNCTION__"(): ", "ERR", Fmt)

#else

#define VERBOSE_FN(Fmt)
#define TERSE_FN(Fmt)
#define WARNING_FN(Fmt)
#define ERR_FN(Fmt)

#define DBGPRINTEX(Level, Fmt)
#define DBGPRINT(Fmt)
#endif // DBG

#endif //__DEBUG_H__