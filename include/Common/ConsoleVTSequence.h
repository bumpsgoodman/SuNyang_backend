// 작성자: bumpsgoodman

#ifndef __CONSOLE_VT_SEQUENCE_H
#define __CONSOLE_VT_SEQUENCE_H

#include <stdio.h>

#define ESC "\x1b"

#define CONSOLE_VT_SET_DEFAULT_COLOR() (printf(ESC "[0m"))
#define CONSOLE_VT_SET_TEXT_COLOR(red, green, blue) (printf(ESC "[38;2;%d;%d;%dm", red, green, blue))
#define CONSOLE_VT_SET_BACKGROUND_COLOR(red, green, blue) (printf(ESC "[48;2;%d;%d;%dm", red, green, blue))

#endif // __CONSOLE_VT_SEQUENCE_H