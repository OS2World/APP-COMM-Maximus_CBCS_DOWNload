
/* ============================================================= */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility. */
/*  -> DOWNSRV.C                                                 */
/*  -> Collection of general service routines for DOWNSORT.      */
/* ============================================================= */

#define INCL_BASE
#define INCL_NOPMAPI
#include <os2.h>

#include <ctype.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "downsort.h"
#include "downfpro.h"

/* function prototype of local functions */

int  comp_area(DOWNPATH *, DOWNPATH *);


/* ------------------- */
/* Welcome to the user */
/* ------------------- */
void show_welcome(void)
{
  static char HD[22] = "ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
  printf("\n%15sÉ%.22s»\n", "", HD);
  printf("É%.14s¹ %8.8s version %c.%c%cÌ%.15s»\n",
                 HD,PROGNAME,VERSION,SUBVERS,SUFFIX,HD);
  printf("º  Copyright   È%.22s¼   Shareware   º\n", HD);
  printf("º  %s %-38.38sº\n", MAX,PROGDESC);
  printf("º      by %-13.13s, %-29sº\n", AUTHOR,CITY);
  printf("º%-29s%24sº\n", PHONE, FIDO);
  printf("È%.22s%.22s%.9s¼\n", HD, HD, HD);
  }

/* ======================================================== */
/* Compare two filenames.                                   */
/* First name must be regular "8.3" format filename.        */
/* Second name may contain wildcards.                       */
/* ======================================================== */
int wild_comp(char a[],
              char b[])
{
  int i;
  char na[11], nb[11];                  /* formatted filename fields     */
                                        /* keep 'm for subsequent calls  */
  static char empty_ext[]  = { "..." }; /* files without ext.            */

  i = non_wild_init(8,na,a);            /* init non-wild string          */
  switch(a[i]) {                        /* after fileNAME                */
    case ' ' :
    case '\0':
      i = non_wild_init(3,na+8,empty_ext);  /* empty extension           */
      break;
    case '.' :
      i = non_wild_init(3,na+8,a+i+1);  /* process extension             */
      break;
    default:                            /* invalid filename              */
      i = non_wild_init(3,na+8,a+i);    /* pseudo extension              */
      break;
    }

  i = wild_init(8,nb,b);
  switch(b[i]) {
    case ' ' :
    case '\0':
      i = wild_init(3,nb+8,empty_ext);  /* empty extension               */
      break;
    case '.' :
      i = wild_init(3,nb+8,b+i+1);      /* process extension             */
      break;
    default:                            /* invalid filename              */
      i = wild_init(3,nb+8,b+i);        /* pseudo extension              */
      break;
    }

#if defined(__DEBUG__)
  printf("\n");                         /* debug of wild compare         */
  for (i=0; i<11; ++i)
    printf("%c", (na[i] != '\0') ? na[i] : ' ');
  printf(" ");
  for (i=0; i<11; ++i)
    printf("%02.2X", na[i]);
  printf("   ");
  for (i=0; i<11; ++i)
    printf("%c", (nb[i] != '\0') ? nb[i] : ' ');
  printf(" ");
  for (i=0; i<11; ++i)
    printf("%02.2X", nb[i]);
#endif

/* for (i=0; i<11 && (na[i]==nb[i] || na[i]=='\0' || nb[i]=='\0'); i++); */
  for (i=0; i<11 && (na[i]==nb[i] || nb[i]=='\0'); i++);

#if defined(__DEBUG__)
  printf(" =#=%d",i);                    /* debug of wild compare        */
#endif
  if (i>=11)                             /* strings equal                */
    return(0);
  else
    return(na[i]-nb[i]);
  }

/* ============================================ */
/* Init string for wild-card filenames compare. */
/* Translation to UPPER case.                   */
/* ============================================ */
int wild_init(short int n,
              char p[],
              char q[])
{
  int i,j;

  i=j=0;
  while (i<n) {                         /* process string 'q'            */
    switch(q[j]) {
      case '?':                         /* single wild char              */
        p[i++] = '\0';                  /* matches with any other char   */
        j++;                            /* proceed with next char        */
        break;
      case '.':                         /* end of filespec-part          */
      case ' ':                         /* logical end of string         */
      case '\0':                        /* end of string                 */
        while (i<n)                     /* fill                          */
          p[i++] = '.';                 /* insert filler chars           */
        break;
      case '*':                         /* wild string                   */
        while (i<n)                     /* fill                          */
          p[i++] = '\0';                /* matches with any other char   */
        j++;                            /* to next char                  */
        break;
      default:                          /* 'normal' characters           */
        p[i] = (char) toupper(q[i]);    /* convert to UPPERcase and copy */
        i++; j++;                       /* proceed with next char        */
        break;
      }
    }

  return(j);                            /* displ. of last examined char  */
  }

