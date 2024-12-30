#ifndef _RC522__H
#define _RC522__H

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"
void rc522_Init();
void rc522_read_cardid(void);
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif

