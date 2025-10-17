#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


int FindRelNum(const char *relName)
{
    for(int i = 0; i < MAXOPEN; i++)
    {
        if((catcache[i].status & VALID_MASK) && strcmp(catcache[i].relcat_rec.relName, relName) == 0)
        {
            return i;  // Found, return relation number (index)
        }
    }
    
    return NOTOK; // Not found
}