#ifndef ERRET

#include <errno.h>

#define ERRET(condition,error_val) if(##condition) { errno = ##error_val; return -1; }

#endif
