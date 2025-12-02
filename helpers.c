#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
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
#include "include/getnextrec.h"
#include "include/findrec.h"
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

int CreateFromAttrList(const char *dstRelName, AttrDesc *head)
{
    if (!dstRelName || !head)
        return NOTOK;

    // Count number of attributes in the linked list
    int numAttrs = 0;
    for (AttrDesc *p = head; p; p = p->next)
        numAttrs++;

    // argc = 2 (for "create" and dstRelName) + 2 * numAttrs (name + format for each attribute)
    int createArgc = 2 + 2 * numAttrs;

    // Allocate argv array (+1 for NULL terminator)
    char **createArgv = (char **)malloc((createArgc + 1) * sizeof(char *));
    if (!createArgv)
    {
        db_err_code = MEM_ALLOC_ERROR;
        return NOTOK;
    }

    createArgv[0] = strdup("_create");
    createArgv[1] = strdup(dstRelName);

    int i = 2;
    for (AttrDesc *p = head; p; p = p->next)
    {
        // Attribute name
        createArgv[i++] = strdup(p->attr.attrName);

        // Build attribute format string
        char format[5];
        switch (p->attr.type)
        {
            case 'i':
                strcpy(format, "i");
                break;
            case 'f':
                strcpy(format, "f");
                break;
            case 's':
                snprintf(format, sizeof(format), "s%d", p->attr.length);
                break;
            default:
                format[0] = '\0';
                break;
        }

        createArgv[i++] = strdup(format);
    }

    // NIL terminator
    createArgv[i] = NULL;

    // ---- Call Create() ----
    int status = Create(createArgc, createArgv);

    // ---- Free all allocated memory ----
    for (int j = 0; j < createArgc; j++)
        free(createArgv[j]);

    free(createArgv);

    return status;
}

bool compareVals(void *valPtr1, void *valPtr2, char type, int size, int cmpOp)
{
    if(type == 'i')
    {
        return *(int *)(valPtr1) == *(int *)(valPtr2);
    }
    else if(type == 'f')
    {
        return float_cmp(*(float *)(valPtr1), *(float *)(valPtr2), FLOAT_REL_EPS, FLOAT_ABS_EPS) == 0;
    }
    else if(type == 's')
    {
        return strncmp(valPtr1, valPtr2, size) == 0;
    }
}

void writeAttrToRec(void *dstRecPtr, void *valuePtr, int type, int size, int offset)
{
    if(type == 'i')
    {
        *(int *)((char *)dstRecPtr + offset) = *(int *)valuePtr;
    }
    else if(type == 'f')
    {
        *(float *)((char *)dstRecPtr + offset) = *(float *)valuePtr;
    }
    else if(type == 's')
    {
        strncpy(dstRecPtr + offset, valuePtr, size);
        *((char *)dstRecPtr + offset + size) = '\0';
    }
}

// Helper function to calculate Levenshtein distance (edit distance)
static int levenshtein_distance(const char *s1, const char *s2)
{
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int **dp = malloc((len1 + 1) * sizeof(int *));
    
    for (int i = 0; i <= len1; i++)
    {
        dp[i] = malloc((len2 + 1) * sizeof(int));
    }
    
    for (int i = 0; i <= len1; i++)
    {
        dp[i][0] = i;
    }
    
    for (int j = 0; j <= len2; j++)
    {
        dp[0][j] = j;
    }
    
    for (int i = 1; i <= len1; i++)
    {
        for (int j = 1; j <= len2; j++)
        {
            int cost = (tolower(s1[i-1]) == tolower(s2[j-1])) ? 0 : 1;
            dp[i][j] = dp[i-1][j-1] + cost; // substitution
            
            if (dp[i-1][j] + 1 < dp[i][j]) // deletion
            {
                dp[i][j] = dp[i-1][j] + 1;
            }
            
            if (dp[i][j-1] + 1 < dp[i][j]) // insertion
            {
                dp[i][j] = dp[i][j-1] + 1;
            }
        }
    }
    
    int result = dp[len1][len2];
    
    for (int i = 0; i <= len1; i++)
    {
        free(dp[i]);
    }
    free(dp);
    
    return result;
}

