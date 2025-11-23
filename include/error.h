/************************************************************
   This file contains the error constants that are
   used by the routine that reports error messages.
   As you find error conditions to report, 
   make additions to this file. 
************************************************************/
#ifndef ERRORS_MINIREL
#define ERRORS_MINIREL
#include <stdbool.h>
int ErrorMsgs(int errorNum, bool printFlag);
#define ARGC_INSUFFICIENT 601 /* Insufficient number of arguments passed */
#define METADATA_SECURITY 602 /* Metadata table modification error */
#define BUFFER_FULL 603 /* Buffer is full */
#define INVALID_FORMAT 604 /* Attribute has invalid format */
#define STR_LEN_INVALID 605 /* String length limit for attribute is more than 50 */
#define TOO_MANY_ARGS   606 /* Too many command line arguments passed */
#define RELNOEXIST  101   /* Relation does not exist */
#define RELEXIST    102   /* Relation already exists */
#define REL_LENGTH_EXCEEDED 103 /* Relation name too long */
#define REL_OPEN_ERROR 104 /* Relation not open */
#define REL_CLOSE_ERROR 105 /* Relation not closed */
#define INVALID_RELNUM 106 /* Relation number is invalid */
#define ATTRNOEXIST 201   /* Attribute does not exist */
#define ATTREXIST   202   /* Attribute already exists */
#define ATTR_NAME_EXCEEDED 203 /* Attribute name too long */
#define DUP_ATTR    204   /* Duplicate attributes in table name */
#define INVALID_VALUE 205 /* Value does NOT match field type */
#define DUP_ATTR_INSERT 206 /* Repeated definition of attribute value during insertion */
#define DUP_ROWS     207 /* CANNOT insert duplicate row into relation */
#define REC_INS_ERR     207 /* Some error in inserting record */
#define REC_DEL_ERR     208 /* Some error in deleting a record */
#define INCOMPATIBLE_TYPES  209 /* Attribute types for Join are incompatible */
#define CAT_CREATE_ERROR   301 /* Error in creating catalog tables */
#define CAT_OPEN_ERROR 302 /* Error in opening catalog files */
#define CAT_ALREADY_EXISTS 303 /* Something went wrong. Catalog files already exist */
#define CAT_CLOSE_ERROR 304 /* Error in closing catalog files */
#define CAT_DELETE_ERROR 305 /* Catalog file can't be deleted */
#define MEM_ALLOC_ERROR 401 /* Memory could not be allocated for processing query */
#define REC_TOO_LONG    402 /* The relation record is too long to fit in one page*/
#define DBNOTEXIST  501   /* DB does NOT exist */
#define DBEXISTS    502   /* DB already exists */
#define DB_LENGTH_EXCEEDED 503 /* DB name too long */
#define DBNOTCLOSED 504 /* Current DB not yet closed */
#define DBNOTOPEN 505 /* No DB is currently open */
#define DBPATHNOTVALID 506 /* The path to the database is NOT valid */
#define DBPREFIXNOTFOUND 507 /* A prefix to the path does NOT exist */
#define DBDESTROYERROR  508 /* DB directory could not be deleted */
#define FILESYSTEM_ERROR   801 /* OS gives an error */
#define FILE_NO_EXIST   802 /* The file does NOT exist */
#define LOAD_NONEMPTY   803 /* Attempt to load data into a non-empty relation */
#define INDEX_NONEMPTY  804 /* Attempt to create an index for an attribute of a non-empty relation*/
#define UNKNOWN_ERROR   901 /* Some unknown error */
#define PAGE_MAGIC_ERROR 902 /* Page does not belong to MINIREL */
#define PAGE_OUT_OF_BOUNDS 903 /* Page not in relation */
#define INVALID_FILE_SIZE 904 /* Trying to load an invalid file */
#endif
