// 작성자: bumpsgoodman

#ifndef __ASSERT_H
#define __ASSERT_H

#ifdef NDEBUG
#define ASSERT(cond, msg) ((void*)(0))
#else
#define ASSERT(cond, msg) { if (!(cond)) { __builtin_trap(); }}
#endif // NDEBUG

#endif // __ASSERT_H