/************************INCLUDES*******************************/
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrec.h"
#include "../include/openrel.h"
#include "../include/closerel.h"
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>


/*------------------------------------------------------------

FUNCTION Load (argc, argv)

PARAMETER DESCRIPTION:
    argc  → number of command arguments
    argv  → command argument vector
            argv[0] = "load"
            argv[1] = relation name
            argv[2] = input filename
            argv[argc] = NIL

FUNCTION DESCRIPTION:
    Loads an external MINIREL-formatted data file into an existing relation. 
    The function:
        - validates arguments. 
        - verifies that the database and relation are properly accessible. 
        - ensures the relation is empty.
        - checks the structural validity of the input file. 
        - copies its pages into the relation file.
        - updates catalog metadata. 
        - performs full rollback if any error occurs during the load process.

ALGORITHM:
    1. Validate argument count.
    2. Check that a database is currently open.
    3. Attempt to open the target relation using OpenRel().
       - If the relation does not exist, report RELNOEXIST.
    4. Lookup the relation’s catalog entry via FindRec() to obtain metadata such as recLength, recsPerPg, and expected page size.
    5. Validate the existence of the input file (access()) and open it.
    6. Obtain file statistics and verify:
        - file size is non-negative,
        - file size is an exact multiple of PAGESIZE.
       Otherwise, report INVALID_FILE_SIZE.
    7. Ensure the target relation is completely empty (numPgs must be 0).
       If not, report LOAD_NONEMPTY.
    8. For each page in the input file:
        a. Read exactly PAGESIZE bytes.
        b. Validate the MINIREL magic header inside the page.
        c. Write the page to the relation file.
        d. Increment numPgs.
        e. Extract slotmap and count set bits to update numRecs.
        f. Print progress information.
    9. If any read, write, or validation error occurs:
        - Truncate the relation file back to zero length,
        - Reset numPgs and numRecs,
        - Close the relation,
        - Return the corresponding error.
   10. Mark the relation’s catalog entry dirty (DIRTY_MASK) so that CloseRel() updates relcat.
   11. Close the relation.
   12. Print load completion message.

BUGS:
    - Assumes the external file strictly follows MINIREL page layout.
    - Does not verify semantic correctness of individual records.

ERRORS REPORTED:
    ARGC_INSUFFICIENT
    TOO_MANY_ARGS
    DBNOTOPEN
    RELNOEXIST
    FILE_NO_EXIST
    INVALID_FILE_SIZE
    PAGE_MAGIC_ERROR
    LOAD_NONEMPTY
    FILESYSTEM_ERROR
    UNKNOWN_ERROR

GLOBAL VARIABLES MODIFIED:
    catcache[relNum].relcat_rec.numPgs
    catcache[relNum].relcat_rec.numRecs
    catcache[relNum].status (DIRTY_MASK set)
    db_err_code

IMPLEMENTATION NOTES:
    - Relation must be empty before loading; appending is not supported.
    - File is processed page-by-page to maintain exact MINIREL format.

------------------------------------------------------------*/

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

    FindRec(RELCAT_CACHE, foundRid, &foundRid, &rc, 's', RELNAME, offsetof(RelCatRec, relName), relName, CMP_EQ);

    if (!isValidRid(foundRid))
    {
        db_err_code = RELNOEXIST;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(access(fileName, F_OK))
    {
        db_err_code = FILE_NO_EXIST;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }

    int *numPgs = &(catcache[r].relcat_rec.numPgs);
    int *numRecs = &(catcache[r].relcat_rec.numRecs);

    if(*numPgs)
    {
        db_err_code = LOAD_NONEMPTY;
        CloseRel(r);
        return ErrorMsgs(db_err_code, print_flag);
    }


    /* ------------------- FILE COPY SECTION ------------------- */

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
        memcpy(&slotmap, page + MAGIC_SIZE, sizeof(slotmap));

        // Count number of set bits in slotmap → numRecs
        for (int b = 0; b < (sizeof(slotmap)<<3); b++)
        {
            if (slotmap & (1ULL << b))
            {
                (*numRecs)++;
            }
        }
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

    printf("Loaded relation '%s' successfully: %d pages, %d records.\n", relName, *numPgs, *numRecs);

    return OK;
}