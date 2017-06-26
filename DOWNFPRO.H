/* ============================================================= */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility  */
/*  -> DOWNFPRO.H                                                */
/*  -> Function prototypes for 'global' functions                */
/*     that may be called from different reporting functions     */
/* ============================================================= */

char    *asciiz(char *);
void     block_title(FILE *, short int, char *, char *, LISTPARM *);
USHORT   count_areas(DOWNPATH _HUGE *, short int);
ULONG    count_bytes(DOWNPATH _HUGE *);
ULONG    count_files(DOWNPATH _HUGE *);
void     desc_part(FILE *, char *, unsigned int,
                   unsigned int, LISTPARM *);
char    *f_date(FDATE);
char    *f_time(FTIME);
char    *f_size_date(FILECHAIN *);
int      file_age(FILECHAIN *);
char     file_age_ind(FILECHAIN *);
void     file_incl(FILE *, LISTPARM *);
void     insert_title(FILE *, STRCHAIN *, int);
char     *next_word(char *);
FILECHAIN *new_acq(FILECHAIN *, FILECHAIN *);
ULONG    preproc_area(DOWNPATH _HUGE *,
                      FILECHAIN * _HUGE *, LISTPARM *);
void     psort(FILECHAIN * _HUGE *, long int, long int,
               int (*)(FILECHAIN *, FILECHAIN *));
int      rpt_coll(FILECHAIN *, LISTPARM *, int);
ULONG    rpt_count(FILECHAIN * _HUGE *, LISTPARM *, FILECHAIN **);
void     sep_line(FILE *, char, unsigned short int,...);
void     show_help(void);
void     show_welcome(void);
void     signature(FILE *, char *);
int      sort_all(FILECHAIN *, FILECHAIN *);
int      sort_al2(FILECHAIN *, FILECHAIN *);
int      sort_akp(FILECHAIN *, FILECHAIN *);
int      sort_fil(FILECHAIN *, FILECHAIN *);
int      sort_gbl(FILECHAIN *, FILECHAIN *);
int      sort_new(FILECHAIN *, FILECHAIN *);
int      sort_summ(const void *, const void *);
char    *strava(char *);
char    *stripf(char *);
char    *strnblk(char *, short int, short int, short int);
int      strsubw(char *, char **, unsigned int);
char    *sys_date(char *);
int      non_wild_init(short int, char *, char *);
int      wild_comp(char *, char *);
int      wild_init(short int, char *, char *);