/* ==================================================== */
/* Init string for non-wild-card filenames compare.     */
/* No wild-cards expected, translation to upper case.   */
/* ==================================================== */
int non_wild_init(short int n,
                  char p[],
                  char q[])
{
  int i,j;

  i=j=0;
  while (i<n) {                         /* process string 'q'            */
    if (q[j]=='.' || q[j]==' ' || q[j]=='\0') {
      p[i] = '.';                       /* insert filler char            */
      i++;                              /* rest filler (no j-increment!) */
      }
    else {
      p[i] = (char) toupper(q[i]);      /* to UPPERcase and copy         */
      i++; j++;                         /* proceed with next char        */
      }
    }

  return(j);                            /* displ. of last examined char  */
  }

/* ------------------ */
/* Produce time stamp */
/* ------------------ */
char *sys_date(char t[])
{
  long int secs_now;                    /* relative time value           */
  char *buf;                            /* pointer to time string        */

  time(&secs_now);                      /* get current time              */
  buf = ctime(&secs_now);               /* convert to char string        */
  strcpy(t, buf);                       /* copy string                   */
  t[16] = '\0';                         /* undo secs, year and newline   */
  return(t);                            /* pointer to buffer             */
  }

/* ------------------------------------------------- */
/* Transform file-date into a  9-char string         */
/* COUNTRY format mm-dd-yy (USA) or dd-mm-jj (EUR)   */
/* ------------------------------------------------- */
char *f_date(FDATE date)
{
  static char string[9];                /* work buffer                   */
  sprintf(string,"%2u%s%02u%s%02u",
         ((c_info.fsDateFmt == 0) ? date.month : date.day),
           c_info.szDateSeparator,
         ((c_info.fsDateFmt == 0) ? date.day : date.month),
           c_info.szDateSeparator,
         (date.year+80)%100);           /* allow 2 digits!               */
  return(string);
  }

/* ------------------------------------ */
/* Calculate file-age in number of days */
/* ------------------------------------ */
int file_age(FILECHAIN *f)
{
  static ULONG mon_tab[] = {
               0,0,31,59,90,120,151,181,212,243,273,304,334,334,334,334};
                                        /* ignore leap year February! */
  ULONG  fday;
  FDATE  fd;                            /* copy of filedate */

  fd = f->cd.date;                      /* copy most recent(!) file date */
  fd.year = min(fd.year, 57);          /* max fileyear supported */
  fday    = ((fd.year+10)*1461+1)/4 + mon_tab[fd.month] + fd.day - 1;
  return((int)(time(NULL)/86400L - fday));  /* file age in days */
  }

/* =================== */
/* determine file-age  */
/* =================== */
char  file_age_ind(FILECHAIN *f)
{
  int age;

  age = file_age(f);                    /* get file age */
  if (age>30)
    return(' ');                        /* older than a month */
  else {
    if (age>7)                          /* older than 7 days */
      return(DAYS_30);
    else {
      if (age>=0)                       /* non-negative negative age     */
        return(DAYS_7);                 /* a week                        */
      else
        return('-');                    /* negative age                  */
      }
    }
  }

/* -------------------------------------------------- */
/* Transform file-size and date into a 15-byte string */
/* Size in K (rounded to next higher number KBytes)   */
/* Date format as inf_date(), plus age indicator.     */
/* If date zero: pointer to 'offline'-text.           */
/* -------------------------------------------------- */
char *f_size_date(FILECHAIN *f)
{
  static char string[16];               /* work buffer */
  ULONG  akb;                           /* filesize in KB */

  if (f->wd.date.day) {                 /* date non-zero */
    akb = (f->size+1023)/1024;          /* (rounded) Kbytes value */
    sprintf(string,"%4lu%c ",           /* filesize */
           (akb <= 9999) ? akb : (akb+512)/1024,
           (akb <= 9999) ? 'K' : 'M');
    strcat(string, f_date(f->wd.date));  /* add formatted date */
    string[14] = file_age_ind(f);       /* add age ind. */
    string[15] = '\0';                  /* end of string */
    return(string);                     /* for 'online' file */
    }
  else
    return(OFFLINE);                    /* for 'offline' file */
  }

