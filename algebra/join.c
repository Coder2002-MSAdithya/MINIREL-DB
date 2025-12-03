#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrel.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include "../include/getnextrec.h"
#include "../include/insertrec.h"
#include "../include/createfromattrlist.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

void copy_attribute(void *srcRec, void *dstRec, AttrDesc **srcAttrDesc, AttrDesc **dstAttrDesc) 
{
    int dOffset = (*dstAttrDesc)->attr.offset;
    int sOffset = (*srcAttrDesc)->attr.offset;
    int sType = (*srcAttrDesc)->attr.type[0];
    int sSize = (*srcAttrDesc)->attr.length;
    
    writeAttrToRec((char *)dstRec, 
                   (char *)srcRec + sOffset, 
                   sType, 
                   sSize, 
                   dOffset);
    
    *dstAttrDesc = (*dstAttrDesc)->next;
    *srcAttrDesc = (*srcAttrDesc)->next;
}

/* Helper: normalize into buffer and print hex bytes for debugging */
static void normalize_str(const char *src, char *dst /* size ATTRNAME */)
{
    strncpy(dst, src, ATTRNAME - 1);
    dst[ATTRNAME - 1] = '\0';
}

static void hex_dump(const char *label, const char *s)
{
    char buf[ATTRNAME];
    normalize_str(s, buf);
    printf("[%s] string: '%s'  bytes:", label, buf);
    for (size_t i = 0; i < ATTRNAME; i++)
    {
        unsigned char c = (unsigned char)buf[i];
        printf(" %02x", c);
        if (c == 0) { break; }
    }
    printf("\n");
}

/* nameCollidesDetailedWithMatch:
   - resHead: attributes already in result
   - s1: relation number for src1 (or -1 to skip)
   - s2: relation number for src2 (or -1 to skip)
   - ad2: AttrDesc* to exclude from s2 checks (the join attribute)
   - exclude_s2: AttrDesc* to exclude from s2 checks (the current attribute node p)
   - outMatch: optional buffer to receive the matching existing name (size ATTRNAME)
   Returns:
     0 => no collision
     1 => collides with resHead
     2 => collides with s1
     3 => collides with s2 (non-ad2, non-exclude_s2)
*/
static int nameCollidesDetailedWithMatch(const char *name, AttrDesc *resHead, int s1, int s2, AttrDesc *ad2, AttrDesc *exclude_s2, char *outMatch /* size ATTRNAME */)
{
    char norm_name[ATTRNAME];
    normalize_str(name, norm_name);

    /* 1) check resHead */
    for (AttrDesc *q = resHead; q; q = q->next)
    {
        char qname[ATTRNAME];
        normalize_str(q->attr.attrName, qname);
        if (strcmp(qname, norm_name) == 0)
        {
            if (outMatch) strncpy(outMatch, qname, ATTRNAME);
            return 1;
        }
    }

    /* 2) check s1 */
    if (s1 >= 0)
    {
        for (AttrDesc *p = catcache[s1].attrList; p; p = p->next)
        {
            char pname[ATTRNAME];
            normalize_str(p->attr.attrName, pname);
            if (strcmp(pname, norm_name) == 0)
            {
                if (outMatch) strncpy(outMatch, pname, ATTRNAME);
                return 2;
            }
        }
    }

    /* 3) check s2 BUT explicitly ignore ad2 and exclude_s2 */
    if (s2 >= 0)
    {
        for (AttrDesc *p = catcache[s2].attrList; p; p = p->next)
        {
            if (p == ad2) continue;
            if (p == exclude_s2) continue; /* do not consider the same node as a collision */
            char pname[ATTRNAME];
            normalize_str(p->attr.attrName, pname);
            if (strcmp(pname, norm_name) == 0)
            {
                if (outMatch) strncpy(outMatch, pname, ATTRNAME);
                return 3;
            }
        }
    }

    return 0;
}

