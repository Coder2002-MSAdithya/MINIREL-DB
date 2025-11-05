#include <string.h>
#include "../include/defs.h"
#include "../include/globals.h"

AttrDesc *FindRelAttr(int relNum, char *attrName)
{
    AttrDesc *ptr = catcache[relNum].attrList;

    for(;ptr;ptr=ptr->next)
    {
        if(strncmp((ptr->attr).attrName, attrName, ATTRNAME) == OK)
        {
            return ptr;
        }
    }

    return NULL;
}