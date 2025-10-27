#ifndef _GETNEXTREC_H
#define _GETNEXTREC_H
#include "../include/defs.h"
int GetNextRec(int relNum, Rid startRid, Rid *foundRid, void *recPtr);
#endif