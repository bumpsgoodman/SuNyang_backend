// 작성자: bumpsgoodman

#ifndef __SAFE_FREE_H
#define __SAFE_FREE_H

#include <stdlib.h>

#define SAFE_FREE(p)        { if ((p)) { free((p)); (p) = NULL; } }
#define SAFE_RELEASE(p)     { if ((p)) { (p)->Release((p)); (p) = NULL; } }

#endif // __SAFE_FREE_H