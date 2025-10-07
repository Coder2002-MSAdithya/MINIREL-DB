#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


int Create(int argc, char *argv[])
{
    if (!db_open)
    {
        return ErrorMsgs(DBNOTOPEN, print_flag);
    }

    if (argc < 3)
    {
        return ErrorMsgs(ARGC_INSUFFICIENT, print_flag);
    }

    char *relName = argv[1];
    int numAttrs = (argc-2)/2;
    int i;

    FILE *fp = fopen(RELCAT, "rb");
    if (!fp)
    {
        return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
    }

    RelCatRec rcrec;
    int found = 0;
    while (fread(&rcrec, sizeof(RelCatRec), 1, fp) == 1)
    {
        if (strcmp(rcrec.relName, relName) == 0)
        {
            return ErrorMsgs(RELEXIST, print_flag);
            break;
        }
    }
    fclose(fp);

    // ---- Create Relation File ----
    char path[MAX_PATH_LENGTH];
    snprintf(path, sizeof(path) + RELNAME + 2, "%s/%s", DB_DIR, relName);

    int fd = creat(path, 0777);
    if (fd < 0)
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }

    // ---- Initialize Header Page ----
    Page page;
    memset(&page, 0, sizeof(Page));
    strcpy(page.magicString, "$MINIREL");
    page.slotmap = 0UL;
    write(fd, &page, PAGESIZE);
    close(fd);

    // ---- Build RelCat Record ----
    RelCatRec newrel;
    strcpy(newrel.relName, relName);
    newrel.numAttrs = numAttrs;
    newrel.numRecs = 0;
    newrel.numPgs = 1;

    // Compute record length from attribute list
    int recLength = 0;
    unsigned len = 0;
    int n = 0;
    char type;
    for (int i = 2; i < argc; i += 2)
    {
        type = argv[i+1][0];
        if (type == 'i')
        {
            len = sizeof(int);
        }

        else if (type == 'f')
        {
            len = sizeof(float);
        }
        else if (type == 's')
        {
            n = atoi(argv[i+1]+1);
            if (n > 50 || n < 1)
            {
                return ErrorMsgs(STR_LEN_INVALID, print_flag);
            }
            len = n;
        }
        else
        {
            return ErrorMsgs(INVALID_FORMAT, print_flag);
        }

        recLength += len;
    }
    newrel.recLength = recLength;
    newrel.recsPerPg = (PAGESIZE - HEADER_SIZE) / recLength;

    // ---- Update RelCat ----
    FILE *relFp = fopen(RELCAT, "ab");
    if (!relFp)
    {
        return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
    }

    fwrite(&newrel, sizeof(RelCatRec), 1, relFp);
    fclose(relFp);

    // ---- Update AttrCat ----
    FILE *attrFp = fopen(ATTRCAT, "ab");
    if (!attrFp)
    {
        return ErrorMsgs(CAT_OPEN_ERROR, print_flag);
    }

    int offset = 0;
    for (int i = 2; i < argc; i += 2)
    {
        AttrCatRec attr;
        attr.offset = offset;
        attr.type = argv[i+1][0];
        if (attr.type == 'i')
        {
            attr.length = sizeof(int);
        }

        else if (attr.type == 'f')
        {
            attr.length = sizeof(float);
        }
        else if (attr.type == 's')
        {
            attr.length = n;
        }
        strcpy(attr.attrName, argv[i]);
        strcpy(attr.relName, relName);

        fwrite(&attr, sizeof(AttrCatRec), 1, attrFp);
        offset += attr.length;
    }

    fclose(attrFp);

    if (print_flag)
    {
        printf("[INFO] Created relation '%s' with %d attributes.\n", relName, numAttrs);
        printf("[INFO] Record length: %d bytes, Records/page: %d\n", newrel.recLength, newrel.recsPerPg);
    }

    return OK;
}