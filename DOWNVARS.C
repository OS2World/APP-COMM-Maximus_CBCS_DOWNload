
/* ============================================================= */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility  */
/*  -> DOWNVARS.C                                                */
/*  -> Global data: variables and tables                         */
/* ============================================================= */

#define INCL_BASE
#define INCL_NOPMAPI
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "..\max\mstruct.h"
#include "downsort.h"

char VERSION      = '5';                /* current version */
char SUBVERS      = '9';                /* current release */
char SUFFIX       = 'q';                /* development suffix */

char *AC         = "Area";
char *AUTHOR     = "Rob Hamerling";    /* program author */
char *BAK        = "BAK";              /* backup file extension */
char *BY         = "Bytes";
char *CFG        = "CFG";              /* config file extension */
char *CITY       = "Vianen, The Netherlands";
char *DAYS       = "days ";
char *DF         = "Date flag: new on this system since:";
char *DOT        = ".";
char *DS         = "Description";
char *DT         = "Date";
char *EMPTY      = "";                 /* empty string */
char *FIDO       = "Fido-net 2:512/4.1098";
char *FILPREFX   = "-ë ";
char *FN         = "Filename";
char *FP         = "FilePath";
char *FS         = "Files";
char *MAX        = "MAXIMUS CBCS";
char *MO         = " = 1 month";
char *MONTHS     = "months ";
char *MP         = "Maximum privilege shown: ";
char *MSG_ARC    = "\tPlaceholder added for empty area (%s)\n";
char *MSG_ARD    = "%6ld\tfiles (+%ld) after "
                     "merging file descriptions for area %s\n";
char *MSG_ARE    = "%6ld\tfiles in DL-dir %s (area %s)\r";
char *MSG_ARO    = "%6ld\tfiles after removing %ld orphans in area %s\n";
char *MSG_CFG    = "Processing configuration file: %s\n";
char *MSG_COA    = "Collecting filearea information from %s\n";
char *MSG_COF    = "Collecting file data\n";
char *MSG_IEC    = "%s internal error in data collection phase:\n"
                     "\tentry-count (%lu) not equal chainsize (%lu)\n";
char *MSG_KWD    = "Unknown keyword or missing parameter value in:"
                     "\n\t%-50.50s\n";
char *MSG_MEM    = "Insufficient memory, %s terminated\n";
char *MSG_MX1    = "Sorry, %s-file has unsupported AREA.DAT format!\n";
char *MSG_MX2    = "%s %c.%c%c works only for %s version 2.00 and up!\n";
char *MSG_OPA    = "\tOpen Action error for %s: %d\n";
char *MSG_OPC    = "\tCould not open configurationfile: %s, rc=%d\n";
char *MSG_OPI    = "\tCould not open inputfile: %s, rc=%d\n";
char *MSG_OPO    = "\tCould not open outputfile: %s, rc=%d\n";
char *MSG_ORP    = "%6ld\torphans detected and dropped in total\n";
char *MSG_PAR    = "Configuration file processing completed\n";
char *MSG_REC    = " \tFile entries written\r";
char *MSG_REP    = "Writing %s\n";
char *MSG_RST    = "Re-sorting first %lu entries on filename\n";
char *MSG_SRT    = "Sorting %lu entries from %hu areas for %s\n";
char *MSG_TRL    = "\tWarning: Include-file - %s - not found!\n";
char *MSG_UNK    = "Unknown Parameter: %-50.50s\n";
char *MSG_ZF     = "Not a single list%s falls withing report criteria, or\n";
char *MSG_ZP     = "no download paths found at all, %s terminated\n";
char  NDS[46]      = "--- no description available ---";
char  OFFLINE[16]  = " -- OFFLINE -- ";
char  ORPHAN[46]   = "************ ORPHAN ************";
char *PHONE      = "Phone 31.3473.72136 (voice)";
char *PROGDESC   = "DOWNload file SORT and list utility";
char *PROGNAME   = "DOWNSORT";         /* program name (and list-fnames)*/
char *SZ         = "Size";
char *SUMM       = " Summary";
char *TM         = "Time";
char *WEEKS      = "weeks ";
char *WK         = " = 1 week";
char *WRITE      = "w";

char *priv_name[] = {"Twit",
                     "#",               /* not used */
                     "Disgrace",
                     "Limited",
                     "Normal",
                     "Worthy",
                     "Privil",
                     "Favored",
                     "Extra",
                     "Clerk",
                     "AsstSysop",
                     "$",               /* not used */
                     "Sysop",
                     "Hidden"};

char today[26];                         /* output timestamp */
char list_title[21]   = "";             /* ALL/IPF/NEW/GBL BLOCK title */

STRCHAIN *pre_title   = NULL;           /* pretitles */
STRCHAIN *sub_title   = NULL;           /* subtitles */
STRCHAIN *bot_title   = NULL;           /* bottitles */
STRCHAIN *incl_area   = NULL;           /* chain of INcluded area's */
STRCHAIN *excl_area   = NULL;           /* chain of EXcluded area's */
STRCHAIN *excl_file   = NULL;           /* chain of excluded files */