// Helper function to calculate Jaro-Winkler similarity (good for short strings like names)
static float jaro_winkler_similarity(const char *s1, const char *s2)
{
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    
    if (len1 == 0 || len2 == 0)
    {
        return 0.0f;
    }
    
    // Calculate match distance
    int max_len = (len1 > len2) ? len1 : len2;
    int match_distance = max_len / 2 - 1;
    if (match_distance < 0)
    {
        match_distance = 0;
    }
    
    int *s1_matches = calloc(len1, sizeof(int));
    int *s2_matches = calloc(len2, sizeof(int));
    
    int matches = 0;
    int transpositions = 0;
    
    // Count matches
    for (int i = 0; i < len1; i++)
    {
        int start = i - match_distance;
        if (start < 0)
        {
            start = 0;
        }
        
        int end = i + match_distance + 1;
        if (end > len2)
        {
            end = len2;
        }
        
        for (int j = start; j < end; j++)
        {
            if (!s2_matches[j] && tolower(s1[i]) == tolower(s2[j]))
            {
                s1_matches[i] = 1;
                s2_matches[j] = 1;
                matches++;
                break;
            }
        }
    }
    
    if (matches == 0)
    {
        free(s1_matches);
        free(s2_matches);
        return 0.0f;
    }
    
    // Count transpositions
    int k = 0;
    for (int i = 0; i < len1; i++)
    {
        if (s1_matches[i])
        {
            while (!s2_matches[k])
            {
                k++;
            }
            if (tolower(s1[i]) != tolower(s2[k]))
            {
                transpositions++;
            }
            k++;
        }
    }
    
    float jaro = ((float)matches / len1 + (float)matches / len2 + 
                 ((float)matches - transpositions / 2.0f) / matches) / 3.0f;
    
    // Calculate common prefix (up to 4 characters)
    int prefix = 0;
    int max_prefix = 4;
    if (len1 < max_prefix)
    {
        max_prefix = len1;
    }
    if (len2 < max_prefix)
    {
        max_prefix = len2;
    }
    
    while (prefix < max_prefix && tolower(s1[prefix]) == tolower(s2[prefix]))
    {
        prefix++;
    }
    
    float winkler = jaro + prefix * 0.1f * (1.0f - jaro);
    
    free(s1_matches);
    free(s2_matches);
    
    return winkler;
}

void printCloseStrings(int catRelNum, int offset, char *typedVal, char *filter)
{
    Rid startRid = INVALID_RID;
    int recSize = catcache[catRelNum].relcat_rec.recLength;
    char *recPtr = malloc(recSize);
    
    // Print a Did you mean message
    printf("Did you mean? ");
    
    if (!recPtr)
    {
        printf("Memory allocation error.\n");
        return;
    }
    
    // Array to store candidates
    Candidate *candidates = NULL;
    int candidate_count = 0;
    int candidate_capacity = 0;
    
    // Collect all schema object names and calculate similarity
    do
    {
        if(catRelNum == RELCAT_CACHE)
        {
            if(GetNextRec(catRelNum, startRid, &startRid, recPtr) == NOTOK)
            {
                break; // No more records
            }
        }
        else
        {
            if(FindRec(catRelNum, startRid, &startRid, recPtr, 's', RELNAME, 
            offsetof(AttrCatRec, relName), filter, CMP_EQ) == NOTOK)
            {
                break;
            }
        }

        if(!isValidRid(startRid))
        {
            break;
        }
        
        char *schemaObjName = recPtr + offset;
        
        // Calculate various similarity measures
        int edit_distance = levenshtein_distance(typedVal, schemaObjName);
        float jw_similarity = jaro_winkler_similarity(typedVal, schemaObjName);
        
        // Check if it's a close match
        // Criteria: 
        // 1. Edit distance <= 3 (for short names) or <= 20% of the longer string length
        // 2. OR Jaro-Winkler similarity >= 0.7
        // 3. OR typedVal is a substring of schemaObjName (or vice versa)
        int max_len = MAX(strlen(typedVal), strlen(schemaObjName));
        int is_substring = (strstr(schemaObjName, typedVal) != NULL) || 
                           (strstr(typedVal, schemaObjName) != NULL);
        
        if (edit_distance <= 3 || 
            edit_distance <= max_len * 0.2 || 
            jw_similarity >= 0.7f ||
            is_substring)
        {
            // Add candidate to array
            if (candidate_count >= candidate_capacity)
            {
                candidate_capacity = candidate_capacity == 0 ? 10 : candidate_capacity * 2;
                Candidate *new_candidates = realloc(candidates, 
                                                   candidate_capacity * sizeof(Candidate));
                if (!new_candidates)
                {
                    // Memory allocation failed
                    for (int i = 0; i < candidate_count; i++)
                    {
                        free(candidates[i].name);
                    }
                    free(candidates);
                    free(recPtr);
                    return;
                }
                candidates = new_candidates;
            }
            
            candidates[candidate_count].name = strdup(schemaObjName);
            candidates[candidate_count].similarity = jw_similarity;
            candidates[candidate_count].edit_distance = edit_distance;
            candidate_count++;
        }
    }
    while(1);
    
    // Sort candidates by similarity (highest first) and then by edit distance (lowest first)
    for (int i = 0; i < candidate_count - 1; i++)
    {
        for (int j = i + 1; j < candidate_count; j++)
        {
            if (candidates[j].similarity > candidates[i].similarity || 
                (candidates[j].similarity == candidates[i].similarity && 
                 candidates[j].edit_distance < candidates[i].edit_distance))
            {
                Candidate temp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = temp;
            }
        }
    }
    
    // Print suggestions (limit to 5 best matches)
    if (candidate_count == 0)
    {
        printf("No close matches found.\n");
    }
    else
    {
        int print_count = candidate_count > 5 ? 5 : candidate_count;
        
        for (int i = 0; i < print_count; i++)
        {
            if (i > 0)
            {
                printf(", ");
            }
            printf("\"%s\"", candidates[i].name);
        }
        printf("\n");
    }
    
    // Free allocated memory
    for (int i = 0; i < candidate_count; i++)
    {
        free(candidates[i].name);
    }
    free(candidates);
    free(recPtr);
}