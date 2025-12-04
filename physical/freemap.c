/************************INCLUDES*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "../include/defs.h"     /* for OK, NOTOK, RELNAME, etc. */
#include "../include/error.h"    /* for FILESYSTEM_ERROR, MEM_ALLOC_ERROR, etc. */
#include "../include/globals.h"  /* For db_err_code catcache etc*/


/*------------------------------------------------------------

FUNCTION  build_fmap_filename(relName, fname, buflen):

PARAMETER DESCRIPTION:
    relName → Name of the relation.
    fname   → Output buffer where "<relName>.fmap" will be written.
    buflen  → Size of output buffer.

FUNCTION DESCRIPTION:
    Constructs the filename used to store the freemap bitmap for a relation.
    The freemap file always follows the naming rule:
            "<relName>.fmap"

ALGORITHM:
    1) Use snprintf() to concatenate relName and ".fmap".
    2) Ensure the output fits into the provided buffer (snprintf guarantees null-termination as long as buflen > 0).

GLOBAL VARIABLES MODIFIED:
    None.

ERRORS REPORTED:
    None.

IMPLEMENTATION NOTES:
    - Caller must provide a sufficiently large buffer (>= RELNAME + 6).
    - The function is used by:
        • FreeMapExists()
        • CreateFreeMap()
        • AddToFreeMap()
        • DeleteFromFreeMap()
        • FindFreeSlot()
    - Keeping filename logic in one helper avoids duplication and ensures consistency.

BUGS
       None known.

------------------------------------------------------------*/

/*
 * We use a fixed-size bitmap indexed by pageNum (short, 0..32767).
 * That’s 32768 bits = 4096 bytes per relation -> very small.
 */

#define MAX_FREEMAP_PAGES   32768
#define FREEMAP_BYTES       (MAX_FREEMAP_PAGES / 8)

void build_fmap_filename(const char *relName, char *fname, size_t buflen)
{
    snprintf(fname, buflen, "%s.fmap", relName);
}


/*------------------------------------------------------------

FUNCTION  FreeMapExists(relName):

PARAMETER DESCRIPTION:
    relName → Name of the relation whose freemap file is being queried.

FUNCTION DESCRIPTION:
    A freemap is an auxiliary bitmap file "<relName>.fmap" that tracks which pages of the heap file contain free slots.  
    This routine checks whether the freemap file exists and returns:
         1  → freemap exists  
         0  → freemap does NOT exist  
        -1  → unexpected filesystem error
       It does NOT modify any global catalog state.

ALGORITHM
    1) Construct the filename "<relName>.fmap".
    2) Try opening the file in read-binary mode.
    3) If fopen() succeeds, close it and return 1, if it fails due to ENOENT, retuen 0, otherwise return -1.

GLOBAL VARIABLES MODIFIED:
    errno

ERRORS REPORTED:
    None.

BUGS:
    None known.

------------------------------------------------------------*/


int FreeMapExists(const char *relName)
{
    char fname[RELNAME + 6];
    build_fmap_filename(relName, fname, sizeof(fname));

    FILE *fp = fopen(fname, "rb");
    if (!fp)
    {
        if (errno == ENOENT)
            return 0;   // does not exist

        // Some other unexpected filesystem error
        return -1;
    }

    fclose(fp);
    return 1;
}


/*------------------------------------------------------------

FUNCTION  CreateFreeMap(relName):

PARAMETER DESCRIPTION:
    relName → Name of the relation for which the freemap file "<relName>.fmap" must be created or reset.

FUNCTION DESCRIPTION:
    Creates a freemap file of fixed size (4096 bytes) with all bits = 0, meaning all pages are “not free / not yet allocated”.
    Called when:
        - A new relation is created.
        - set_freemap_bit() discovers that no freemap exists.

ALGORITHM:
    1) Construct freemap filename.
    2) Open file in write-binary mode ("wb").
    3) Write FREEMAP_BYTES of zero bytes.
    4) Close file and return OK.

GLOBAL VARIABLES MODIFIED:
    db_err_code → set to FILESYSTEM_ERROR on write/open failure.

ERRORS REPORTED:
    FILESYSTEM_ERROR

BUGS
    None known.

------------------------------------------------------------*/

