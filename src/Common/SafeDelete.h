// 작성자: bumpsgoodman

#ifndef __SAFE_FREE_H
#define __SAFE_FREE_H

#define SAFE_FREE(p) { if ((p)) { free((p)); (p) = NULL; } }

#endif // __SAFE_FREE_H