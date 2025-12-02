#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/getnextrec.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/* assume all required headers, types, and globals (db_open, db_err_code, etc.) are defined */
static void printSeparator(AttrDesc *attrList, int *colWidths);
static void printHeader(AttrDesc *attrList, int *colWidths);
static char* formatIntValue(int value);
static char* formatFloatValue(float value);
static char* formatStringValue(const char *data, int length);

/* ============================================================= */
int Print(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    int r = OpenRel(relName);

    if(r == NOTOK)
    {
        printf("Relation '%s' does NOT exist in the DB.\n", relName);
        printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), relName, NULL);
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    printf("OK, printing relation %s\n\n", relName);

    /* --------- Count attributes --------- */
    int attrCount = 0;
    for(AttrDesc *p = catcache[r].attrList; p; p = p->next)
        attrCount++;

    if(attrCount == 0)
    {
        printf("(Empty relation)\n");
        return OK;
    }

    /* --------- Allocate column width array --------- */
    int *colWidths = (int *)malloc(sizeof(int) * attrCount);
    if(!colWidths)
    {
        return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
    }

    /* --------- Determine column widths --------- */
    int idx = 0;
    for(AttrDesc *p = catcache[r].attrList; p; p = p->next, idx++)
    {
        int nameLen = (int)strlen(p->attr.attrName);
        int dataLen;
        switch(p->attr.type)
        {
            case 'i': 
                // Calculate maximum int string length (including sign)
                {
                    char tempBuf[32];
                    dataLen = snprintf(tempBuf, sizeof(tempBuf), "%d", INT_MIN);
                }
                break;
            case 'f': 
                // Calculate maximum width needed for float representation
                // We need to consider both fixed-point and scientific notation
                {
                    // Fixed-point representation: "%.2f" for medium-sized numbers
                    char tempBuf1[64];
                    int len1 = snprintf(tempBuf1, sizeof(tempBuf1), "%.2f", 9999999.99f);
                    
                    // Scientific notation: "%.2e" for extreme values
                    char tempBuf2[64];
                    int len2 = snprintf(tempBuf2, sizeof(tempBuf2), "%.2e", FLT_MAX);
                    
                    // For negative values in scientific notation
                    char tempBuf3[64];
                    int len3 = snprintf(tempBuf3, sizeof(tempBuf3), "%.2e", -FLT_MAX);
                    
                    // Take the maximum of all representations
                    dataLen = len1;
                    if (len2 > dataLen) dataLen = len2;
                    if (len3 > dataLen) dataLen = len3;
                    
                    // Ensure a reasonable minimum width
                    if (dataLen < 10) dataLen = 10;
                }
                break;
            case 's': 
                dataLen = p->attr.length; 
                break;
            default:  
                dataLen = 10; 
                break;
        }

        int colWidth = (nameLen > dataLen ? nameLen : dataLen);
        colWidth += 2; // padding for spacing
        colWidths[idx] = colWidth;
    }

    /* --------- Print header --------- */
    printHeader(catcache[r].attrList, colWidths);

    /* --------- Print records --------- */
    Rid startRid = INVALID_RID;
    int recSize = catcache[r].relcat_rec.recLength;
    void *recPtr = malloc(recSize);
    if(!recPtr)
    {
        free(colWidths);
        return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
    }

    int rowCount = 0;

    while(true)
    {
        int status = GetNextRec(r, startRid, &startRid, recPtr);
        if(status == NOTOK || !isValidRid(startRid))
            break;

        printf("|");
        idx = 0;

        for(AttrDesc *p = catcache[r].attrList; p; p = p->next, idx++)
        {
            AttrCatRec *ac = &(p->attr);
            char *attrPtr = (char *)recPtr + ac->offset;

            switch(ac->type)
            {
                case 'i': {
                    int ival;
                    memcpy(&ival, attrPtr, sizeof(int));
                    char *formatted = formatIntValue(ival);
                    if (formatted) {
                        printf(" %*s |", colWidths[idx] - 2, formatted);
                        free(formatted);
                    } else {
                        printf(" %*s |", colWidths[idx] - 2, "ERROR");
                    }
                    break;
                }

                case 'f': {
                    float fval;
                    memcpy(&fval, attrPtr, sizeof(float));
                    char *formatted = formatFloatValue(fval);
                    if (formatted) {
                        printf(" %*s |", colWidths[idx] - 2, formatted);
                        free(formatted);
                    } else {
                        printf(" %*s |", colWidths[idx] - 2, "ERROR");
                    }
                    break;
                }

                case 's': {
                    int len = ac->length;
                    char *formatted = formatStringValue(attrPtr, len);
                    if (formatted) {
                        printf(" %-*s |", colWidths[idx] - 2, formatted);
                        free(formatted);
                    } else {
                        printf(" %-*s |", colWidths[idx] - 2, "ERROR");
                    }
                    break;
                }

                default: {
                    char *unknown = strdup("(unknown)");
                    if (unknown) {
                        printf(" %-*s |", colWidths[idx] - 2, unknown);
                        free(unknown);
                    } else {
                        printf(" %-*s |", colWidths[idx] - 2, "ERROR");
                    }
                    break;
                }
            }
        }

        printf("\n");
        rowCount++;
    }

    /* --------- Footer --------- */
    if (rowCount > 0) {
        printSeparator(catcache[r].attrList, colWidths);
    }
    printf("%d row%s in set\n", rowCount, (rowCount == 1 ? "" : "s"));

    free(recPtr);
    free(colWidths);
    return OK;
}