/* -------------------------------------------------------------- */
/* Transform file-time into string (hh:mm:ssa)                    */
/* COUNTRY format hh:mm:ssa (12 hr USA) or hh-mm-ss  (24 hr EUR)  */
/* -------------------------------------------------------------- */
char *f_time(FTIME tim)
{
  static char ftime[7] = {'\0'};        /* work buffer                    */
  sprintf(ftime,"%2u%s%02u%c%c",
          (c_info.fsTimeFmt==0 && tim.hours>12) ? tim.hours-12 : tim.hours,
           c_info.szTimeSeparator,
          tim.minutes,
          ((c_info.fsTimeFmt==0) ? ((tim.hours>11) ? 'p' : 'a') : ' '),
          '\0');
  return(ftime);
  }

/* ------------------------------ */
/* Build pointer-array for sorts  */
/* ------------------------------ */
FILECHAIN * _HUGE *prep_sort(ULONG      cnt,
                             FILECHAIN *chn)
{
  ULONG       i;                        /* counter                       */
  FILECHAIN   * _HUGE *dm;              /* pointer to huge array         */
  FILECHAIN  *ca;                       /* pointer to fileinfo struct.   */

#ifndef __32BIT__
  dm = (FILECHAIN * huge *)halloc(cnt, sizeof(FILECHAIN *));
#else
  dm = (FILECHAIN **)malloc(cnt * sizeof(FILECHAIN *));
#endif
  if (dm == NULL) {                     /* not enough memory             */
    printf(MSG_MEM,PROGNAME);
    exit(11);
    }
  ca = chn;                             /* ptr to first file-info        */
  for (i=0; ca != NULL; i++) {          /* init sort array               */
    dm[i] = ca;
    ca = ca->next_element;
    }
  return(dm);
  }

/* ====================================================== */
/* compare for sort on file date+time, filename, areaname */
/* ====================================================== */
int  sort_new(FILECHAIN *p,
              FILECHAIN *q)
{
  int  rc;                              /* returncode */
  unsigned int x, y;                    /* calculation results */

  x = min(p->wd.idate, p->cd.idate);    /* file date p */
  y = min(q->wd.idate, q->cd.idate);    /* file date q */
  if ( x == y ) {                       /* equal file dates */
    x = min(p->wt.itime, p->ct.itime);  /* file time p */
    y = min(q->wt.itime, q->ct.itime);  /* file time q */
    if ( x == y ) {                     /* equal file times */
      rc = stricmp(p->fname,q->fname);
      if (rc)                           /* unequal filenames */
        return(rc);
      else
        return(comp_area(p->parea, q->parea));  /* areaname */
      }
    else
      return( (int)(y - x) );           /* time difference */
    }
  else
    return( (int)(y - x) );             /* date difference */
  }

/* ========================================================== */
/* Compare for sort on case-insensitive filename + area-name  */
/* ========================================================== */
int  sort_gbl(FILECHAIN *p,
              FILECHAIN *q)
{
  int   rc;

  rc = stricmp(p->fname, q->fname);
  if (rc)                               /* unequal filename              */
    return(rc);
  else
    return(comp_area(p->parea, q->parea));
  }

/* ======================================================= */
/* Compare for sort of filearray on filename + area-name   */
/* (for file collect process)                              */
/* ======================================================= */
int  sort_fcol(const void *p,
               const void *q)
{
  int   rc;
  FILECHAIN *a, *b;

  a = *(FILECHAIN **)p;
  b = *(FILECHAIN **)q;
  rc = strcmp(a->fname, b->fname);
  if (rc)                               /* unequal filename              */
    return(rc);
  else
    return(comp_area(a->parea, b->parea));
  }

/* =================================================== */
/* Compare for sort of area-array on download pathname */
/* =================================================== */
int  sort_path(const void *p,
               const void *q)
{
  int  rc;
  DOWNPATH *a, *b;

  a = (DOWNPATH *)p;
  b = (DOWNPATH *)q;
  if ((rc = stricmp(a->pname, b->pname)) == 0)  /* download pathname     */
    rc = stricmp(a->name, b->name);     /* area name                     */
  return(rc);
  }

/* ======================================== */
/* Compare for sort on areaname + filename  */
/* ======================================== */
int   sort_all(FILECHAIN *p,
               FILECHAIN *q)
{
  int   rc;

  rc = comp_area(p->parea, q->parea);
  if (rc)                                 /* unequal areaname            */
    return(rc);
  else
    return(stricmp(p->fname, q->fname));  /* filename                    */
  }

