// 작성자: bumpsgoodman

#ifndef __ASSERT_H
#define __ASSERT_H

#define ASSERT(cond, msg) { if (!(cond)) { __asm("int 3"); }}

#endif // __ASSERT_H