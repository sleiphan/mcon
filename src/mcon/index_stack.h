// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#ifndef MCON_EPOLL_ENTRY_STACK
#define MCON_EPOLL_ENTRY_STACK

#define STACK_NAME idx_stack
#define STACK_TYPE unsigned int
#define STACK_INDEX unsigned int
#include <ctools/stack.h>
#undef STACK_NAME
#undef STACK_TYPE
#undef STACK_INDEX

#endif // MCON_EPOLL_ENTRY_STACK
