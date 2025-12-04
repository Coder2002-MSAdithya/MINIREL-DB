#include "../include/pinrel.h"
#include "../include/globals.h"
#include "../include/defs.h"

void PinRel(int relNum)
{
    (catcache[relNum].status |= PINNED_MASK);
}