/* ======================================================== */
/* Compare for sort on areaname + file-date/time + filename */
/* ======================================================== */
int   sort_al2(FILECHAIN *p,
               FILECHAIN *q)
{
  int  rc;
  unsigned int x, y;

  rc = comp_area(p->parea, q->parea);
  if (rc)                               /* unequal area-name */
    return(rc);
  else {
    x = min(p->wd.idate, p->cd.idate);
    y = min(q->wd.idate, q->cd.idate);
    if (x==y) {                         /* equal file dates */
      x = min(p->wt.itime, p->ct.itime);
      y = min(q->wt.itime, q->ct.itime);
      if (x==y)                         /* equal file times */
        return(stricmp(p->fname, q->fname)); /* filename */
      else
        return( (int)(y-x) );           /* unequal file dates */
      }
    else
      return( (int)(y-x) );             /* unequal file dates */
    }
  }

/* ======================================================== */
/* Compare for sort on areaname + FILES.BBS sequence number */
/* ======================================================== */
int  sort_akp(FILECHAIN *p,
              FILECHAIN *q)
{
  int   rc;

  rc = comp_area(p->parea, q->parea);
  if (rc)                               /* unequal areacode              */
    return(rc);
  else {
    if (p->fseq != q->fseq)             /* unequal sequence number       */
      return( (int)((p->fseq < q->fseq) ? -1 : +1) );
    else
      return(0);                        /* equal sequence number         */
    }
  }

/* =================================================== */
/* Compare for sort on areaname + privilege + filename */
/* No sort on privilege if below area-privilege.       */
/* =================================================== */
int  sort_fil(FILECHAIN *p,
              FILECHAIN *q)
{
  int   rc;

  rc = comp_area(p->parea, q->parea);
  if (rc)                               /* unequal areacode              */
    return(rc);
  else {
    if (p->priv <= p->parea->priv &&    /* both within area priv         */
        q->priv <= q->parea->priv)
      return(stricmp(p->fname, q->fname));  /* sort on filename          */
    else if (p->priv == q->priv)        /* same privilege                */
      return(stricmp(p->fname, q->fname));  /* filename                  */
    else
      return( (int)(p->priv - q->priv) ); /* file priv                   */
    }
  }

/* ========================================= */
/* Compare for sort on areaname of AREA-info */
/* ========================================= */
int  sort_summ(const void *p,
               const void *q)
{
  DOWNPATH *a, *b;

  a = (DOWNPATH *)p;
  b = (DOWNPATH *)q;
  return( comp_area(a, b) );
  }

/* ========================================= */
/* Compare for sort on areaname of AREA-info */
/* ========================================= */
int   comp_area(DOWNPATH *p,
                DOWNPATH *q)
{
  int x,y;                              /* sequence number in chain */
  STRCHAIN *s, *t;                      /* pointer to chain element */

  switch(area_seq) {
    case KEEPSEQ: return(p->anum - q->anum);
                  break;
    case ALPHA:   return(stricmp(p->name, q->name));
                  break;
    case INCLUDE: if ((s=incl_area) != NULL) {  /* areaINclude used  */
                    for (x=0; s != NULL   &&
                              strnicmp(s->str, p->name, strlen(s->str));
                                x++, s=s->next);  /* just determine x */
                    t = incl_area;              /* head of chain */
                    for (y=0; t != NULL   &&
                              strnicmp(t->str, q->name, strlen(t->str));
                                y++, t=t->next);  /* just determine y */
                    if (x!=y)           /* if not equal */
                      return(x - y);    /* relative position */
                    }                   /* may fall through */
    case GROUP:                         /* default */
    default:      return(stricmp(p->ename, q->ename));
                  break;
    }
  }

