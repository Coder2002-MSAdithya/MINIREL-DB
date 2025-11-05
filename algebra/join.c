#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrel.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include <stdio.h>


int Join(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *dstRelName = argv[1];
    char *src1RelName = argv[2];
    char *attrName1 = argv[3];
    char *src2RelName = argv[4];
    char *attrName2 = argv[5];

    int s1 = OpenRel(src1RelName);
    int s2 = OpenRel(src2RelName);

    if(s1 == NOTOK || s2 == NOTOK)
    {
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int d = FindRel(dstRelName);

    if(d)
    {
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }
    
    AttrDesc *ad1 = FindRelAttr(s1, attrName1);
    AttrDesc *ad2 = FindRelAttr(s2, attrName2);

    if(!ad1 || !ad2)
    {
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char t1 = (ad1->attr).type;
    char t2 = (ad1->attr).type;

    if(t1 != t2)
    {
        db_err_code = INCOMPATIBLE_TYPES;
        return ErrorMsgs(db_err_code, print_flag);
    }

    return OK;
}
