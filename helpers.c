#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include "include/defs.h"
#include "include/helpers.h"
#include "include/globals.h"
#include "include/error.h"
#define BYTES_PER_LINE 16

int ceil_div(int a, int b)
{
    return (a+b-1)/b;
}

double dmax(double a, double b) { return (a > b) ? a : b; }

bool isValidPath(const char *path)
{
    // Empty path is invalid
    if (path == NULL || *path == '\0')
        return false;

    const char *p = path;

    while (*p)
    {
        // Skip leading slashes (allow multiple consecutive /)
        while (*p == '/')
            p++;

        if (*p == '\0')
            break; // trailing slashes are fine

        // First character of component must be alphabet
        if (!isalpha((unsigned char)*p))
            return false;

        p++;

        // Rest of the component: only alphanumeric
        while (*p && *p != '/')
        {
            if (!isalnum((unsigned char)*p))
                return false;
            p++;
        }
    }

    return true;
}

int remove_all_entry(const char *path)
{
    struct stat st;
    if (lstat(path, &st) == -1)
    {
        /* If the file doesn't exist any more, treat as success for this entry. */
        if (errno == ENOENT)
        {
            return 0;
        }
        fprintf(stderr, "lstat(%s) failed: %s\n", path, strerror(errno));
        return -1;
    }

    if (S_ISDIR(st.st_mode))
    {
        /* Directory: recurse into it, remove its contents, then remove the directory itself. */
        DIR *d = opendir(path);
        if (d == NULL)
        {
            fprintf(stderr, "opendir(%s) failed: %s\n", path, strerror(errno));
            return -1;
        }

        struct dirent *entry;
        int ret = 0;

        while ((entry = readdir(d)) != NULL)
        {
            /* Skip "." and ".." */
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            /* Build full child path */
            char child[MAX_PATH_LENGTH];
            int n = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
            if (n < 0 || (size_t)n >= sizeof(child))
            {
                fprintf(stderr, "Path too long: %s/%s\n", path, entry->d_name);
                ret = -1;
                continue;
            }

            if (remove_all_entry(child) != 0)
            {
                /* keep going but record failure */
                ret = -1;
            }
        }

        closedir(d);

        /* After removing contents, remove the directory itself. */
        if (rmdir(path) == -1)
        {
            fprintf(stderr, "rmdir(%s) failed: %s\n", path, strerror(errno));
            return -1;
        }

        return ret;
    }
    else
    {
        /* Not a directory (regular file, symlink, socket, fifo, device): unlink it. */
        if (unlink(path) == -1)
        {
            fprintf(stderr, "unlink(%s) failed: %s\n", path, strerror(errno));
            return -1;
        }
        return 0;
    }
}

/*
 * remove_contents(dirpath)
 *   Removes all entries inside dirpath but does not remove dirpath itself.
 *   Returns 0 if everything removed successfully, non-zero if any error occurred.
 */
int remove_contents(const char *dirpath)
{
    struct stat st;
    if (lstat(dirpath, &st) == -1)
    {
        fprintf(stderr, "lstat(%s) failed: %s\n", dirpath, strerror(errno));
        return -1;
    }

    if (!S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "%s is not a directory\n", dirpath);
        return -1;
    }

    DIR *d = opendir(dirpath);
    if (d == NULL)
    {
        fprintf(stderr, "opendir(%s) failed: %s\n", dirpath, strerror(errno));
        return -1;
    }

    struct dirent *entry;
    int overall_ret = 0;

    while ((entry = readdir(d)) != NULL)
    {
        /* Skip "." and ".." */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char child[MAX_PATH_LENGTH];
        int n = snprintf(child, sizeof(child), "%s/%s", dirpath, entry->d_name);
        if (n < 0 || (size_t)n >= sizeof(child))
        {
            fprintf(stderr, "Path too long: %s/%s\n", dirpath, entry->d_name);
            overall_ret = -1;
            continue;
        }

        if (remove_all_entry(child) != 0)
        {
            overall_ret = -1;
        }
    }

    closedir(d);
    return overall_ret;
}


void print_page_hex(const char *buf) 
{
    for (int line = 0; line < PAGESIZE / BYTES_PER_LINE; ++line) {
        int base = line * BYTES_PER_LINE;
        /* print line number (decimal, zero-padded to 2 digits) */
        printf("%02X: ", line);
        /* print 16 bytes in hex */
        for (int i = 0; i < BYTES_PER_LINE; ++i) {
            printf("%02X", (unsigned char) buf[base + i]);
            if (i + 1 < BYTES_PER_LINE) putchar(' ');
        }
        putchar('\n');
    }
}

