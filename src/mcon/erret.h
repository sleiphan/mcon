// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#ifndef ERRET

#include <errno.h>

#define ERRET(condition, error_val)                                                                \
    if (##condition) {                                                                             \
        errno = ##error_val;                                                                       \
        return -1;                                                                                 \
    }

#endif