int CreateFreeMap(const char *relName)
{
    char fname[RELNAME + 6];
    build_fmap_filename(relName, fname, sizeof(fname));

    FILE *fp = fopen(fname, "wb");
    if (!fp)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    unsigned char zero[FREEMAP_BYTES];
    memset(zero, 0, sizeof(zero));

    if (fwrite(zero, 1, sizeof(zero), fp) != sizeof(zero))
    {
        fclose(fp);
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    fclose(fp);
    return OK;
}


/*------------------------------------------------------------

FUNCTION  set_freemap_bit(relName, pageNum, value):

PARAMETER DESCRIPTION:
    relName → Relation name
    pageNum → Page number whose freemap bit must be set/cleared
    value   → 1 to mark page as having free space, 0 to mark as full

FUNCTION DESCRIPTION:
    Low-level helper routine that performs a single-bit update in the freemap file. 
    One byte is read, modified, and re-written.

ALGORITHM:
    1) Validate that pageNum ∈ [0, MAX_FREEMAP_PAGES).
    2) Build filename "<relName>.fmap".
    3) Try opening file in "rb+" mode.
            If ENOENT → create freemap via CreateFreeMap() and reopen.
    4) Seek to corresponding byte = pageNum/8.
    5) Read the byte (assume 0 if EOF).
    6) Modify the bit:
            value == 1 → set bit  
            value == 0 → clear bit
    7) Seek back and write modified byte.
    8) Close file and return OK.

GLOBAL VARIABLES MODIFIED:
    db_err_code → set to FILESYSTEM_ERROR on fseek/fwrite failure.

ERRORS REPORTED:
    FILESYSTEM_ERROR

BUGS:
    None known.

------------------------------------------------------------*/

int set_freemap_bit(const char *relName, short pageNum, int value /*0 or 1*/)
{
    if (pageNum < 0 || pageNum >= MAX_FREEMAP_PAGES)
        return NOTOK;

    char fname[RELNAME + 6];
    build_fmap_filename(relName, fname, sizeof(fname));

    FILE *fp = fopen(fname, "rb+");
    if (!fp)
    {
        /* If the file doesn't exist, create it and reopen. */
        if (errno == ENOENT)
        {
            if (CreateFreeMap(relName) != OK)
                return NOTOK;
            fp = fopen(fname, "rb+");
            if (!fp)
            {
                db_err_code = FILESYSTEM_ERROR;
                return NOTOK;
            }
        }
        else
        {
            return NOTOK;
        }
    }

    long byteIndex = pageNum / 8;
    int  bitIndex  = pageNum % 8;

    /* Seek to the correct byte in the bitmap */
    if (fseek(fp, byteIndex, SEEK_SET) != 0)
    {
        fclose(fp);
        return NOTOK;
    }

    unsigned char byte;
    size_t n = fread(&byte, 1, 1, fp);
    if (n != 1)
    {
        /* If EOF, treat as zero and extend */
        byte = 0;
    }

    if (value)
        byte |= (1u << bitIndex);   /* set bit */
    else
        byte &= ~(1u << bitIndex);  /* clear bit */

    /* Seek back and write updated byte */
    if (fseek(fp, byteIndex, SEEK_SET) != 0)
    {
        db_err_code = FILESYSTEM_ERROR;
        fclose(fp);
        return NOTOK;
    }

    if (fwrite(&byte, 1, 1, fp) != 1)
    {
        db_err_code = FILESYSTEM_ERROR;
        fclose(fp);
        return NOTOK;
    }

    fclose(fp);
    return OK;
}


/*------------------------------------------------------------

FUNCTION  AddToFreeMap(relName, pageNum):

PARAMETER DESCRIPTION:
    relName → name of the relation
    pageNum → page that just gained at least one free slot

FUNCTION DESCRIPTION:
    Marks a page as having free space by setting its freemap bit to 1.
    O(1): single-byte read–modify–write.
    Returns OK or NOTOK depending upon the success of set_freemap_bit function.

------------------------------------------------------------*/

int AddToFreeMap(const char *relName, short pageNum)
{
    return set_freemap_bit(relName, pageNum, 1);
}


/*------------------------------------------------------------

FUNCTION  DeleteFromFreeMap(relName, pageNum)

PARAMETER DESCRIPTION
    relName → name of the relation
    pageNum → page that became full (no free slots)

FUNCTION DESCRIPTION:
       Clears the freemap bit for this page (sets bit to 0) to indicate that it is no longer free..
       O(1): single-byte read–modify–write.
       Returns OK or NOTOK depending upon success of set_freemap_bit function.

------------------------------------------------------------*/

int DeleteFromFreeMap(const char *relName, short pageNum)
{
    return set_freemap_bit(relName, pageNum, 0);
}


/*------------------------------------------------------------

FUNCTION  FindFreeSlot(relName):

PARAMETER DESCRIPTION:
       relName → Name of the relation whose freemap must be scanned.

FUNCTION DESCRIPTION:
    Scans the bitmap file "<relName>.fmap" to find ANY page whose bit=1.
    That page has free slots available for insertion.   
    Return the page number of ANY page with bit=1 (i.e., has free slot), or -1 if none exist or file missing.
    This is O(#pages/8) = O(FREEMAP_BYTES). With MAX_FREEMAP_PAGES fixed, this is bounded (4096 bytes) and very fast.

ALGORITHM:
    1) Build freemap filename and attempt to open in "rb".
        If open fails → return -1.
    2) Read full bitmap (FREEMAP_BYTES).
        If fread() returns 0 → set db_err_code and return NOTOK.
    3) For each byte:
        If byte != 0:
            Scan each of the 8 bits:
                If bit == 1 → compute pageNum and return it.
    4) If no bits found, return -1.

GLOBAL VARIABLES MODIFIED:
    db_err_code → set to FILESYSTEM_ERROR on read failure.

ERRORS REPORTED:
    FILESYSTEM_ERROR

BUGS:
    None known.

------------------------------------------------------------*/

int FindFreeSlot(const char *relName)
{
    char fname[RELNAME + 6];
    build_fmap_filename(relName, fname, sizeof(fname));

    FILE *fp = fopen(fname, "rb");
    if (!fp)
        return -1;   /* no fmap or cannot open => assume no known free page */

    unsigned char buf[FREEMAP_BYTES];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);

    if (n == 0)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    /* Scan for first non-zero byte */
    for (int byteIndex = 0; byteIndex < (int)n; byteIndex++)
    {
        unsigned char byte = buf[byteIndex];
        if (byte == 0)
            continue;

        /* Find first 1-bit in this byte */
        for (int bit = 0; bit < 8; bit++)
        {
            if (byte & (1u << bit))
            {
                int pageNum = byteIndex * 8 + bit;
                if (pageNum < MAX_FREEMAP_PAGES)
                    return pageNum;
            }
        }
    }

    return -1;
}