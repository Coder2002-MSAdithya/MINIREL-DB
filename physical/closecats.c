#include <stdio.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/error.h"

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
        }
    }

    db_open = false;  // mark database as closed

    return OK;
}