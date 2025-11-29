#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrec.h"
#include "../include/openrel.h"
#include "../include/closerel.h"
#include "../include/freemap.h"
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

int Load(int argc, char **argv)
{
    if (argc < 3)
    {
        db_err_code = ARGC_INSUFFICIENT;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if (argc > 3)
    {
        db_err_code = TOO_MANY_ARGS;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    char *fileName = argv[2];

    int r = OpenRel(relName);
    if (r == NOTOK)
    {
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    Rid foundRid = INVALID_RID;
    RelCatRec rc;

    FindRec(RELCAT_CACHE, foundRid, &foundRid, &rc, 's', RELNAME,
            offsetof(RelCatRec, relName), relName, CMP_EQ);

    if (!isValidRid(foundRid))
    {
        db_err_code = RELNOEXIST;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    // Check if source file exists
    if(access(fileName, F_OK))
    {
        db_err_code = FILE_NO_EXIST;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    int *numPgs = &(catcache[r].relcat_rec.numPgs);
    int *numRecs = &(catcache[r].relcat_rec.numRecs);

    // Ensure relation is empty
    if(*numPgs)
    {
        db_err_code = LOAD_NONEMPTY;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    // ------------------- FILE COPY SECTION -------------------

    int srcFd = open(fileName, O_RDONLY);
    if (srcFd < 0)
    {
        db_err_code = FILE_NO_EXIST;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    struct stat st;
    if (fstat(srcFd, &st) < 0)
    {
        close(srcFd);
        db_err_code = FILESYSTEM_ERROR;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    off_t fileSize = st.st_size;
    if (fileSize % PAGESIZE != 0)
    {
        close(srcFd);
        db_err_code = INVALID_FILE_SIZE;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    off_t numPages = fileSize / PAGESIZE;
    unsigned char page[PAGESIZE];
    int writeError = 0; // flag for rollback

    for (off_t i = 0; i < numPages; i++)
    {
        ssize_t bytesRead = read(srcFd, page, PAGESIZE);
        if (bytesRead != PAGESIZE)
        {
            writeError = 1;
            db_err_code = FILESYSTEM_ERROR;
            break;
        }

        // Validate the _MINIREL magic string at the beginning of the page
        if (memcmp(page+1, GEN_MAGIC, MAGIC_SIZE-1))
        {
            writeError = 1;
            db_err_code = PAGE_MAGIC_ERROR;
            break;
        }

        // Write page to relation file
        ssize_t bytesWritten = write(catcache[r].relFile, page, PAGESIZE);
        if (bytesWritten != PAGESIZE)
        {
            writeError = 1;
            db_err_code = FILESYSTEM_ERROR;
            break;
        }

        // Update numPgs
        (*numPgs)++;

        // Extract 8-byte slotmap (bytes 8–15)
        unsigned long slotmap = 0;
        bool foundFreeSlot = false;
        memcpy(&slotmap, page + MAGIC_SIZE, sizeof(slotmap));

        // Count number of set bits in slotmap → numRecs
        for(int b = 0; b < (sizeof(slotmap)<<3); b++)
        {
            if(slotmap & (1ULL << b))
            {
                (*numRecs)++;
            }
            else
            {
                foundFreeSlot = true;
            }
        }

        if(foundFreeSlot)
        AddToFreeMap(relName, i);
        
        printf("Page %d - Records read till now : %d\n", (int)i, *numRecs);
    }

    close(srcFd);

    if(writeError)
    {
        // Rollback — remove any written content
        ftruncate(catcache[r].relFile, 0);
        lseek(catcache[r].relFile, 0, SEEK_SET);

        *numPgs = 0;
        *numRecs = 0;

        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    // Mark relation as dirty
    catcache[r].status |= DIRTY_MASK;

    CloseRel(r);

    printf("Loaded relation '%s' successfully: %d pages, %d records.\n",
           relName, *numPgs, *numRecs);

    return OK;
}