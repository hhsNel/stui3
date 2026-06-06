#ifndef STUI_ERRNO_H
#define STUI_ERRNO_H

#include "stui.h"

extern stui_result stui_errno;

#define IF_ERROR_RETURN(VAL) do { if(!STUI_IS_OK(stui_errno)) return (VAL); } while(0);

#endif