int writeRecsToFile(const char *filename, void *recs, int numRecs, int recordSize, char magicChar)
{
    FILE *fp = fopen(filename, "wb");

    if(!fp)
    {
        db_err_code = CAT_CREATE_ERROR;
        return NOTOK;
    }

    char page[PAGESIZE];
    int recsPerPg = (PAGESIZE-HEADER_SIZE)/recordSize;

    int written = 0;

    while(written < numRecs)
    {
        // Prepare the magic word to write
        char MAGIC_STR[MAGIC_SIZE+1] = {magicChar};
        strcpy(MAGIC_STR+1, GEN_MAGIC);

        //Reset page after it is over
        memset(page, 0, PAGESIZE);

        // Write Magic string
        strcpy(page, MAGIC_STR);

        //Pointer to slotmap
        unsigned long *slotmapPtr = (unsigned long *)(page + MAGIC_SIZE);
        *slotmapPtr = 0;

        //Page data start
        char *dataStart = page + HEADER_SIZE;

        //Fill records into this page
        int slot = 0;
        
        while(slot < recsPerPg && slot < (SLOTMAP << 3) && written < numRecs)
        {
            memcpy(dataStart+slot*recordSize, (char *)recs+written*recordSize, recordSize);

            //Mark slot map filled (LSB first)
            *slotmapPtr |= (1UL << slot);

            slot++;
            written++;
        }

        //Write the page
        fwrite(page, 1, PAGESIZE, fp);
    }

    fclose(fp);
    return OK;
}

int float_cmp(double a, double b, double rel_eps, double abs_eps)
{
    if (isnan(a) || isnan(b)) return 2;

    if (isinf(a) || isinf(b)) {
        if (a == b) return 0;
        return (a > b) ? 1 : -1;
    }

    double diff = a - b;
    double tol = dmax(abs_eps, rel_eps * dmax(fabs(a), fabs(b)));

    if (fabs(diff) <= tol)
        return 0;
    return (diff > 0) ? 1 : -1;
}

Rid IncRid(Rid rid, int recsPerPg)
{
    if(!isValidRid(rid))
    {
        return (Rid){0, 0};
    }
    else
    {
        rid.slotnum++;
        rid.slotnum %= recsPerPg;

        if(!rid.slotnum)
        {
            rid.pid++;
        }
        
        return rid;
    }

    return INVALID_RID;
}

bool isValidRid(Rid rid)
{
    return (rid.pid >= 0 && rid.slotnum >= 0);
}

int FreeLinkedList(void **headPtr, size_t nextOff)
{
    if (headPtr == NULL || *headPtr == NULL)
        return NOTOK;

    void *curr = *headPtr;
    void *next = NULL;

    while (curr != NULL)
    {
        // Compute address of the "next" field inside the current node
        void **nextPtr = (void **)((char *)curr + nextOff);
        next = *nextPtr;  // get next node

        free(curr);       // free current node
        curr = next;      // move to next
    }

    *headPtr = NULL;  // set head to NULL after freeing
    return OK;
}

bool isValidInteger(char *str)
{
    if(*str == '-' || *str == '+')
        str++;

    if(!*str)
        return false;

    while(*str)
    {
        if(!isdigit((unsigned char)*str))
            return false;
        str++;
    }

    return true;
}

bool isValidFloat(char *str)
{
    int dotSeen = 0;

    if(*str == '-' || *str == '+')
        str++;

    if(!*str)
        return false;

    while(*str)
    {
        if(*str == '.')
        {
            if(dotSeen)
                return false;
            dotSeen = 1;
        }
        else if(!isdigit((unsigned char)*str))
        {
            return false;
        }
        str++;
    }

    return true;
}

bool isValidForType(char type, int size, void *value, void *dstValuePtr)
{
    if(type == 'i')
    {
        if(!isValidInteger(value))
        {
            return false;
        }

        *(int *)(dstValuePtr) = atoi(value);
    }
    else if(type == 'f')
    {
        if(!isValidFloat(value))
        {
            return false;
        }

        *(float *)(dstValuePtr) = atof(value);
    }
    else if(type == 's')
    {
        strncpy(dstValuePtr, value, size);
        ((char *)dstValuePtr)[size - 1] = '\0';
        return true;
    }

    return true;
}