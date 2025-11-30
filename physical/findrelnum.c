/************************INCLUDES*******************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


/*------------------------------------------------------------

FUNCTION FindRelNum (relName)

PARAMETER DESCRIPTION:
    relName → Name of the relation as a null-terminated string.

FUNCTION DESCRIPTION:
    Scans the catalog cache to locate whether a relation is currently open. 
    If the cache entry is valid and its relName matches the requested name, the function returns the index of that cache slot.

ALGORITHM:
    1. Loop over all MAXOPEN cache entries.
    2. For each entry:
         a. Check whether VALID_MASK is set → relation is open.
         b. Compare relName with relcat_rec.relName using string compare function.
    3. If match found → return the cache index.
    4. If no match after scanning → return NOTOK.

ERRORS REPORTED:
    - None.

GLOBAL VARIABLES MODIFIED:
    - None.

------------------------------------------------------------*/

int FindRelNum(const char *relName)
{
    for(int i = 0; i < MAXOPEN; i++)
    {
        if((catcache[i].status & VALID_MASK) && strncmp(catcache[i].relcat_rec.relName, relName, RELNAME) == 0)
        {
            return i;
        }
    }
    
    return NOTOK;
}