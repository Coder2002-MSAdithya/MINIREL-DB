#ifndef _BPTREE_H
#define _BPTREE_H
#include "../include/defs.h"
int CreateBPTree(int relNum, AttrCatRec *attrPtr);
int AddKeytoBPTree(int relNum, AttrCatRec *attrPtr, void *valuePtr, Rid insertRid);
int RemoveKeyfromBPTree(int relNum, AttrCatRec *attrPtr, Rid *removeRid);
int FindKeyWithBPTree(int relNum, AttrCatRec *attrPtr, Rid startRid, Rid *foundRid, void *valuePtr, int cmpOp);
int DestroyBPTree(int relNum, AttrCatRec *attrPtr);
int ReadBPTreePage(int relNum, AttrCatRec *attrPtr, short pidx);
int FlushBPTreePage(int relNum, AttrCatRec *attrPtr);
int DumpBPTree(int relNum, AttrCatRec *attrPtr);
#endif