/* ===================================== */
/* Header file for DOWNSORT : DOWNSORT.H */
/* ===================================== */

/* used in Maximus CBCS structures */

#ifndef __32BIT__
  #define _HUGE  huge                   /* migration! */
#else
  #define _HUGE                         /* empty */
#endif

#define MAXANAME    10
#define MAXFN       13
#define MAXRCD    2048
#define MAXDESC    (MAXRCD-MAXFN-3)
#define MAXPATH    256
#define MAXTIT      20

#define QMARK       '?'
#define ALPHA       'A'
#define CONT        'C'
#define TIMESTAMP   'D'
#define FONT        'F'
#define GROUP       'G'
#define HELP        'H'
#define INCLUDE     'I'
#define KEEPSEQ     'K'
#define LONGLIST    'L'
#define QUIET       'Q'
#define RUBBDATE    '-'
#define TRUNC       'T'
#define VERBOSE     'V'
#define WRAP        'W'
#define EXCLPRIV    'X'

#define FALSE      0
#define  TRUE      1

#define AVA_A      1                 /* ^A  */
#define AVA_B      2                 /* ^B  */
#define AVA_C      3                 /* ^C  */
#define AVA_D      4                 /* ^D  */
#define AVA_E      5                 /* ^E  */
#define AVA_F      6                 /* ^F  */
#define AVA_G      7                 /* ^G  */
#define AVA_H      8                 /* ^H  */
#define AVA_I      9                 /* ^I  */
#define AVA_J      10                /* ^J  */
#define AVA_K      11                /* ^K  */
#define AVA_L      12                /* ^L  */
#define AVA_M      13                /* ^M  */
#define AVA_N      14                /* ^N  */
#define AVA_V      22                /* ^V  */
#define AVA_W      23                /* ^W  */
#define AVA_Y      25                /* ^Y  */

#define DAYS_30     '+'
#define DAYS_7      '*'

#define FONT0       0
#define FONT1       1
#define FONT2       2
#define FONT3       3
#define FONT4       4

enum  {LINE1, LINE2, LINE3, LINE4, LINE5};

/* (foreground) COLORS */

#define O_BLINK   0x80
#define O_BRIGHT  0x08
#define O_LIGHT   0x08

#define O_BLACK   0x00
#define O_BLUE    0x01
#define O_GREEN   0x02
#define O_CYAN    0x03
#define O_RED     0x04
#define O_MAGENTA 0x05
#define O_BROWN   0x06
#define O_GRAY    0x07
#define O_YELLOW  (O_BRIGHT+O_BROWN)
#define O_WHITE   (O_BRIGHT+O_GRAY)

typedef struct _filechain {             /* file information           */
        struct _filechain *next_element;  /* pointer to next element  */
        struct _downpath *parea;        /* pointer to area-structure  */
        char   *fdesc;                  /* ptr to file description    */
        char   *fpath;                  /* ptr to file path           */
        ULONG  size;                    /* file size                  */
        union  {
                FDATE   date;          /* file date (last write)     */
                unsigned int idate;
                } wd;
        union  {
                FTIME   time;          /* file time (last write)     */
                unsigned int itime;
                } wt;
        union  {
                FDATE   date;           /* file date (creation)       */
                unsigned int idate;
                 } cd;
        union   {
                FTIME   time;           /* file time (creation)       */
                unsigned int itime;
                 } ct;
        USHORT fseq;                    /* FILES.BBS sequence number  */
        short  int priv;                /* file privilege             */
        unsigned int dl_b:1,            /* unlimited download (bytes) */
                     dl_t:1,            /* unlimited download (time)  */
                     cmt:1;             /* files.bbs comment entry    */
        char   fname[MAXFN];            /* filename                   */
          } FILECHAIN;

typedef struct _downpath {              /* download area information  */
        struct _filechain *newest;      /* pointer to most recent     */
        ULONG  byte_count;              /* Area byte count area summ. */
        ULONG  file_count;              /* Area file count area summ. */
        short int priv;                 /* Area Privilege             */
        short int anum;                 /* Area sequence number       */
        ULONG  filelock;                /* MAX area file lock bits    */
        char   name[MAXANAME];          /* MAX Area name              */
        char   ename[MAXANAME];         /* edited right aligned       */
        char   pname[80];               /* Download-pathname          */
        char   filesbbs[80];            /* Path to FILES.BBS          */
        char   adesc[80];               /* Filearea description       */
          } DOWNPATH;

