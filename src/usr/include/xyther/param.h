#pragma once

#include <stddef.h>
#include <limits.h>
#include <xyther/signal.h>
#include <xyther/types.h>

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER    __LITLE_ENDIAN

#define BIG_ENDIAN __BIG_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BYTE_ORDER __BYTE_ORDER
