#include <errno.h>

static int app_errno;

int *__errno(void)
{
    return &app_errno;
}