DUPCHAIN *non_dup_ext = NULL;           /* non dup extensions */

char areadat_path[MAXPATH] = "AREA.DAT";  /* (default) AREA.DAT filespec */
char cfg_path[MAXPATH] = "DOWNSORT.CFG";  /* (default) config filespec   */
char buf2[MAXRCD]         = "";         /* text manipulation buffer */
char filesbbs_path[MAXPATH] = "";       /* path to FILES.BBS output */
char oper_mode            = ' ';        /* blank: give some progress info*/
                                        /* V=verbose: extensive reporting*/
                                        /* Q=quit (only error messages) */
                                        /* H: display help-screen */
int     area_seq          = GROUP;      /* area sort sequence */
char    strip_ava         = 'Y';        /* default: strip AVATAR graphics*/
char    keepseq           = ' ';        /* 'K' if any list has KEEPSEQ */
USHORT  max_aname         = 0;          /* actual maximum length areaname*/
USHORT  area_total_count  = 0;          /* number of area's with files */
ULONG   file_total_count  = 0;          /* counter for number of files */
ULONG   byte_count        = 0;          /* counter for file sizes */
ULONG   areakeys          = 0x00000000; /* cumulative report keys */
short int MAX_level       = 0;          /* version of MAX from AREA.DAT */
short int ABS_MAX_priv    = TWIT;       /* overall max report-privilege */
FILECHAIN  *first_element = NULL;       /* ptr first chain element */
COUNTRYCODE c_code;                     /* country code buffer */
COUNTRYINFO c_info;                     /* country information buffer */

LISTPARM  lp[]  = {
      {                                 /* BBS-list (P_BBS) */
        NULL, 0L, P_BBS, HIDDEN+1,
        FONT3, 65535, 0, "DOWNSORT.HDR",
        ' ', TIMESTAMP, TRUNC, ' ', ' ', RUBBDATE,
        "DOWNSORT", "BBS"},
      {                                 /* FILES.BBS (P_FIL) */
        NULL, 0L, P_FIL, HIDDEN+1,
        FONT3, 65535, 0, NULL,
        ' ', KEEPSEQ, TRUNC, ' ', ' ', ' ',
        "FILES", "BBS"},
      {                                 /* GBL-list (P_GBL) */
        NULL, 0L, P_GBL, HIDDEN+1,
        FONT3, 65535, 0, NULL,
        ' ', ALPHA, WRAP, ' ', ' ', ' ',
        "DOWNSORT", "G~"},
      {                                 /* NEW-list (P_NEW) */
        NULL, 0L, P_NEW, HIDDEN+1,
        FONT3, 65535, 0, NULL,
        ' ', TIMESTAMP, TRUNC, ' ', ' ', RUBBDATE,
        "DOWNSORT", "N~"},
      {                                 /* ALL-list (P_ALL) */
        NULL, 0L, P_ALL, HIDDEN+1,
        FONT3, 65535, 0, NULL,
        ' ', ALPHA, WRAP, ' ', ' ', ' ',
        "DOWNSORT", "A~"},
      {                                 /* IPF-list (P_IPF) */
        NULL, 0L, P_IPF, HIDDEN+1,
        FONT3, 250, 0, NULL,
        ' ', ALPHA, WRAP, ' ', ' ', ' ',
        "DOWNSORT", "I~"},
      {                                 /* DUP-list (P_DUP) */
        NULL, 0L, P_DUP, HIDDEN+1,
        FONT3, 65535, 0, NULL,
        ' ', ALPHA, WRAP, ' ', ' ', ' ',
        "DOWNSORT", "DUP"},
      {                                 /* ORP-list (P_ORP) */
        NULL, 0L, P_ORP, HIDDEN+1,
        FONT3, 65535, 0, NULL,
        ' ', ALPHA, TRUNC, ' ', ' ', ' ',
        "DOWNSORT", "ORP"},
      {                                 /* OK-file (P_OK) */
        NULL, 0L, P_OK, HIDDEN+1,
        FONT3, 65535, 0, NULL,
        ' ', ALPHA, TRUNC, ' ', ' ', ' ',
        "DOWNSORT", "O~"},
      {                                 /* IPF2-file (P_IP2) */
        NULL, 0L, P_IP2, HIDDEN+1,
        FONT3, 250, 0, NULL,
        ' ', ALPHA, WRAP, ' ', ' ', ' ',
        "DOWNSORT", "I~"},
      {                                 /* EMI-file (P_EMI) */
        NULL, 0L, P_EMI, HIDDEN+1,
        FONT3, 65535, 0, NULL,
        ' ', TIMESTAMP, WRAP, ' ', ' ', RUBBDATE,
        "DOWNSORT", "E~"},
         };