/* ---------------------------- */
/* Sort file-info pointer array */
/* ---------------------------- */
void psort(FILECHAIN * _HUGE *arr,
           long int left,
           long int right,
           int (*comp)(FILECHAIN *, FILECHAIN *))
{
  long int  asc, des;
  FILECHAIN *ref, *tmp, *wall;

  if ((right-left) < 1)                 /* too few elements              */
    return;

  asc = left;                           /* left 'wall'                   */
  des = right;                          /* right 'wall'                  */
  ref = arr[(left+right)/2];            /* reference value               */

  do {
    wall = arr[asc];                    /* copy pointer */
    while (comp(wall, ref) < 0)         /* move right                    */
      wall = arr[++asc];                /* copy pointer */
    wall = arr[des];                    /* copy pointer */
    while (comp(wall, ref) > 0)         /* move left                     */
      wall = arr[--des];
    if (asc <= des) {                   /* swap                          */
      tmp = arr[des];
      arr[des--] = arr[asc];
      arr[asc++] = tmp;
      }
    } while (asc <= des);

  if ((des-left) < (right-asc)) {        /* sort smaller part first        */
    if (left < des)
      psort(arr, left, des, comp);
    if (right > asc)
      psort(arr, asc, right, comp);
    }
  else {                                 /* sort now larger part */
    if (right > asc)
      psort(arr, asc, right, comp);
    if (left < des)
      psort(arr, left, des, comp);
    }
  }

/* ============================================ */
/* Compare for newest acquisition in ALL-list   */
/* ============================================ */
FILECHAIN *new_acq(FILECHAIN *p,
                   FILECHAIN *q)
{
  unsigned int x, y;

  if (q==NULL  ||  q->cmt==1)           /* first or comment entry */
    return(p);                          /* then return first */
  x = min(p->wd.idate, p->cd.idate);
  y = min(q->wd.idate, q->cd.idate);
  if (x==y) {                           /* equal file dates */
    x = min(p->wt.itime, p->ct.itime);
    y = min(q->wt.itime, q->ct.itime);
    return((x>y) ? p : q);              /* return most recent */
    }
  else
    return((x>y) ? p : q);              /* return most recent */
  }

/* ---------------------------------------------------- */
/* Determine if file belongs to collection of this list */
/* ---------------------------------------------------- */
int  rpt_coll(FILECHAIN *fx,            /* ptr to file info */
              LISTPARM  *ls,            /* list specifications */
              int       ign)            /* ignore file age (boolean) */
{
  long  age,                            /* file age in days */
        maxage;                         /* maximum allowed file age */

  if (fx->priv <= ls->priv) {           /* privilege OK */
    if ((ls->userkeys ^ (fx->parea->filelock | ls->userkeys)) == 0) {
                                        /* have keys for all locks */
      if (fx->fname[0] != '\0') {       /* file name present */
        if (ls->listflag==' ' || ign==FALSE)  /* no selection on file age */
          return(1);                    /* no: file may be listed */
        else {
          age = file_age(fx);           /* file age in days */
          if (age >= 0) {               /* probably valid file date */
            maxage = ls->max_fil *      /* max file age */
                       ((ls->listflag == 'W') ? 7 :
                        ((ls->listflag == 'M') ? 30 : 1));
            return (age <= maxage);     /* true if 'new' */
            }
          else                          /* probably invalid date */
            return(ls->rubbflag != RUBBDATE);    /* negative date allowed */
          }
        }
      else if (ls->sortflag == KEEPSEQ) /* comments to be included */
        return(1);
      }
    }
  return(0);                            /* not member of collection */
  }

/* ---------------------------------------------------- */
/* Determine the number of entries to be included in    */
/* further processing (for re-sorting and/or reporting).*/
/* Array is supposed to be sorted in descending date.   */
/* Returns how many entries (counted from begin of      */
/* array) should take part in further processing        */
/* (sorting, but not necessarily reporting!)            */
/* --------------------------------------------------Ä- */
ULONG  rpt_count(FILECHAIN * _HUGE *dm,   /* ptr to file info array */
               LISTPARM  *ls,           /* list specs */
               FILECHAIN **cn)          /* newest file in collection */
{
  ULONG  i, j;                          /* counters */
  short int maxage;                     /* allowed file age in days */
  FILECHAIN *fx;                        /* local pointer */

  maxage = ls->max_fil *                /* when file age limit */
             ((ls->listflag == 'W') ? 7 :
              ((ls->listflag == 'M') ? 30 : 1));
  *cn = NULL;                           /* none yet */
  for (i=j=0; i<file_total_count; i++) {  /* complete collection */
    fx = dm[i];                         /* copy pointer */
    if (rpt_coll(fx, ls, TRUE)) {       /* check if to be listed */
      j++;                              /* net counter */
      *cn = new_acq(fx, *cn);           /* most recent within collection */
      }
    if (ls->listflag == ' ') {          /* limit: number of entries */
      if (j > ls->max_fil)              /* enough collected */
        break;                          /* escape from loop */
      }
    else {                              /* limit: file age */
      if (file_age(fx) > maxage)        /* enough collected */
        break;                          /* escape from loop */
      }
    }
  return(i);                            /* gross size of collection */
  }

