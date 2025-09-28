    /* 
	This is a complete file for the front-end.

    */
/*************************************************************
		CONSTANT AND TYPE DEFINITIONS 
*************************************************************/
#define TRUE		1	/* boolean */
#define FALSE		0

#define	OK			0	/* return codes */
#define NOTOK		-1


/* parser constants */

/* equality operators code */
#define	LOWER   500
#define	EQOP	501		
#define	GEOP	502
#define	GTOP	503
#define	LEOP	504
#define	NOTEQOP	505
#define LTOP	506
#define	HIGH	507

/* indicators */
#define CONTINUE	1	
#define EQUAL		0

/* parser data structure sizes */
#define INPUTLEN	200	
#define ROW			40
#define	COLUMN		80
#define MAXLEN		100

/* prompt */
#define	PROMPT		"query > "	

/*****************************************************************/

#define MAXPARAS  	50
#define PARALEN 	1024

/* reserved names */
#define	TMP_NAME		"tmp_file"	
#define	PRINT_NAME		"print"
#define	DESTROY_NAME	"destroy"

/* token codes */
#define COMMA		100		
#define	NUMBER		101
#define	COLON		108
#define LPARAN		109
#define	RPARAN		110
#define ARROW		111
#define LSQR		112
#define RSQR		113
#define	QUOTEDSTR	114
#define	DOT			115
#define SEMI		116
#define	STR			117

/* Note: Commands codes must be contiguous */

/* commands codes - reserved words */
#define	CREATEDB	201		
#define DESTROYDB	202
#define OPENDB		203
#define	CLOSEDB		204
#define	CREATE		205
#define DESTROY		206
#define	LOAD		207
#define PRINT		208
#define SORT		209
#define QUIT		210
#define SELECT		211
#define PROJECT		212
#define JOIN		213
#define INSERT		214
#define DELETE		215
#define INTERSECT	216
#define UNIONOP		217
#define HELP		218
#define BUILDINDEX	219
#define DROPINDEX	220

/* other reserved words */
#define ON		301		
#define	KEY		302
#define	SIZE	303
#define IS		304
#define FROM	305
#define INTO	306
#define TO		307
#define WHERE	308
#define AND		309
#define FOR		310

/* action codes */
#define	ONE		1		
#define	DONE	0
#define	EAT1	-1
#define EAT3	-3
#define	THROW	-4
#define INC		-5
#define INC2	-6

/* arguments position */
#define RESULTARG	1		
#define	STDARG		2

/* error codes */
#define	LONGSTR			-501		
#define UNMATCHQUOTE	-502
#define NOCOMMAND		-503
#define ILLEGAL			-504

/*****************************************************************/
