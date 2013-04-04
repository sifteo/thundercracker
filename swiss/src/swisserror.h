#ifndef SWISS_ERROR_H
#define SWISS_ERROR_H

#include <errno.h>

/*
 * For lack of a better convention, use errno codes
 * for our process exit code.
 *
 * Define a symbol for the OK case just for clarity.
 */

#ifndef EOK
#define EOK 0
#endif

#endif // SWISS_ERROR_H