/* =================================== */
/* Include a text file into a report.  */
/* Output file is supposed to be open! */
/* =================================== */
void file_incl(FILE   *pfo,             /* output file pointer           */
               LISTPARM *ls)            /* filelist id                   */
{
  char  buf[MAXRCD];                    /* I/O buffer                    */
  FILE *pfi;                            /* input file pointer            */

  if (ls->incl_fspec != NULL) {         /* filespec present              */
    if ((pfi = fopen(ls->incl_fspec,"r")) != NULL) {
      while (fgets(buf, MAXRCD, pfi) != NULL) { /* all input records     */
        if (ls->id != P_FIL)            /* all but FILES.BBS             */
          fputs(buf, pfo);              /* "asis"                        */
        else                            /* for FILES.BBS output          */
          fprintf(pfo,"%s %s", FILPREFX, buf);  /* special prefix        */
        }
      fclose(pfi);
      }
    else {
      if (oper_mode == VERBOSE)         /* report only in verbose mode   */
        printf(MSG_TRL,ls->incl_fspec);  /* file not included            */
      }
    }
  }

/* --------------------------------------------------------------- */
/* Produce file description parts for first and continuation lines */
/* --------------------------------------------------------------- */
void desc_part(FILE *pf,                /* output file pointer           */
               char *desc,              /* complete description          */
               unsigned int l1,         /* length 1st part of descr.     */
               unsigned int l2,         /* length of subsequent parts    */
               LISTPARM *ls)            /* report structure index        */
{
  unsigned int k,n;                     /* length of part of string      */
  char *p;                              /* pointer to output string      */

  if ((k=strsubw(desc, &p, l1)) != 0) { /* length (max = l1)             */
    if (ls->wrapflag != WRAP) {      /* truncate                      */
      n = strlen(desc);                 /* total string length           */
      k = (l1 > n) ? n : l1;            /* shortest of l1 and n          */
      }
    fprintf(pf,"%-.*s\n", k, p);        /* (1st part of) string          */
    if (ls->wrapflag==WRAP) {           /* wrapping requested            */
      while ((k=strsubw(p+k, &p, l2)) > 0) {  /* still descriptions parts*/
        fprintf(pf,"%*s%-.*s\n", 79-l2,"", k, p); /* 2nd+ descr parts  */
        }
      }
    }
   else                                 /* no descr: should not occur */
     fprintf(pf,"\n");                  /* at least new line */
  }

/* ============================================================== */
/* routine to select a substring while skipping leading blanks    */
/*         and ending within the boundaries on a word-end.        */
/*         Truncate if single word. Respect NL as end of string.  */
/*         Return strlen,  0 for NULL-pointer.                    */
/* ============================================================== */
int strsubw(char *a,
            char **b,
            unsigned int m)
{
  int i,j,k;                            /* counters                      */

  if (a==NULL)                          /* NULL pointer                  */
    return(0);                          /* no string                     */

  for (i=0; a[i] == ' '; ++i);          /* skip leading blanks           */
  a = *b = a+i;                         /* offset to first non-blank char*/
  for (i=0; a[i] != '\0' && a[i]!='\r' && a[i]!='\n'; ++i); /* search end*/
  if (i==0)                             /* nothing left                  */
    return(0);                          /* end!                          */

  for (k=0; k<m && k<i; ++k);           /* maximum substring             */
  if (k<i) {                            /* there is more in string       */
    if (a[k]==' ' || a[k]=='\r' || a[k]=='\n' || a[k]=='\0');  /*word end*/
    else {
      for (j=k-1; j>0 && a[j]!=' '; --j); /* try to remove 'split' word  */
      if (j>0)                          /* any space found?              */
        k = j;                          /* OK, else split!               */
      }
    }
  for (; k>0 && a[k-1] == ' '; --k);    /* remove trailing blanks        */
  return(k);                            /* return length of substring    */
                                        /* b contains start-point        */
  }

/* ====================================================== */
/* Function to locate next non-blank character in buffer. */
/* Returns pointer to word or NULL-ptr if no next word.   */
/* ====================================================== */
char *next_word(char *line)
{
  unsigned int i;

  for (i=0; line[i]!=' '  &&            /* skip non-blanks               */
            line[i]!='\r' &&
            line[i]!='\n' &&
            line[i]!='\0'; ++i);
  for (   ; line[i]==' '; ++i);         /* skip blanks                   */
  if (line[i] != '\0' &&
      line[i] != '\r' &&
      line[i] != '\n')
    return(line+i);                     /* next word found               */
  else
    return(NULL);                       /* NULL if no next word          */
  }

