#ifndef __EVWRAP_H__
#define __EVWRAP_H__

#define EV_STANDALONE              /* keeps ev from requiring config.h */
#define EV_USE_SELECT          1
#define EV_SELECT_IS_WINSOCKET 1   /* configure libev for windows select */

#ifdef __cplusplus
extern "C"
{
#include "ev.h"
}
#endif
#endif