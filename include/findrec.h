#ifndef _FINDREC_H
#define _FINDREC_H
#include "defs.h"
int FindRec(int relNum, Rid startRid, Rid *foundRid, void *recPtr, char attrType, int attrSize, int attrOffset, void *valuePtr, int compOp);
#endif