/* =========================================== */
/* Function to make ASCIIZ string of ONE word. */
/* =========================================== */
char *asciiz(char *buf)
{
  unsigned short int i;

  for (i=0; buf[i] != ' '  &&           /* end copy at first blank       */
            buf[i] !='\r'  &&           /* CR character                  */
            buf[i] !='\n'  &&           /* LF character                  */
            buf[i] !='\0'; ++i)         /* or end of string              */
    buf2[i] = buf[i];                   /* copy                          */
  buf2[i] = '\0';                       /* end of string                 */
  return(buf2);                         /* pointer to ASCIIZ string      */
  }

/* ============================================ */
/* Function to strip off AVATAR codes in string */
/* ============================================ */
char *strava(char *buf)
{
  unsigned short int i, j, k, x;        /* counters                      */

  k = strlen(buf);                      /* length of source string       */
  for (i=j=0; i < k; ) {                /* whole input string            */
    switch(buf[i]) {
      case AVA_G:
      case AVA_H:
      case AVA_I:
      case AVA_L: ++i; break;
      case AVA_Y: for (x=0; x<buf[i+2]; x++)
                    buf2[j++] = buf[i+1];      /* repeat                 */
                  i += 3;
                  break;
      case AVA_V: switch(buf[i+1]) {
                    case AVA_B:
                    case AVA_C:
                    case AVA_D:
                    case AVA_E:
                    case AVA_F:
                    case AVA_G:
                    case AVA_I:
                    case AVA_N: i += 2; break;
                    case AVA_A: i += 3; break;
                    case AVA_H: i += 4; break;
                    case AVA_L: i += 5; break;
                    case AVA_M: i += 6; break;
                    case AVA_J:
                    case AVA_K: i += 7; break;
                    case AVA_Y: i += 2; break;
                    default: buf2[j++]=buf[i++]; break;
                    }
                  break;
/* (5.8)  case '-':   (i==0) ? i++ : (buf2[j++] = buf[i++]);  */
/* (5.8)               break;                                 */
      default:    buf2[j++] = buf[i++];
                  break;
      }
    }
  buf2[j] = '\0';                       /* end of string                 */
  return(buf2);                         /* pointer to ASCIIZ string      */
  }

/* ========================================================= */
/* creates filecount and bytecount per area within privilege */
/*   and pointer to most recent file in area                 */
/* returns total filecount all area's within priv.           */
/* should be run before calling other count_functions below. */
/* ========================================================= */
ULONG  preproc_area(DOWNPATH  _HUGE *area,
                    FILECHAIN * _HUGE *dm,
                    LISTPARM  *ls)
{
  USHORT   i;                           /* area count */
  ULONG    j;                           /* file count */
  ULONG    fpc;                         /* total filecount within priv */
  FILECHAIN *fx;                        /* local pointer to file info */
  DOWNPATH  *ax;                        /* local pointer to area info */

  for (i=0; i<area_total_count; i++) {  /* all area's in array */
    ax = &area[i];                      /* pointer to area info */
    ax->file_count = 0;                 /* init filecount per area */
    ax->byte_count = 0L;                /* init bytecount per area */
    ax->newest = NULL;                  /* null most-recent file */
    }
  for (j=0; j<file_total_count; j++) {  /* scan file-chain */
    fx = dm[j];                         /* copy pointer */
    if (rpt_coll(fx, ls, FALSE)  &&     /* ignore file age */
        fx->cmt == 0) {                 /* not a comment entry */
      ax = fx->parea;                   /* copy pointer */
      ax->file_count++;                 /* increment area filecount */
      ax->byte_count += fx->size;       /* add filesize */
      ax->newest = new_acq(fx, ax->newest);  /* keep newest file */
      }
    }
  fpc = 0;                              /* init total filecount */
  for (i=0; i<area_total_count; i++) {  /* all area's in array */
    ax = &area[i];                      /* copy pointer */
    fpc += ax->file_count;              /* sum of area file_counts */
    }
  return(fpc);                          /* return total file_count */
  }