enum  ListType {P_BBS, P_FIL, P_GBL, P_NEW, P_ALL,
                P_IPF, P_DUP, P_ORP, P_OK, P_IP2, P_EMI,
                P_MAX=P_EMI};

typedef struct _listparm {              /* list parameters            */
        struct _listparm *next;         /* pointer to next list parms */
        ULONG  userkeys;                /* userkeys for filelocks     */
        short  int id;                  /* list type indicator        */
        short  int priv;                /* max file priv to list      */
        short  int tfont;               /* title font                 */
        SHORT  max_fil;                 /* maximum # of files to list */
        short int desc_indent;          /* description cont. indent   */
        char   *incl_fspec;             /* ptr to include-filespec    */
        char   longflag,                /* long OK-file or FILES.BBS  */
               sortflag,                /* file seq. (name/date/keep) */
               wrapflag,                /* wrap/truncate file descr.  */
               exclflag,                /* privilege exclude flag     */
               listflag,                /* max list length in number  */
                                        /*   or in days, weeks, months*/
               rubbflag;                /* report rubbish CDROM dates */
        char   name[9];                 /* list filename              */
        char   ext[4];                  /* list extension             */
          } LISTPARM;

typedef struct _strchain {              /* string chain element */
        struct _strchain *next;         /* pointer to next */
        char   *str;                    /* pointer to string */
          } STRCHAIN;

typedef struct _dupchain {              /* Duplicate .ext chain element */
        struct _dupchain *next;         /* pointer to next */
        char   ext1[4],                 /* extension 1 */
               ext2[4];                 /* extension 2 */
          } DUPCHAIN;

typedef struct _areagroup {             /* Duplicate .ext chain element */
        ULONG             keys;         /* userkeys bitstring */
        struct _strchain *incl;         /* pointer to include chain */
        struct _strchain *excl;         /* pointer to exclude chain */
          } AREAGROUP;

/* externs for global variables */

extern char   VERSION,SUBVERS,SUFFIX;
extern char   *PROGNAME, *PROGDESC, *AUTHOR, *CITY, *PHONE, *FIDO, *MAX,
              *MSG_ARC, *MSG_ARD, *MSG_ARE, *MSG_ARO, *MSG_CFG, *MSG_COA,
              *MSG_COF, *MSG_FNT, *MSG_IEC, *MSG_KWD, *MSG_MEM, *MSG_MX1,
              *MSG_MX2, *MSG_OPA, *MSG_OPC, *MSG_OPI, *MSG_OPO, *MSG_ORP,
              *MSG_PAR, *MSG_SRT, *MSG_RST, *MSG_TRL, *MSG_REC, *MSG_REP,
              *MSG_UNK, *MSG_ZF,  *MSG_ZP,
              *OPEN_FAIL, *FILPREFX,
              *BY, *FN, *FS, *AC, *DT, *TM, *SZ, *DS, *FP, *SUMM,
              *MO, *MP, *DF,
              *BAK, *DOT, *EMPTY, *WK, *DAYS, *MONTHS, *WEEKS, *WRITE;
extern char   *priv_name[];
extern char    NDS[],ORPHAN[],OFFLINE[];
extern char    today[], list_title[], areadat_path[],
               filesbbs_path[], cfg_path[], buf2[];
extern STRCHAIN *pre_title, *sub_title, *bot_title;
extern STRCHAIN *incl_area, *excl_area;
extern STRCHAIN *excl_file;
extern STRCHAIN *area_group;
extern DUPCHAIN *non_dup_ext;
extern char   wrap_flag, oper_mode, keepseq;
extern int    area_seq;
extern char   strip_ava;
extern int    area_IN_EX;
extern short int   MAX_level, ABS_MAX_priv;
extern USHORT      area_total_count;
extern USHORT      max_aname;
extern ULONG       areakeys;
extern ULONG       file_total_count;
extern USHORT      title_font;
extern ULONG       byte_count;
extern FILECHAIN  *first_element;
extern LISTPARM    lp[];
extern COUNTRYCODE c_code;
extern COUNTRYINFO c_info;

