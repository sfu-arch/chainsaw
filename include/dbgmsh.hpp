#ifndef __DEBUGMSH__
#define __DEBUGMSH__

#define DBGVAR( os, var ) \
  (os) << "DBG: " << __FILE__ << "(" << __LINE__ << ") "\
       << #var << " = [" << (var) << "]" << std::endl

#define DBGMSG( os, msg ) \
  (os) << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
       << msg << std::endl

#define PRINT( os, msg ) \
  (os) << "PRINT: " << msg << std::endl


#ifdef MY_DEBUG
#  define D(x) x
#else
#  define D(x)
#endif // DEBUG


#endif
