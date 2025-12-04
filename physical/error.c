/************************INCLUDES*******************************/

#include <stdio.h>
#include <stdbool.h>
#include "../include/error.h"


/*--------------------------------------------------------------

FUNCTION ErrorMsgs (errorNum, printFlag):

PARAMETER DESCRIPTION:
    errorNum  → Integer error code representing the type of error.
    printFlag → Boolean flag indicating whether the error message should be printed (non-zero = print, 0 = silent).

FUNCTION DESCRIPTION:
    This routine centralizes all system-level and command-level error reporting in MINIREL. 
    Given an error code, it prints the corresponding descriptive message only if printFlag is true.
    Regardless of printing, it returns the same error code to allow usage such as:
        return ErrorMsgs(db_err_code, print_flag);

ALGORITHM:
    1) If printFlag is false, return errorNum immediately.
    2) Switch on errorNum:
        - For each known MINIREL error code, print a descriptive, user-friendly message explaining:
            • what went wrong,
            • why it likely happened,
            • what the user should do next.
    3) If errorNum does not match any known code, print a generic “unrecognized error code” message.
    4) Return errorNum.

GLOBAL VARIABLES MODIFIED:
    None.

---------------------------------------------------------------*/

int ErrorMsgs(int errorNum, bool printFlag)
{
    if(printFlag)
    {
        switch(errorNum)
        {
            case ARGC_INSUFFICIENT:
                printf("Error %d: Insufficient number of arguments passed.\n"
                       "→ Ensure you provide the required arguments in the correct order.\n",
                       errorNum);
                break;

            case METADATA_SECURITY:
                printf("Error %d: Metadata table modification error.\n"
                       "→ Direct modification of system metadata is not allowed.\n"
                       "  Use provided APIs or commands to alter metadata safely.\n",
                       errorNum);
                break;
            
            case FILE_NO_EXIST:
                    printf("Error %d: File that you are trying to load from does'nt exist.\n", errorNum);
                break;
            
            case LOAD_NONEMPTY:
                    printf("Error %d: CANNOT load data into a non-empty relation.\n", errorNum);
                break;
            
            case INDEX_NONEMPTY:
                    printf("Error %d: CANNOT create index for an attribute of a non-empty relation.\n", errorNum);
                    break;
            
            case IDXNOEXIST:
                    printf("Error %d: Index on the given attribute for the given relation does NOT exist.\n", errorNum);
                    break;
            
            case IDXEXIST:
                    printf("Error %d: Index already exists on given attribute of the relation.\n", errorNum);
                    break;

            case RELNOEXIST:
                printf("Error %d: Relation does not exist.\n"
                       "→ Check the relation name for typos or create the relation before use.\n",
                       errorNum);
                break;
            
            case REL_OPEN_ERROR:
                printf("Error %d: Relation file could not be opened ...\n", errorNum);
                break;

            case RELEXIST:
                printf("Error %d: Relation already exists.\n"
                       "→ Choose a different relation name or drop the existing relation first.\n",
                       errorNum);
                break;

            case REL_LENGTH_EXCEEDED:
                printf("Error %d: Relation name too long.\n"
                       "→ Use a shorter name within the allowed length.\n",
                       errorNum);
                break;

            case ATTRNOEXIST:
                printf("Error %d: Attribute does not exist.\n"
                       "→ Verify that the attribute name is correct and exists in the relation.\n",
                       errorNum);
                break;

            case ATTREXIST:
                printf("Error %d: Attribute already exists.\n"
                       "→ Use a unique attribute name when creating or altering a table.\n",
                       errorNum);
                break;

            case ATTR_NAME_EXCEEDED:
                printf("Error %d: Attribute name too long.\n"
                       "→ Shorten the attribute name to meet the allowed length.\n",
                       errorNum);
                break;

            case DUP_ATTR:
                printf("Error %d: Duplicate attributes in table definition.\n"
                       "→ Remove duplicate attributes or rename them uniquely.\n",
                       errorNum);
                break;
            
            case ATTR_SET_INVALID:
                printf("Error %d: ONE or more attribute values missing for insertion.\n"
                , errorNum);
                break;
            
            case DUP_ATTR_INSERT:
                printf("Error %d: Duplicate assignment of fields in insert query NOT allowed\n", errorNum);
            
            case DUP_ROWS:
                    printf("Error %d: CANNOT have duplicate tuples in relation..\n", errorNum);
                    break;
            
            case INVALID_VALUE:
                    printf("Error %d: You have entered an invalid value for a field that is incompatible with its type..\n", errorNum);
                    break;
            
            case INCOMPATIBLE_TYPES:
                    printf("Error %d: Join attributes of BOTH the relations are of incompatible types..\n", errorNum);
                    break;
            
            case INVALID_FORMAT:
                    printf("Error %d: One or more type(s) you have specified CANNOT be understood..", errorNum);
                    break;
            
            case STR_LEN_INVALID:
                    printf("Error %d: Length of a string attribute has to be between 0 and 50.\n", errorNum);
            
            case CAT_CREATE_ERROR:
                printf("Error %d: The catalog files could NOT be created.\n"
                "Please try again after closing any processes that are using this directory.\n", errorNum);
                break;
            
            case CAT_OPEN_ERROR:
                printf("Error %d: Could NOT open catalogs for this database.\n"
                "Maybe you have chosen an invalid directory.\n", errorNum);
                break;
            
            case CAT_DELETE_ERROR:
                printf("Error %d: Could NOT DELETE catalogs for the database\n"
                , errorNum);
                break;
            
            case CAT_ALREADY_EXISTS:
                printf("Error %d: Catalogs already exist for the database here.\n", errorNum);
                break;

            case MEM_ALLOC_ERROR:
                    printf("Error %d: Memory could NOT be allocated to perform this operation.\n", errorNum);
                    break;
            
            case REC_TOO_LONG:
                    printf("Error %d: This relation CANNOT be created as its record would NOT fit within a page.\n", errorNum);
                    break;
            
            case REL_PAGE_LIMIT_REACHED:
                    printf("Error %d: ONLY 32768 pages supported per relation. CANNOT add new record\n", errorNum);
                    break;
            
            case BUFFER_FULL:
                    printf("Error %d: Buffer of relation catalog cache is full. CANNOT open any more relations.\n", errorNum);
                    break;

            case DBNOTEXIST:
                printf("Error %d: Database does not exist.\n"
                       "→ Verify the database name or create the database first.\n",
                       errorNum);
                break;
            
            case DBPREFIXNOTFOUND:
                    printf("Error %d: Prefix to your Database path does NOT exist.\n"
                    "Implementation ONLY supports ONE level of directory creation", errorNum);

            case DBEXISTS:
                printf("Error %d: Database already exists.\n"
                       "→ Choose another name or drop the existing database.\n",
                       errorNum);
                break;

            case DB_LENGTH_EXCEEDED:
                printf("Error %d: Database name too long.\n"
                       "→ Use a shorter database name within the allowed length.\n",
                       errorNum);
                break;

            case DBNOTCLOSED:
                printf("Error %d: Current database not yet closed.\n"
                       "→ Close the currently open database before performing this operation.\n",
                       errorNum);
                break;

            case DBNOTOPEN:
                printf("Error %d: No database is currently open.\n"
                       "→ Open a database before attempting this operation.\n",
                       errorNum);
                break;

            case DBPATHNOTVALID:
                    printf("Error %d: The path you have provided for DB creation contains illegal characters.\n" 
                        "Only paths containing alphabets and digits other than / are allowed.\n", errorNum);
                    break;
            
            case PATH_NOT_VALID:
                    printf("Error %d: The path you have provided to load a file contains illegal characters"
                        "Only paths containing alphabets and digits other than / are allowed.\n", errorNum);
                    break;
            
            case DBDESTROYERROR:
                    printf("Deletion of DB directory NOT successful..\n");
                    break;

            case FILESYSTEM_ERROR:
                printf("Error %d: File system error occurred.\n"
                       "→ Check OS-level file permissions, disk space, or path validity.\n",
                       errorNum);
                break;
            
            case PAGE_MAGIC_ERROR:
                printf("Error %d: Page does NOT have a valid start magic sequence.." 
                "File may be corrupted.\n", errorNum);
                break;
            
            case PAGE_OUT_OF_BOUNDS:
                printf("Error %d: Trying to read page index NOT in the relation.\n", errorNum);
                break;

            case UNKNOWN_ERROR:
                printf("Error %d: Unknown error encountered.\n"
                       "→ Recheck the operation. If the issue persists, consult logs or support.\n",
                       errorNum);
                break;

            default:
                printf("Error %d: Unrecognized error code.\n"
                       "→ Check if the error code is defined in errors.h.\n",
                       errorNum);
                break;
        }
    }
    return errorNum;
}