/* ============================================================= */
/* --------- Helper functions --------- */

static void printSeparator(AttrDesc *attrList, int *colWidths)
{
    int idx = 0;
    printf("+");
    for(AttrDesc *p = attrList; p; p = p->next, idx++)
    {
        for(int i = 0; i < colWidths[idx]; i++)
            printf("-");
        printf("+");
    }
    printf("\n");
}

static void printHeader(AttrDesc *attrList, int *colWidths)
{
    printSeparator(attrList, colWidths);
    printf("|");
    int idx = 0;
    for(AttrDesc *p = attrList; p; p = p->next, idx++)
    {
        printf(" %-*s |", colWidths[idx] - 2, p->attr.attrName);
    }
    printf("\n");
    printSeparator(attrList, colWidths);
}

static char* formatIntValue(int value)
{
    // First pass: get the length needed
    int len = snprintf(NULL, 0, "%d", value);
    if (len < 0) return NULL;
    
    // Allocate exactly the right amount
    char *buffer = (char *)malloc(len + 1);
    if (!buffer) return NULL;
    
    // Second pass: write the formatted string
    snprintf(buffer, len + 1, "%d", value);
    return buffer;
}

static char* formatFloatValue(float value)
{
    char *buffer = NULL;
    
    // Handle special cases without math.h functions
    // Check for NaN (Not a Number) - NaN is not equal to itself
    if (value != value) {
        buffer = strdup("NaN");
        return buffer;
    }
    
    // Check for infinity by comparing with very large values
    // Note: This is a simplified check that works for most cases
    if (value > 1e38f || value < -1e38f) {
        if (value > 0) {
            buffer = strdup("Inf");
        } else {
            buffer = strdup("-Inf");
        }
        return buffer;
    }
    
    // Check if the value is exactly zero
    if (value == 0.0f) {
        buffer = strdup("0.00");
        return buffer;
    }
    
    // Get absolute value without fabsf
    float absValue = value;
    if (absValue < 0) absValue = -absValue;
    
    // Determine if we should use scientific notation
    int useScientific = 0;
    
    // Very large or very small numbers use scientific notation
    if (absValue >= 1e7f || (absValue < 0.01f && absValue > 0.0f)) {
        useScientific = 1;
    } else {
        // For numbers in the "normal" range, check if rounding to 2 decimal places
        // accurately represents the number
        
        // Manual rounding to 2 decimal places without roundf
        float rounded;
        if (value >= 0) {
            // For positive numbers: multiply by 100, add 0.5, truncate, divide by 100
            rounded = (float)((int)(value * 100.0f + 0.5f)) / 100.0f;
        } else {
            // For negative numbers: multiply by 100, subtract 0.5, truncate, divide by 100
            rounded = (float)((int)(value * 100.0f - 0.5f)) / 100.0f;
        }
        
        // Calculate the absolute difference
        float diff = rounded - value;
        if (diff < 0) diff = -diff;
        
        // Calculate the relative difference
        float relDiff = diff;
        float valAbs = value;
        if (valAbs < 0) valAbs = -valAbs;
        if (valAbs != 0) relDiff = diff / valAbs;
        
        // If the relative difference is significant (> 0.5%),
        // or if the absolute difference is more than half of the last digit (0.005),
        // then the number is not accurately represented by 2 decimal places
        if (relDiff > 0.005f || diff > 0.005f) {
            useScientific = 1;
        }
    }
    
    // Format the number appropriately
    if (useScientific) {
        // Use scientific notation with 2 decimal places
        int len = snprintf(NULL, 0, "%.2e", value);
        if (len < 0) return NULL;
        
        buffer = (char *)malloc(len + 1);
        if (!buffer) return NULL;
        
        snprintf(buffer, len + 1, "%.2e", value);
    } else {
        // Use fixed-point notation with 2 decimal places
        int len = snprintf(NULL, 0, "%.2f", value);
        if (len < 0) return NULL;
        
        buffer = (char *)malloc(len + 1);
        if (!buffer) return NULL;
        
        snprintf(buffer, len + 1, "%.2f", value);
    }
    
    return buffer;
}

static char* formatStringValue(const char *data, int length)
{
    if (!data || length <= 0) {
        // Return an empty string
        char *empty = (char *)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }
    
    // Find actual string length (trim trailing nulls/spaces)
    int actualLen = length;
    while (actualLen > 0 && (data[actualLen - 1] == '\0' || data[actualLen - 1] == ' ')) {
        actualLen--;
    }
    
    if (actualLen <= 0) {
        // Return an empty string
        char *empty = (char *)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }
    
    // Allocate exactly what we need
    char *buffer = (char *)malloc(actualLen + 1);
    if (!buffer) return NULL;
    
    // Copy the data
    memcpy(buffer, data, actualLen);
    buffer[actualLen] = '\0';
    
    return buffer;
}