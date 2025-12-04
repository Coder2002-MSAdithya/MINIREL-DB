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
    relName → pointer to the name of a relation to be searched for in the catcache[].

FUNCTION DESCRIPTION:
    The routine scans the open-relation table (catcache), checking each slot to determine whether the named relation is currently open. 
    If it is found, the routine returns the corresponding cache index (relNum). 
    If not found, it returns NOTOK.
    Only relations whose cache entry is marked VALID_MASK are considered. 
    Comparisons use the stored relName inside each RelCatRec.

ALGORITHM:
    1) Iterate over all MAXOPEN cache slots.
    2) For each slot, test whether it is valid (slot in use).
    3) If valid, compare the stored relation name with relName.
    4) If identical, return the index of that slot.
    5) If the full scan completes without a match, return NOTOK.

BUGS:
    None found.

ERRORS REPORTED:
    None.

GLOBAL VARIABLES MODIFIED:
    None.

IMPLEMENTATION NOTES:
    • Fast O(MAXOPEN) lookup.
    • Caller must interpret NOTOK as “relation not open.”

------------------------------------------------------------*/

int FindRelNum(const char *relName)
{
    for(int i = 0; i < MAXOPEN; i++)
    {
        if((catcache[i].status & VALID_MASK) && strncmp(catcache[i].relcat_rec.relName, relName, RELNAME) == 0)
        {
            return i;  // Found, return relation number (index)
        }
    }
    
    return NOTOK; // Not found
}