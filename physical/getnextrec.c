#include <stdio.h>
#include <string.h>
#include "../include/readpage.h"
#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/helpers.h"

int GetNextRec(int relNum, Rid startRid, Rid *foundRid, void *recPtr)
{
    int numPgs = catcache[relNum].relcat_rec.numPgs;
    int numRecs = catcache[relNum].relcat_rec.numRecs;
    int recsPerPg = catcache[relNum].relcat_rec.recsPerPg;
    int recSize = catcache[relNum].relcat_rec.recLength;
    char *page = buffer[relNum].page;
    
    Rid rid = IncRid(startRid, recsPerPg);
    *foundRid = (Rid){-1, -1};
    memset(recPtr, 0, recSize);

    while(rid.pid < numPgs)
    {
        if(ReadPage(relNum, rid.pid) == NOTOK)
        return NOTOK;

        unsigned long slotmap;
        memcpy(&slotmap, page+MAGIC_SIZE, sizeof(slotmap));

        if(slotmap & (1UL << rid.slotnum))
        {
            *foundRid = rid;
            memcpy(recPtr, page+HEADER_SIZE+recSize*rid.slotnum, recSize);
            return OK;
        }

        rid = IncRid(rid, recsPerPg);
    }

    return OK;
}
