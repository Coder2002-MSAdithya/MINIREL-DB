#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/closerel.h"


/* Close all system catalogs */
int CloseCats() 
{
    if(!(catcache[0].status & VALID_MASK) || !(catcache[1].status & VALID_MASK))
    {
        return NOTOK;
    }

    for(int i=2;i<MAXOPEN;i++)
    {
        if(catcache[i].status & VALID_MASK)
        {
            CloseRel(i);
        }
    }

    /* Close relation attrcat */
    CloseRel(ATTRCAT_CACHE);

    /* Close relation relcat */
    CloseRel(RELCAT_CACHE);

    return OK;
}