#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrel.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include <stdio.h>
#include <string.h>

int Join(int argc, char **argv)
{
    if (!db_open)
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

    if (s1 == NOTOK || s2 == NOTOK)
    {
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int d = FindRel(dstRelName);
    if (d)
    {
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *ad1 = FindRelAttr(s1, attrName1);
    AttrDesc *ad2 = FindRelAttr(s2, attrName2);

    if (!ad1 || !ad2)
    {
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char t1 = (ad1->attr).type;
    char t2 = (ad2->attr).type;

    if (t1 != t2)
    {
        db_err_code = INCOMPATIBLE_TYPES;
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *head1 = catcache[s1].attrList;
    AttrDesc *head2 = catcache[s2].attrList;

    AttrDesc *resHead = NULL, *resTail = NULL;

    // --- Step 1: Copy all attributes from first relation (including attrName1) ---
    for (AttrDesc *p = head1; p; p = p->next)
    {
        AttrDesc *newNode = (AttrDesc *)malloc(sizeof(AttrDesc));
        if (!newNode)
        {
            db_err_code = MEM_ALLOC_ERROR;
            return ErrorMsgs(db_err_code, print_flag);
        }

        newNode->attr = p->attr; // copy metadata
        newNode->next = NULL;

        if (resHead == NULL)
            resHead = resTail = newNode;
        else
        {
            resTail->next = newNode;
            resTail = newNode;
        }
    }

    // --- Step 2: Copy attributes from second relation, excluding join attribute ---
    for (AttrDesc *p = head2; p; p = p->next)
    {
        if (strncmp(p->attr.attrName, attrName2, ATTRNAME) == 0)
            continue; // skip join attr from second relation

        AttrDesc *newNode = (AttrDesc *)malloc(sizeof(AttrDesc));
        if (!newNode)
        {
            db_err_code = MEM_ALLOC_ERROR;
            return ErrorMsgs(db_err_code, print_flag);
        }

        newNode->attr = p->attr;
        newNode->next = NULL;

        // --- Step 3: Handle name conflicts safely (≤20 chars including '\0') ---
        bool duplicate = false;
        for (AttrDesc *q = resHead; q; q = q->next)
        {
            if (strncmp(q->attr.attrName, p->attr.attrName, ATTRNAME) == 0)
            {
                duplicate = true;
                break;
            }
        }

        if (duplicate)
        {
            char newName[ATTRNAME];
            snprintf(newName, ATTRNAME, "%s_%s", p->attr.attrName, src2RelName);

            // Ensure total length ≤ 19 (reserve 1 byte for '\0')
            newName[ATTRNAME - 1] = '\0';

            // Truncate safely if it exceeds max length
            if (strlen(newName) >= ATTRNAME)
                newName[ATTRNAME - 1] = '\0';

            strncpy(newNode->attr.attrName, newName, ATTRNAME - 1);
            newNode->attr.attrName[ATTRNAME - 1] = '\0';
        }

        // Append to result list
        if (resHead == NULL)
            resHead = resTail = newNode;
        else
        {
            resTail->next = newNode;
            resTail = newNode;
        }
    }

    /* --- Step 4: Create the new joined relation schema --- */
    int status = CreateFromAttrList(dstRelName, resHead);

    /* --- Step 5: Cleanup memory for the result schema list --- */
    AttrDesc *tmp;
    while (resHead)
    {
        tmp = resHead;
        resHead = resHead->next;
        free(tmp);
    }

    return status;
}
