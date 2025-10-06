#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


/* Close all system catalogs */
int CloseCats() 
{
    // Loop through the catalog cache and flush if dirty
    for (int i = 0; i < MAXOPEN; i++) 
    {
        if (catcache[i].relFile > 0) 
        {  
            // file is open
            if (catcache[i].dirty) 
            {
                FlushPage(i);  
            }

            // Close the file descriptor
            if (close(catcache[i].relFile) != 0) 
            {
                return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
            }

            // Reset cache entry
            catcache[i].relFile = -1;
            catcache[i].dirty = 0;
            catcache[i].valid = 0;
            catcache[i].attrList = NULL;
            catcache[i].relcat_rec.relName[0] = '\0';
            catcache[i].relcat_rec.recLength = 0;
            catcache[i].relcat_rec.recsPerPg = 0;
            catcache[i].relcat_rec.numAttrs = 0;
            catcache[i].relcat_rec.numRecs = 0;
            catcache[i].relcat_rec.numPgs = 0;
            catcache[i].relcatRid.pid = -1;
            catcache[i].relcatRid.slotnum = 0;
        }
    }

    db_open = false;  // mark database as closed

    return OK;
}