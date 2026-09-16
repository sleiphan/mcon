include(CheckCSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES uring)



# Verify the existence of io_uring_initialize_sqe

check_c_source_compiles("
#include <liburing.h>

int main(void)
{
    struct io_uring_sqe sqe;
    io_uring_initialize_sqe(&sqe);
    return 0;
}
" MCON_HAS_IO_URING_INITIALIZE_SQE)

# Verify the existence of IORING_ACCEPT_DONTWAIT

check_c_source_compiles("
#include <liburing.h>

int main(void)
{
    struct io_uring_sqe sqe;
    sqe.ioprio = IORING_ACCEPT_DONTWAIT;
    return 0;
}
" MCON_HAS_IORING_ACCEPT_DONTWAIT)



# Produce error messages on failure

if(NOT MCON_HAS_IO_URING_INITIALIZE_SQE)
    message(FATAL_ERROR "mcon requires a liburing providing io_uring_initialize_sqe()")
endif()

if(NOT MCON_HAS_IORING_ACCEPT_DONTWAIT)
    message(FATAL_ERROR "mcon requires io_uring UAPI providing IORING_ACCEPT_DONTWAIT")
endif()