/* small wrapper boolean version */
static bool nameCollidesExcludingSelf(const char *name, AttrDesc *resHead, int s1, int s2, AttrDesc *ad2, AttrDesc *exclude_s2)
{
    return (nameCollidesDetailedWithMatch(name, resHead, s1, s2, ad2, exclude_s2, NULL) != 0);
}

/* ----- Corrected Join routine (diagnostic build) ----- */
int Join(int argc, char **argv)
{
    if (!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *dstRelName = argv[1];
    char *src1RelName = argv[2];
    char *attrName1   = argv[3];
    char *src2RelName = argv[4];
    char *attrName2   = argv[5];

    int s1 = OpenRel(src1RelName);
    int s2 = OpenRel(src2RelName);

    if (s1 == NOTOK)
    {
        printf("Relation '%s' does NOT exist in the DB.\n", src1RelName);
        printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), src1RelName, NULL);
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if (s2 == NOTOK)
    {
        printf("Relation '%s' does NOT exist in the DB.\n", src2RelName);
        printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), src2RelName, NULL);
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int d = FindRel(dstRelName);
    if (d)
    {
        printf("Relation '%s' already exists in the DB.\n", dstRelName);
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *ad1 = FindRelAttr(s1, attrName1);
    AttrDesc *ad2 = FindRelAttr(s2, attrName2);

    if (!ad1)
    {
        printf("Attribute '%s' NOT present in relation '%s' of the DB.\n", src1RelName, attrName1);
        printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), attrName1, src1RelName);
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if (!ad2)
    {
        printf("Attribute '%s' NOT present in relation '%s' of the DB.\n", src2RelName, attrName2);
        printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), attrName2, src2RelName);
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char t1 = (ad1->attr).type[0];
    char t2 = (ad2->attr).type[0];

    if (t1 != t2)
    {
        printf("Type '%c' of '%s.%s' incompatible with '%c' of '%s.%s'.\n",
               t1, src1RelName, attrName1, t2, src2RelName, attrName2);
        db_err_code = INCOMPATIBLE_TYPES;
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *head1 = catcache[s1].attrList;
    AttrDesc *head2 = catcache[s2].attrList;

    AttrDesc *resHead = NULL, *resTail = NULL;

    /* Step 1: copy attrs from relation1 */
    for (AttrDesc *p = head1; p; p = p->next)
    {
        AttrDesc *newNode = (AttrDesc *)malloc(sizeof(AttrDesc));
        if (!newNode)
        {
            db_err_code = MEM_ALLOC_ERROR;
            AttrDesc *tmp;
            while (resHead) { tmp = resHead; resHead = resHead->next; free(tmp); }
            return ErrorMsgs(db_err_code, print_flag);
        }
        newNode->attr = p->attr;
        newNode->attr.attrName[ATTRNAME - 1] = '\0';
        newNode->next = NULL;

        if (!resHead) resHead = resTail = newNode;
        else { resTail->next = newNode; resTail = newNode; }
    }

    /* Step 2: attrs from relation2 (exclude join attr), with rename if collision */
    const int reserve_digits = 4; /* for ATTRNAME == 20 */
    int max_base_len = ATTRNAME - 1 - reserve_digits;
    if (max_base_len < 1) max_base_len = 1;

    int max_suffix = 1;
    for (int i = 0; i < reserve_digits; i++) max_suffix *= 10;
    max_suffix -= 1;

    for (AttrDesc *p = head2; p; p = p->next)
    {
        /* skip join attr from second relation (normalized comparison) */
        char p_name_norm[ATTRNAME];
        normalize_str(p->attr.attrName, p_name_norm);
        if (strcmp(p_name_norm, attrName2) == 0) continue;

        AttrDesc *newNode = (AttrDesc *)malloc(sizeof(AttrDesc));
        if (!newNode)
        {
            db_err_code = MEM_ALLOC_ERROR;
            AttrDesc *tmp;
            while (resHead) { tmp = resHead; resHead = resHead->next; free(tmp); }
            return ErrorMsgs(db_err_code, print_flag);
        }

        newNode->attr = p->attr;
        newNode->attr.attrName[ATTRNAME - 1] = '\0';
        newNode->next = NULL;

        /* Use normalized collision check but exclude the current node p from s2 checks */
        char matching_name[ATTRNAME];
        int collSrc = nameCollidesDetailedWithMatch(p->attr.attrName, resHead, s1, s2, ad2, p, matching_name);
        bool collision = (collSrc != 0);

        if (collSrc == 0)
        {
            /* No collision: keep original name */
            strncpy(newNode->attr.attrName, p->attr.attrName, ATTRNAME - 1);
            newNode->attr.attrName[ATTRNAME - 1] = '\0';
            // printf("[JOIN-DEBUG] Keeping name '%s' unchanged (no collision detected).\n", newNode->attr.attrName);
        }
        else
        {
            const char *src_label = (collSrc == 1) ? "resHead" : (collSrc == 2) ? "s1" : "s2";
            // printf("[JOIN-DEBUG] Collision detected for incoming attr '%s' with source %s (existing='%s').\n",
            //        p_name_norm, src_label, matching_name);

            // hex_dump("incoming", p->attr.attrName);
            // hex_dump("existing", matching_name);

            /* Collision: create base (preserve original bytes if they fit) */
            char base[ATTRNAME];
            size_t orig_len = strnlen(p->attr.attrName, ATTRNAME);
            bool truncated_base = false;

            if (orig_len <= (size_t)max_base_len)
            {
                normalize_str(p->attr.attrName, base);
            }
            else
            {
                strncpy(base, p->attr.attrName, max_base_len);
                base[max_base_len] = '\0';
                truncated_base = true;
            }

            if (truncated_base)
            {
                // printf("[JOIN-DEBUG] Truncated attribute name '%s' -> base '%s' to reserve %d suffix digits.\n",
                //        p->attr.attrName, base, reserve_digits);
                // hex_dump("base", base);
            }

            char candidate[ATTRNAME];
            int suffix = 0;
            bool assigned = false;

            while (suffix <= max_suffix)
            {
                snprintf(candidate, ATTRNAME, "%s%d", base, suffix);
                candidate[ATTRNAME - 1] = '\0';

                char match2[ATTRNAME];
                int coll2 = nameCollidesDetailedWithMatch(candidate, resHead, s1, s2, ad2, p, match2);
                if (coll2 == 0)
                {
                    strncpy(newNode->attr.attrName, candidate, ATTRNAME - 1);
                    newNode->attr.attrName[ATTRNAME - 1] = '\0';
                    // printf("[JOIN-DEBUG] Renamed attribute '%s' -> '%s' (suffix=%d)\n",
                    //        p->attr.attrName, newNode->attr.attrName, suffix);
                    assigned = true;
                    break;
                }
                suffix++;
            }

            if (!assigned)
            {
                // printf("[JOIN-DEBUG] Exhausted suffix attempts for '%s' (reserve_digits=%d). Aborting join.\n",
                //        p->attr.attrName, reserve_digits);
                AttrDesc *tmp;
                while (resHead) { tmp = resHead; resHead = resHead->next; free(tmp); }
                free(newNode);
                db_err_code = MEM_ALLOC_ERROR;
                return ErrorMsgs(db_err_code, print_flag);
            }
        }

        /* append to result list */
        if (!resHead) resHead = resTail = newNode;
        else { resTail->next = newNode; resTail = newNode; }
    }

    /* Step 3: create schema, cleanup */
    int status = CreateFromAttrList(dstRelName, resHead);
    if (status != OK)
    {
        AttrDesc *tmp;
        while (resHead) { tmp = resHead; resHead = resHead->next; free(tmp); }
        return ErrorMsgs(db_err_code, print_flag);
    }

    /* free resHead */
    {
        AttrDesc *tmp;
        while (resHead) { tmp = resHead; resHead = resHead->next; free(tmp); }
    }

    /* Step 4: open dest and insert joined rows (same as before) */
    d = OpenRel(dstRelName);
    if (d == NOTOK) { db_err_code = RELNOEXIST; return ErrorMsgs(db_err_code, print_flag); }

    if(d == s1 || d == s2 || s1 == s2)
    {
        printf("[JOIN_DEBUG] There is a problem with JOIN.\n");
    }

    int dstRecSize = catcache[d].relcat_rec.recLength;
    void *recPtr1 = malloc(catcache[s1].relcat_rec.recLength);
    void *recPtr2 = malloc(catcache[s2].relcat_rec.recLength);
    if (!recPtr1 || !recPtr2)
    {
        if (recPtr1) free(recPtr1);
        if (recPtr2) free(recPtr2);
        return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
    }

    Rid rid1 = INVALID_RID, rid2 = INVALID_RID;
    char t = t1;
    int o1 = ad1->attr.offset;
    int o2 = ad2->attr.offset;

    int cmpSize = MIN(ad1->attr.length, ad2->attr.length);
    int dNumAttrs = catcache[d].relcat_rec.numAttrs;
    int s1NumAttrs = catcache[s1].relcat_rec.numAttrs;
    int s2NumAttrs = catcache[s2].relcat_rec.numAttrs;

    rid1 = INVALID_RID;
    while (1)
    {
        rid2 = INVALID_RID;
        if (GetNextRec(s1, rid1, &rid1, recPtr1) == NOTOK)
        {
            free(recPtr1); free(recPtr2);
            return ErrorMsgs(db_err_code, print_flag);
        }
        if (!isValidRid(rid1)) break;

        while (1)
        {
            if (GetNextRec(s2, rid2, &rid2, recPtr2) == NOTOK)
            {
                free(recPtr1); free(recPtr2);
                return ErrorMsgs(db_err_code, print_flag);
            }
            if (!isValidRid(rid2)) break;

            void *valPtr1 = (char *)recPtr1 + o1;
            void *valPtr2 = (char *)recPtr2 + o2;

            if (compareVals(valPtr1, valPtr2, t, cmpSize, CMP_EQ))
            {
                void *dstRecPtr = malloc(dstRecSize);
                if (!dstRecPtr)
                {
                    free(recPtr1); free(recPtr2);
                    db_err_code = MEM_ALLOC_ERROR;
                    return ErrorMsgs(db_err_code, print_flag);
                }

                AttrDesc *dstAttrDescPtr = catcache[d].attrList;
                AttrDesc *src1AttrDescPtr = catcache[s1].attrList;
                AttrDesc *src2AttrDescPtr = catcache[s2].attrList;

                for (int ctr = 0; ctr < dNumAttrs; ctr++)
                {
                    if (!dstAttrDescPtr) break;
                    if (ctr < s1NumAttrs)
                    {
                        copy_attribute(recPtr1, dstRecPtr, &src1AttrDescPtr, &dstAttrDescPtr);
                    }
                    else
                    {
                        if (src2AttrDescPtr == ad2)
                        {
                            src2AttrDescPtr = src2AttrDescPtr->next;
                            ctr--;
                            continue;
                        }
                        else
                        {
                            copy_attribute(recPtr2, dstRecPtr, &src2AttrDescPtr, &dstAttrDescPtr);
                        }
                    }
                }

                if (InsertRec(d, dstRecPtr) == NOTOK)
                {
                    free(dstRecPtr); free(recPtr1); free(recPtr2);
                    return ErrorMsgs(db_err_code, print_flag);
                }
                free(dstRecPtr);
            }
        } /* inner */
    } /* outer */

    free(recPtr1); free(recPtr2);
    printf("Join of relations %s and %s into %s successfully performed.\n",
           src1RelName, src2RelName, dstRelName);
    return OK;
}