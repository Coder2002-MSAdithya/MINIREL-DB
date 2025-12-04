#include "../include/unpinrel.h"
#include "../include/globals.h"
#include "../include/defs.h"

void UnPinRel(int relNum)
{
    (catcache[relNum].status &= (~PINNED_MASK));
}