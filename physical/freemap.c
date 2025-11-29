#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "../include/defs.h"     /* for OK, NOTOK, RELNAME, etc. */
#include "../include/error.h"    /* for db_err_code, MEM_ALLOC_ERROR, etc. */

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

/*
 * FreeMapExists:
 *   Returns:
 *      1  → freemap file exists
 *      0  → freemap file does NOT exist
 *     -1 → error accessing filesystem (rare)
 *
 * Called by InsertRec/DeleteRec to decide whether to use freemap or fallback scan.
 */
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

/* Create or reset an empty free-map file: all bits = 0 (no free pages). */
int CreateFreeMap(const char *relName)
{
    char fname[RELNAME + 6];
    build_fmap_filename(relName, fname, sizeof(fname));

    FILE *fp = fopen(fname, "wb");
    if (!fp)
        return NOTOK;

    unsigned char zero[FREEMAP_BYTES];
    memset(zero, 0, sizeof(zero));

    if (fwrite(zero, 1, sizeof(zero), fp) != sizeof(zero))
    {
        fclose(fp);
        return NOTOK;
    }

    fclose(fp);
    return OK;
}

/* Helper: set or clear a single bit in the bitmap file. */
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
                return NOTOK;
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
        fclose(fp);
        return NOTOK;
    }

    if (fwrite(&byte, 1, 1, fp) != 1)
    {
        fclose(fp);
        return NOTOK;
    }

    fclose(fp);
    return OK;
}

/*
 * AddToFreeMap:
 *   Mark pageNum as having at least one free slot.
 *   O(1): single-byte read–modify–write.
 */
int AddToFreeMap(const char *relName, short pageNum)
{
    return set_freemap_bit(relName, pageNum, 1);
}

/*
 * DeleteFromFreeMap:
 *   Mark pageNum as "no longer free" (full).
 *   O(1): single-byte read–modify–write.
 */
int DeleteFromFreeMap(const char *relName, short pageNum)
{
    return set_freemap_bit(relName, pageNum, 0);
}

/*
 * FindFreeSlot:
 *   Return the page number of ANY page with bit=1 (i.e., has free slot),
 *   or -1 if none exist or file missing.
 *
 *   This is O(#pages / 8) = O(FREEMAP_BYTES). With MAX_FREEMAP_PAGES fixed,
 *   this is bounded (4096 bytes) and very fast.
 */
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
        return -1;

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