/* =========================================== */
/* count areas within privilege for top-header */
/* works only correctly after count_priv_files */
/* =========================================== */
USHORT count_areas(DOWNPATH _HUGE *area,
                          short int p)
{
  USHORT i,j;

  for (i=j=0; i<area_total_count; i++) {  /* whole area-array            */
    if (area[i].priv <= p       &&      /* area within privilege         */
        area[i].file_count > 0)         /* any files                     */
      ++j;                              /* add to 'active' area_count    */
    }
  return(j);                            /* return areas within priv.     */
  }

/* ================================================ */
/* (re-)Count files within privilege for top-header */
/*   (file_priv_count already returns file-count)   */
/* Works only correctly after preproc_area().       */
/* ================================================ */
ULONG  count_files(DOWNPATH _HUGE *area)
{
  ULONG i,f;

  for (i=f=0; i<area_total_count; i++)  /* scan area array */
    f += area[i].file_count;            /* add to file_count */
  return(f);                            /* return files within priv. */
  }

/* =========================================== */
/* count bytes within privilege for top-header */
/* works only correctly after count_priv_files */
/* =========================================== */
ULONG  count_bytes(DOWNPATH _HUGE *area)
{
  USHORT i;                             /* counter                       */
  ULONG  b;                             /* byte counter                  */

  for (i=0,b=0; i<area_total_count; i++)  /* scan area array            */
    b += area[i].byte_count;            /* add to byte_count             */
  return(b);                            /* return bytes within priv.     */
  }

/* ==================================== */
/* insert title lines from DOWNSORT.CFG */
/* ==================================== */
void insert_title(FILE *pf,
                  STRCHAIN *title,
                  int  ipf)             /* if not zero: call stripf() */
{
  if (title != NULL) {                  /* any title lines present */
    do {
      fprintf(pf,"%s\n", (ipf) ? stripf(title->str) : title->str);
      title = title->next;
      } while (title != NULL);
    }
  }

/* ============================================================ */
/* insert separator line, variable number, separated by 1 blank */
/* ============================================================ */
void sep_line(FILE *pf,                 /* output file pointer           */
              char c,                   /* separator char                */
              unsigned short int size,...)  /* length(s)                 */
{
  char buf[80];                         /* work buffer                   */
  unsigned short int k,ll;              /* offset and size               */
  va_list mk;                           /* argument marker               */

  memset(buf, c, 79);                   /* whole line buffer             */
  buf[79] = '\0';                       /* end of string                 */
  va_start(mk, size);                   /* start variable arg processing */
  k = size;                             /* take first                    */
  while (k < 80) {                      /* all parts of line             */
    buf[k++] = ' ';                     /* replace sepline char by space */
    ll = va_arg(mk, unsigned short int);  /* next argument               */
    if (ll > 0)                         /* more in list                  */
      k += ll;                          /* add to offset                 */
    else {
      buf[k] = '\0';                    /* end of string                 */
      break;                            /* escape from while-loop        */
      }
    }
  fprintf(pf, "%-s\n", buf);            /* output                        */
  va_end(mk);                           /* end variable argument list    */
  }

/* ================================== */
/* some marketing below every report! */
/* ================================== */
void signature(FILE *pf,
               char *now)
{
  static char *mode[] = {"DOS","OS/2"};

  fprintf(pf,"\n\n  ");
  sep_line(pf, 'Í', 73, 0);
  fprintf(pf,"   This list was created with %s %c.%c%c (%s-bits)  -  by %s\n"
             "                  on %s under %s %d.%d\n  ",
                PROGNAME,VERSION,SUBVERS,SUFFIX,
#ifndef __32BIT__
                "16",
#else
                "32",
#endif
                AUTHOR, now,
                mode[_osmode & 1],
                _osmajor/( (_osmode) ? 10 : 1),
                _osminor/10);
  sep_line(pf, 'Í', 73, 0);
  fprintf(pf, "\n");                    /* extra space                   */
  }

/* ================ */
/* HELP information */
/* ================ */
void show_help(void)
{
  static char *help[] = {
   "Syntax:  [drive:][path]DOWNSORT  [@filespec]  [/H | /Q | /V]\n\n",
   "@filespec   - '@' followed by filespec of Downsort's parameter file\n",
   "/H          - Display this HELP-screen\n",
   "/Q          - Suppress all progress messages\n",
   "/V          - Display extensive progress messages\n",
   "ÄÄÄÄÄÄÄÄÄÄÄ\n",
   "Read documentation and sample configuration file "
     "for details and defaults.\n",
   NULL};

  int i;
  for (i=0; help[i]; ++i)
    printf(help[i]);
  exit(1);
  }

