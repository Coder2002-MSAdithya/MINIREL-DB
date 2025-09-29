#include <stdio.h>
#include "../include/defs.h"
#include "../include/globals.h"

int CreateAttrCat()
{
    printf("CreateAttrCat\n");
    return OK;
}

int CreateRelCat()
{
    printf("CreateAttrCat\n");
    return OK;
}

int CreateCats()
{
    if(CreateRelCat() == OK && CreateAttrCat() == OK)
    {
        printf("CreateCats OK\n");
        return OK;
    }

    printf("CreateCats NOTOK");
    return NOTOK;
}