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
#define RELNOEXIST  101   /* Relation does not exist */
#define RELEXIST    102   /* Relation already exists */
#define REL_LENGTH_EXCEEDED 103 /* Relation name too long */
#define ATTRNOEXIST 201   /* Attribute does not exist */
#define ATTREXIST   202   /* Attribute already exists */
#define ATTR_NAME_EXCEEDED 203 /* Attribute name too long */
#define DUP_ATTR    204   /* Duplicate attributes in table name */
#define CAT_CREATE_ERROR   301 /* Error in creating catalog tables */
#define CAT_OPEN_ERROR 302 /* Error in opening catalog files */
#define CAT_ALREADY_EXISTS 303 /* Something went wrong. Catalog files already exist */
#define CAT_CLOSE_ERROR 304 /* Error in closing catalog files */
#define DBNOTEXIST  501   /* DB does NOT exist */
#define DBEXISTS    502   /* DB already exists */
#define DB_LENGTH_EXCEEDED 503 /* DB name too long */
#define DBNOTCLOSED 504 /* Current DB not yet closed */
#define DBNOTOPEN 505 /* No DB is currently open */
#define DBPATHNOTVALID 506 /* The path to the database is NOT valid */
#define DBPREFIXNOTFOUND 507 /* A prefix to the path does NOT exist */
#define DBDESTROYERROR  508 /* DB directory could not be deleted */
#define FILESYSTEM_ERROR   801 /* OS gives an error */
#define UNKNOWN_ERROR   901 /* Some unknown error */
#endif
