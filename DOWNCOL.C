
/* ============================================================ */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility. */
/*  -> DOWNCOL.C                                                 */
/*  -> Functions to collect download file information.           */
/* ============================================================= */

/* #define __DEBUG__  */

#define INCL_DOS
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>

#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys\types.h>

#include "..\max\mstruct.h"             /* MAXIMUS definitions          */
#include "downsort.h"                   /* downsort defines             */
#include "downfpro.h"                   /* downsort function prototypes */


/* prototypes of local functions */

FILECHAIN *add_comment(char *, DOWNPATH *,
                       char *, short int, USHORT);
void       add_pathspec(char *, FILECHAIN *);
void       add_to_chain(FILECHAIN **, FILECHAIN *);
int        area_selection(char *);
FILECHAIN *assign_desc(long int, char *, FILECHAIN **,
           DOWNPATH _HUGE *, char *, short int, USHORT);
char      *build_desc(FILE *, char *, char **);
long int   collect_area(DOWNPATH _HUGE **);
long int   collect_file(USHORT, DOWNPATH _HUGE *);
long int   combine_comments(void);
long int   count_orphans(FILECHAIN *);
FILECHAIN *desc_dup(FILECHAIN *, char *,
               DOWNPATH _HUGE *, short int, USHORT, USHORT, USHORT);
int        file_selection(char *);
long int   fill_chn(DOWNPATH *, FILECHAIN **);
#ifndef __32BIT__
  FILECHAIN *file_element(DOWNPATH *, FILEFINDBUF *);
#else
  FILECHAIN *file_element(DOWNPATH *, FILEFINDBUF3 *);
#endif
long int   free_orphan(FILECHAIN **);
long int   get_desc(DOWNPATH *, FILECHAIN **);
char      *parse_fname(char *);
int        sort_fcol(const void *, const void *);
int        sort_path(const void *, const void *);
unsigned int  split_fname(char *);

#ifdef __DEBUG__
  USHORT  dump_area_array(DOWNPATH _HUGE *, ULONG, char *);
  ULONG   dump_file_chain(FILECHAIN *, char *);
#endif

/* -------------------------------------- */
/* Collect download file area information */
/* returns - number of area's             */
/*         - pointer to (huge) array      */
/* -------------------------------------- */
long int collect_area(DOWNPATH _HUGE **area)
{
#ifndef __32BIT__
  FILESTATUS  fs;                       /* file status buffer */
  USHORT  af;                           /* file handle */
  USHORT  oaction;                      /* open_action */
#else
  FILESTATUS3 fs;                       /* file status buffer */
  ULONG   af;                           /* file handle */
  ULONG   oaction;                      /* open_action */
#endif
  int     rc;                           /* returncode Dos-calls */
  long int i,j,k;                       /* signed counter(s) */
  ULONG   m;                            /* unsigned counter(s) */
  struct _area a;                       /* MAXIMUS (minimum) area struct.*/
  DOWNPATH _HUGE *d;                    /* ptr to array with area-info   */

#ifdef __DEBUG__
  remove("downsort.log");               /* remove previous debug file */
#endif
  af = 0;                               /*
  oaction = 0;                          /* init */
  rc = DosOpen(areadat_path,            /* open 'carefully' */
          &af,                          /* pointer to File Handle */
          &oaction,                     /* pointer to action field */
          0L,                           /* new filesize (N/A) */
          FILE_NORMAL,                  /* attributes */
          FILE_OPEN,                    /* open flags */
          OPEN_ACCESS_READONLY    +     /* mode */
            OPEN_SHARE_DENYWRITE,
          0L);                          /* no EA's */
  if (rc != NO_ERROR) {                 /* open error */
    fprintf(stderr, MSG_OPI, areadat_path, rc);
    exit(rc);
    }
  if (oaction != FILE_EXISTED) {        /* not found */
    fprintf(stderr, MSG_OPA, areadat_path, oaction);
    exit(5);
    }
  else {
    if (oper_mode != QUIET)
      fprintf(stdout, MSG_COA, areadat_path);
#ifndef __32BIT__
    rc = DosQFileInfo(af, 1, &fs, sizeof(fs));     /* info AREA.DAT */
#else
    rc = DosQueryFileInfo(af, 1, &fs, sizeof(fs)); /* info AREA.DAT */
#endif
    read(af, (char *)&a, sizeof(struct _area));  /* obtain first part */
    m = 0;                              /* init participating file count */
    if (a.id != AREA_id) {              /* validate MAXIMUS format */
      fprintf(stderr, MSG_MX1, areadat_path);
      fprintf(stderr, MSG_MX2, PROGNAME,VERSION,SUBVERS,SUFFIX,MAX);
      }
    else {                              /* acceptable AREA.DAT */
      k = (ushort)(fs.cbFile / a.struct_len);     /* # area's */
      for (i=0; i<k; i++) {             /* count area's with downloads */
        lseek(af, i*(long)a.struct_len, SEEK_SET);  /* locate to next */
        read(af, (char *)&a, sizeof(struct _area));
        if (a.filepath[0] != '\0'       &&
            a.filepath[0] != ' '        &&
            a.filepriv <= ABS_MAX_priv  &&   /* within report privilege */
            (areakeys ^ (a.filelock | areakeys)) == 0  &&   /* all keys */
            area_selection(a.name))     /* check include and exclude */
          m++;                          /* participating filearea count */
        }
      if (m > 0) {                      /* any area's participating */
#ifndef __32BIT__
        d = (DOWNPATH huge *)halloc(m, sizeof(DOWNPATH));
#else
        d = (DOWNPATH *)malloc(m * sizeof(DOWNPATH));
#endif
        if (d == NULL) {                /* memory not obtained? */
          fprintf(stderr, MSG_MEM, PROGNAME);
          exit(6);
          }
        else {                          /* have memory for area-info */
          *area = d;                    /* return pointer to caller */
          for (i=0, m=0; i<k; i++) {    /* all area's */
            lseek(af, i*(long)a.struct_len, SEEK_SET);  /* locate to next*/
            read(af, (char *)&a, sizeof(struct _area));
            if ( a.filepath[0] != '\0'      &&  /* filename(?) */
                 a.filepath[0] != ' '       &&  /* filename(?) */
                 a.filepriv <= ABS_MAX_priv &&  /* within report privileg*/
                (areakeys ^ (a.filelock | areakeys)) == 0  &&    /* keys */
                 area_selection(a.name)) {      /* check selection */
              d[m].priv = a.filepriv;           /* download privilege */
              d[m].anum = (short)i;             /* AREA.DAT seq. nbr     */
              d[m].filelock = a.filelock;       /* File Lock             */
              d[m].file_count = 0;              /* init file count       */
              d[m].byte_count = 0L;             /* init byte count       */
              d[m].newest     = NULL;           /* pointer to newest file*/
              strcpy(d[m].name, a.name);        /* area name ... edit!   */
              strcpy(d[m].pname, a.filepath);   /* download path         */
              j = strlen(d[m].pname);
              if (j>0 && d[m].pname[j-1] != '\\')
                d[m].pname[j] = '\\';           /* backslash if needed   */
              strcpy(d[m].filesbbs, a.filesbbs); /* files.bbs */
              strcpy(d[m].adesc, a.fileinfo);   /* filearea title */
              ++m;                              /* filearea index */
              }
            }
          }
                            /* prepare here area-names for group sorting */
        max_aname = 0;                  /* init max areaname length */
        for (i=0; i<m; i++)             /* all collected area's */
          max_aname = (unsigned short)max(strlen(d[i].name), max_aname);
        for (i=0; i<m; i++) {           /* all collected area's */
          sprintf(d[i].ename,
                 (d[i].name[0]<'0' || d[i].name[0]>'9')  /* not numeric */
                    ? "%-*.*s" : "%*.*s",  /* left or right aligned */
                  max_aname, max_aname, d[i].name);
          d[i].ename[max_aname] = '\0'; /* end of string */
          j = k = max_aname-1;          /* init offset values */
          while (j>0 && d[i].ename[j] == ' ')   /* search last char */
            j--;
          if (j < k) {                  /* spaces found */
            while (j>0                &&
                   d[i].ename[j]>='0' &&
                   d[i].ename[j]<='9') {
              d[i].ename[k--] = d[i].ename[j];  /* move */
              d[i].ename[j--] = ' ';            /* replace by blank */
              }
            }
          }
        }
      }
#if defined(__DEBUG__)
      dump_area_array(*area, m, "after area.dat processing");
#endif
    DosClose(af);                       /* close area.dat-file */
    fflush(stdout);                     /* secure information messages */
    fflush(stderr);                     /* secure error messages */
    return((int) m);                    /* report number downloadarea's */
    }
  return(0);
  }

/* ---------------------------- */
/* Collect all file information */
/* ---------------------------- */
long int collect_file(USHORT   a,       /* number entries in area-array */
                      DOWNPATH _HUGE *area)  /* pointer to area array */
{
  long int  i, k, l;                    /* counter(s) */
  long int  file_count;                 /* total file count */
  long int  orpcnt;                     /* total orphan count */
  FILECHAIN *ca,                        /* ptr to first of path */
            *ce;                        /* ptr to file info */

  file_count = 0;                       /* init */
  orpcnt = 0;                           /* init */
  if (oper_mode != QUIET)
    fprintf(stdout, MSG_COF);
  qsort(area, a, sizeof(DOWNPATH), sort_path);
                         /* WARNING: do no sort this array ever again! */
  ca = NULL;                            /* init: no group yet */
  for (i=0; i<a; i++) {                 /* all 'included' download areas */
    area[i].file_count = k = 0;         /* init filecount this area */
    if (oper_mode == VERBOSE)
      fprintf(stdout, MSG_ARE, k, area[i].pname, area[i].name);
    else if (oper_mode != QUIET)
      fprintf(stdout, DOT);
    fflush(stdout);                     /* show output */
    if (i<1 || stricmp(area[i].pname, area[i-1].pname)) {  /* pathnames */
      if (lp[P_ORP].next==NULL)         /* orphan report not req'd */
        orpcnt += free_orphan(&first_element); /* free rest-orphans */
      k = fill_chn(&area[i], &ce);      /* read directory */
                                        /* returns # of files and */
                                        /*   pointer to first of group */
      ca = NULL;                        /* indicate new group */
      add_to_chain(&ca, ce);            /* add group(!) to chain */
      }
    else                                /* new area, same DL-dir */
      k = 0;                            /* restart directory file count */
    if (oper_mode==VERBOSE)
      fprintf(stdout, "%6ld\n", k);     /* total files this DL-dir */
    l = get_desc(&area[i], &ca);        /* obtain descriptions */
    if (oper_mode==VERBOSE && l>0)      /* offline files added to chain */
      fprintf(stdout, MSG_ARD, k+l, l, area[i].name);
    file_count += k + l;                /* add to # of input entries */
    }                                   /* all area's processed */

  if (lp[P_ORP].next==NULL)             /* orphan report not req'd */
    orpcnt += free_orphan(&first_element);  /* free orphans */
  if (oper_mode==VERBOSE && orpcnt>0)
    fprintf(stdout, MSG_ORP, orpcnt);
  else if (oper_mode!=QUIET)
    fprintf(stdout,"\n");               /* newline..flush */

  file_count -= orpcnt;                 /* less dropped orphans */

  if (lp[P_FIL].next != NULL) {         /* FILES.BBS report requested */
    ce = first_element;                 /* head of chain */
    while (ce != NULL) {                /* whole chain */
      ce->parea->file_count++;          /* bump up area filecount */
      ce = ce->next_element;            /* ptr to next element */
      }
    ca = NULL;                          /* new 'group' */
    for (i=0; i<a; i++) {               /* all area's in array */
      if (area[i].file_count == 0) {    /* no files in this area */
        ce = add_comment(EMPTY, &area[i],     /* dummy filename */
                         " ", TWIT, 0xFFFF);  /* 'dummy' decription */
        add_to_chain(&ca, ce);          /* add element to chain */
        file_count++;                   /* update total count */
        if (oper_mode==VERBOSE)
          fprintf(stdout, MSG_ARC, area[i].name);  /* added placeholder */
        }
      }
    }

  if (file_count) {                     /* file(s) present */
    if ((ca = first_element) != NULL) { /* pointer to first element */
      for (k=1; ca->next_element != NULL; k++)   /* at least 1, find last */
        ca = ca->next_element;          /* pointer to next */
      }
    else
      k=0;                              /* no files in chain */
    if (file_count != k) {              /* compare counts */
      DosBeep(880,50);
      DosBeep(440,50);                  /* warning signal */
      DosBeep(880,50);
      fprintf(stderr, MSG_IEC, PROGNAME, file_count, k);  /* internal error */
#if defined(__DEBUG__)
      dump_file_chain(first_element, "file_count / chain discrepancy");
#endif
      exit(99);                         /* quit */
      }
    }
  return(file_count);                   /* file total count */
  }

/* ---------------------------------------- */
/* Add all subdir-filenames to the chain,   */
/* for further processing by mainline.      */
/* NOTE: - full path name assumed!          */
/* returns - number of files in this area   */
/*         - pointer to first element       */
/* ---------------------------------------- */
long int fill_chn(DOWNPATH *parea,      /* pointer to area array */
                  FILECHAIN **cp)       /* return-ptr to first element */
{
  int      rc;                          /* returncode of DOS-calls */
  long int fc;                          /* area-file counter */
  FILECHAIN  *ce, *tp;                  /* curr, temp chain ptrs */
  char   down_spec[MAXPATH];            /* file specification buffer */
  HDIR   fhandle;                       /* FindFirst/Last handle */

#ifndef __32BIT__
#define  MAXENT 1                       /* must be 1 for DOS */
  FILEFINDBUF buf;                      /* file-info buffer (1 entry!) */
  FILEFINDBUF *cf;                      /* pointer to current entry */
  USHORT fentries;                      /* # entries to be retrieved */
#else
#define  MAXENT 32                      /* dir-entries per DosFind */
  int    i;                             /* file-entry counter */
  char   buf[MAXENT * sizeof(FILEFINDBUF3)];    /* file-info buffer */
  FILEFINDBUF3  *cf;                    /* pointer to current entry */
  ULONG  fentries;                      /* # entries to be retrieved */
#endif
  char  *pBuf;                          /* buffer pointer */

  fc = 0;                               /* init filecount this dir */
  *cp = NULL;                           /* indicate in group yet */
  strcpy(down_spec,parea->pname);       /* path */
  strcat(down_spec, "*.*");             /* file-spec */
  fentries = MAXENT;                    /* # to retrieve at a time */
  fhandle  = HDIR_CREATE;               /* FindFirst/Last handle */
  rc = DosFindFirst(down_spec,
                    &fhandle,
                    FILE_NORMAL,
                    &buf,
                    sizeof(buf),
                    &fentries,
#ifndef __32BIT__
                    0L);                /* unused with OS/2 < 2.0 */
#else
                    FILE_ARCHIVED || FILE_READONLY); /* OS/2 2.0+ */
#endif
  while (rc == 0) {                     /* until no more files in dir */
    pBuf = (char *)&buf;                /* init pointer to buffer */
#ifndef __32BIT__
    cf = (FILEFINDBUF *)pBuf;           /* pointer to first (only) entry */
#else
    cf = (FILEFINDBUF3 *)pBuf;          /* pointer to first (only) entry */
    for (i=1; i<=fentries; i++) {       /* break after last entry! */
#endif
      if (file_selection(cf->achName)) {  /* not an excluded file */
        tp = file_element(parea, cf);   /* add new element to chain */
                                        /* note: exits if no memory */
        if (*cp == NULL)                /* it is first element in group */
          *cp = ce = tp;                /* return ptr, 1st and current */
        else {                          /* not first */
          ce->next_element = tp;        /* chain-ptr in previous */
          ce = tp;                      /* new last in group */
          }
        ++fc;                           /* update filecount this area */
        byte_count += cf->cbFile;       /* update file bytecount */

        if (oper_mode==VERBOSE && (fc%25)==0) {  /* every 25 files */
          fprintf(stdout, "%6lu\r", fc);
          fflush(stdout);
          }
        }
#ifndef __32BIT__
                                        /* nothing for OS/2 < 2.0 */
#else
      pBuf += cf->oNextEntryOffset;     /* set pointer to next entry */
      cf = (FILEFINDBUF3 *)pBuf;        /* casted pointer */
      }
#endif
    fentries = MAXENT;                  /* # to retrieve at a time */
    rc = DosFindNext(fhandle,
                     &buf,
                     sizeof(buf),
                     &fentries);
    }
  DosFindClose(fhandle);                /* close directory association */
  return(fc);                           /* # of directory entries */
  }

/* --------------------------------------------------- */
/* Add file description to chain elements              */
/* returns - number of additional entries in FILES.BBS */
/*         - pointer to first file info structure      */
/* --------------------------------------------------- */
long int get_desc(DOWNPATH  *parea,     /* pointer to info of THIS area */
                 FILECHAIN **cp)        /* return pointer 1st element */
{
#ifndef __32BIT__
  USHORT  oaction;                      /* open action flags */
  USHORT  fih;                          /* file handle */
#else
  ULONG   oaction;                      /* open action flags */
  ULONG   fih;                          /* file handle */
#endif
  FILE   *fi;                           /* file pointer */
  long int x, y;                        /* number of offline files */
  FILECHAIN *ce, *tp;                   /* ptrs to file-info */
  char   buf[MAXRCD];                   /* read-buffer for FILES.BBS */
  char   filename[MAXFN];               /* filename from FILES.BBS buf */
  char   *desc, *rp;                    /* ptr to desc / return string */
  int    i;                             /* counter(s) */
  short  int  k;                        /* counter(s) */
  int    m;                             /* counter(s) */
  short int fpriv;                      /* file privilege */
  ULONG  rc;                            /* returncode */
  int    fn_off;                        /* filename offset */
  USHORT bbsseq;                        /* current line number FILES.BBS */
  char   desc_spec[MAXPATH];            /* explicit pathspec FILES.BBS */
  char   fpathspec[MAXPATH];            /* pathspec of file in FILES.BBS */
  FILECHAIN **fa;                       /* pointer to file sort-array */

  if (strlen(parea->filesbbs) > 0)      /* explicit specification? */
    strcpy(desc_spec,parea->filesbbs);  /* explicitly specified FileList */
  else {                                /* no */
    strcpy(desc_spec,parea->pname);     /* get path to download directory*/
    strcat(desc_spec,lp[P_FIL].name);   /* add filename */
    strcat(desc_spec,".");              /* add separator */
    strcat(desc_spec,lp[P_FIL].ext);    /* add extension */
    }

  fpriv = parea->priv;                  /* area-priv is default file priv*/
                                        /* FILES.BBS may be in use!!!    */
  fih = 0;                              /* file handle */
  rc = DosOpen(desc_spec,               /* Open 'carefully' */
          &fih,                         /* pointer to File Handle */
          &oaction,                     /* pointer to action field */
          0L,                           /* new filesize (N/A) */
          FILE_NORMAL,                  /* attributes */
          FILE_OPEN,                    /* open flags */
          OPEN_ACCESS_READONLY    +     /* mode */
            OPEN_SHARE_DENYWRITE,
          0L);                          /* no EA's OS/2 2.0+ */
  if (rc) {                             /* open error */
    fprintf(stderr, MSG_OPI, desc_spec, rc);
    return(0);                          /* no: return to caller */
    }
  if ((fi = fdopen(fih, "r")) == NULL) {
    fprintf(stderr, MSG_OPA, desc_spec, -1);   /* dummy rc */
    return(0);                          /* no: return to caller */
    }

  ce = *cp;                             /* ptr to 1st element of group */
  for (x=0; ce != NULL; x++)            /* count collected dir entries */
    ce = ce->next_element;              /* pointer to next file entry */

  if (x > 0) {                          /* any files in directory */
    fa = (FILECHAIN **)malloc((unsigned int)x * sizeof(FILECHAIN *));
    if (fa == NULL) {                   /* not enough memory */
      fprintf(stderr, MSG_MEM, PROGNAME);
      exit(11);
      }
    ce = *cp;                           /* ptr to 1st element of group */
    for (i=0; ce != NULL; i++) {        /* init sort array */
      fa[i] = ce;                       /* element pointer into array */
      ce = ce->next_element;            /* pointer to next file entry */
      }
    qsort(fa, (unsigned int)x, sizeof(FILECHAIN *), sort_fcol);
    }
  else                                  /* no files in group */
    fa = NULL;                          /* no array allocated */

  y = 0;                                /* init 'offline' counter */
  bbsseq = 0;                           /* init counter */
  rp = fgets(buf, MAXRCD, fi);          /* get first FILES.BBS record */
  while (rp != NULL) {                  /* until end of FILES.BBS */
    bbsseq++;                           /* line counter */
    m = strlen(buf);                    /* length of input string */
    if (m>1 && buf[0] == '\20') {       /* privilege change (^P) ? */
      for (k=0; k<HIDDEN-TWIT && priv_name[k][0]!=buf[1]; k++)
        ;                               /* just scan */
      if (TWIT + k > parea->priv)       /* only if higher than area-priv */
        fpriv = k + TWIT;               /* new (higher) prilivege */
      rp = fgets(buf, MAXRCD, fi);      /* continue */
      }
    else if (parse_fname(buf) == NULL) { /* NOT likely a filename */
      if ( strncmp(buf, FILPREFX, 3)==0  ||  /* 3 chars only: bwd compat.*/
           bbsseq <= 8 ) {              /* first 8 lines */
        }                               /* just skip these */
      else if (keepseq == KEEPSEQ) {    /* comments to be collected */
        tp = add_comment(EMPTY, parea,     /* filename = null-string */
                     buf, fpriv, bbsseq);
        add_to_chain(cp, tp);           /* add element to end-of-chain */
        ++y;                            /* add 1 to 'offline' filecount */
        }
      rp = fgets(buf, MAXRCD, fi);      /* continue with next line */
      }
    else {                              /* probably file descr record */
      bbsseq = max(bbsseq, 8);          /* forced acceptance of comments */
      for (i=0;                         /* starting point of scan */
                i < m               &&  /* string length */
                buf[i] != ' '       &&  /* space character */
                buf[i] != '\r'      &&  /* CR character */
                buf[i] != '\n';         /* LF character */
                   i++)
          ;                             /* scan for filespec */
      strncpy(fpathspec, buf, i);       /* copy full filespec */
      fpathspec[i] = '\0';              /* end of string */
      strupr(fpathspec);                /* in capitals */
      fn_off = split_fname(fpathspec);  /* split path */
      strncpy(filename, fpathspec+fn_off, MAXFN);   /* copy filename */
      filename[MAXFN-1] = '\0';         /* ensure end of string */
      strupr(filename);                 /* filename in capitals */
      if (file_selection(filename)) {   /* not excluded filename */
        desc = build_desc(fi, buf, &rp);  /* build description string */
        tp = assign_desc(x, filename,   /* assign desc to file(s) */
                         fa, parea, desc, fpriv, bbsseq);
        if (tp != NULL) {               /* new element for group */
          add_to_chain(cp, tp);         /* add element to end-of-chain */
          if (fn_off > 0)               /* pathspec -> extra file */
            add_pathspec(fpathspec, tp);  /* add filesize/date, etc */
          ++y;                          /* add 1 to 'extra' filecount */
          }
        }
      else
        rp = fgets(buf, MAXRCD, fi);    /* continue with next line */
      }
    }
  if (fa != NULL)                       /* array memory was allocated */
    free(fa);                           /* free temp sort array */
  x = combine_comments();               /* combine consecutive comments */
  fclose(fi);                           /* finished with this FILES.BBS */
  DosClose(fih);                        /* close the handle */
  return(y-x);                          /* return 'offline' filecount */
                                        /* (includes comment elements) */
  }

/* ---------------------------------- */
/* Add a file element to end-of-chain */
/* ---------------------------------- */
void add_to_chain(FILECHAIN **cp,       /* ptr to ptr to a chain element */
                  FILECHAIN *tp)        /* pointer to new element */
{
  FILECHAIN *ce;                        /* work pointer */

  if (tp == NULL)                       /* trying to add null element */
    return;                             /* nothing to do here! */
  if (*cp == NULL) {                    /* first / empty group (?) */
    *cp = tp;                           /* return ptr to first in group */
    if (first_element == NULL) {        /* whole chain empty */
      first_element = tp;               /* very first: start of chain */
      return;                           /* ===> nothing else to do here! */
      }
    else                                /* something in chain */
      ce = first_element;               /* ptr to very 1st element */
    }
  else
    ce = *cp;                           /* start search for last */

  while (ce->next_element != NULL)      /* search last element */
    ce = ce->next_element;
  ce->next_element = tp;                /* add new element to chain */
  }

/* ------------------------------------- */
/* Build file description memory block   */
/* returns pointer to description string */
/* ------------------------------------- */
char *build_desc(FILE  *fi,             /* opened FILES.BBS */
                 char  *ibuf,           /* file I/O read buffer */
                 char  **rp)            /* pointer to next input buffer */
                                        /*     or NULL if no more input */
{
  char   *fd;                           /* pointer memory block with desc*/
  char   *desc;                         /* string work pointers */
  char   buftmp[MAXDESC];               /* work buffer */
  ULONG  k;                             /* counter */

  desc = next_word(ibuf);               /* locate description */
  if (desc != NULL) {                   /* start of description found */
    for (k=0; desc[k]; ++k) {           /* search first CR/LF */
      if (desc[k]=='\r' || desc[k]=='\n') {
        desc[k] = '\0';
        break;
        }
      }
    strncpy(buftmp, desc, MAXDESC-1);   /* save desc in buftmp */
    while (((*rp = fgets(ibuf, MAXDESC, fi)) != NULL) &&  /* next rcd */
           (ibuf[0] == ' ') &&          /* probably desc continuation */
           ((desc = next_word(ibuf)) != NULL)) {
      for (k=0; desc[k]!='\0'; ++k) {
        if (desc[k]=='\r' || desc[k]=='\n') {
          desc[k] = '\0';
          break;
          }
        }
      if ((strlen(buftmp) + k + 1) <= sizeof(buftmp)) {  /* not too long */
        strcat(buftmp," ");             /* single space */
        strcat(buftmp, desc);           /* concat desc to buftmp */
        }
      }
    fd = malloc(strlen(buftmp)+1);      /* obtain memory */
    if (fd == NULL) {
      fprintf(stderr, MSG_MEM, PROGNAME);  /* not enough memory */
      exit(12);
      }
    else
      strcpy(fd, buftmp);               /* store description */
    }
  else {                                /* no description */
    *rp = fgets(ibuf, MAXDESC, fi);     /* read ahead for next file */
    fd = NULL;                          /* return value */
    }
  return(fd);                           /* return with pointer */
  }

/* ------------------------------------- */
/* Assign description to file element(s) */
/* ------------------------------------- */
FILECHAIN *assign_desc(long int x,            /* # of files in dir */
                       char   *filename,      /* filespec */
                       FILECHAIN **fa,        /* pointer to file ptr */
                                              /* array of this area only */
                       DOWNPATH _HUGE *parea, /* pointer to area */
                       char   *desc,          /* description string */
                       short  int fpriv,      /* file privilege */
                       USHORT bbsseq)         /* seq# in FILES.BBS */
                                              /* (partly for new files)  */
{
  long  int    low,high,index;          /* indexes */
  long  int    b,n;                     /* counters */
  USHORT       dlb,dlt;                 /* download flags in description */
  FILECHAIN   *tp;                      /* ptr to file-info */
  char         dl_flags[5];             /* copy of first part description*/

  dlb = dlt = 0;                        /* flags off */
  while (desc != NULL) {                /* only when real descr. present */
    strncpy(dl_flags, desc, 4);         /* copy part of description */
    if (dl_flags[0] == '/') {           /* could be valid dl_flag */
      strupr(dl_flags);                 /* make all uppercase */
      if (dl_flags[1] == 'B' || dl_flags[2] == 'B')
        dlb = 1;                        /* unlimited download bytes flag */
      if (dl_flags[1] == 'T' || dl_flags[2] == 'T')
        dlt = 1;                        /* unlimited download time flag  */
      desc = next_word(desc);           /* shift the description pointer */
      }
    else                                /* not MAXIMUS dl-flag */
      break;                            /* escape from while-loop */
    }

  low  = 0;                             /* index of first entry */
  high = x-1;                           /* index of last entry */
  while (low <= high) {                 /* binary search in array */
    index = (high + low) / 2;           /* middle of interval */
    if ((b = wild_comp(fa[index]->fname, filename)) < 0)
      low = index + 1;                  /* new low boundary  */
    else if (b > 0)                     /* 'b'-value from previous 'if'  */
      high = index - 1;                 /* new high boundary */
    else
      break;                            /* equal: found */
    }

  tp = NULL;                            /* init to 'not offline' */
  if (x < 1  ||                         /* no files in directory at all  */
      wild_comp(fa[index]->fname,filename)) {  /* this one not in dir */
    if (oper_mode == VERBOSE)
      fprintf(stdout," \t%s  %s\n", OFFLINE, filename); /* filename */
    tp = file_element(parea, NULL);     /* alloc new element */
    strcpy(tp->fname, filename);        /* filename */
    tp->fdesc = (desc != NULL) ? desc : NDS; /* assign 'some' desc */
    tp->priv = fpriv;                   /* copy privilege */
    tp->fseq = bbsseq;                  /* copy FILES.BBS line number */
    tp->dl_b = dlb;                     /* copy download bytes flag */
    tp->dl_t = dlt;                     /* copy download time flag */
    }
  else {                                /* file found in directory */
    n = index;                          /* 'hit' and lower entries */
    while (n>=0 && wild_comp(fa[n]->fname,filename) == 0) {
      if (fa[n]->fdesc == NULL) {       /* no description assigned yet */
        fa[n]->fdesc = (desc==NULL) ? NDS : desc;
        fa[n]->parea = parea;           /* copy / overwrite area ptr */
        fa[n]->priv  = fpriv;           /* copy privilege */
        fa[n]->dl_b  = dlb;             /* copy download bytes flag */
        fa[n]->dl_t  = dlt;             /* copy download time flag */
        fa[n]->fseq  = bbsseq;          /* copy FILES.BBS line number */
        }
      else if (strcmp(fa[n]->parea->name,parea->name))  /* diff. dir */
        tp = desc_dup(fa[n], desc, parea, fpriv, bbsseq, dlb, dlt); /*new*/
      --n;                              /* next lower */
      }
    n = index + 1;                      /* higher entries */
    while (n<x && wild_comp(fa[n]->fname,filename) == 0) {
      if (fa[n]->fdesc == NULL) {       /* no description assigned yet   */
        fa[n]->fdesc = (desc==NULL) ? NDS : desc;
        fa[n]->parea = parea;           /* copy / overwrite area ptr */
        fa[n]->priv  = fpriv;           /* copy privilege */
        fa[n]->dl_b  = dlb;             /* copy download bytes flag */
        fa[n]->dl_t  = dlt;             /* copy download time flag  */
        fa[n]->fseq  = bbsseq;          /* copy FILES.BBS line number */
        }
      else if (strcmp(fa[n]->parea->name,parea->name))  /* diff. dir */
        tp = desc_dup(fa[n], desc, parea, fpriv, bbsseq, dlb, dlt); /*new*/
      ++n;                              /* next higher */
      }
    }
  return(tp);                           /* return pointer to new(?)      */
                                        /* element or NULL if not new    */
  }

/* ---------------------------------------------------- */
/* Create duplicate file element with description       */
/* returns - pointer to new element (or NULL if no new) */
/* ---------------------------------------------------- */
FILECHAIN *desc_dup(FILECHAIN *fa,      /* pointer to file el. in array */
                    char      *desc,    /* pointer to file desription */
                    DOWNPATH _HUGE *parea,  /* pointer to area array */
                    short int fpriv,    /* file privilege */
                    USHORT    bbsseq,   /* seq# in FILES.BBS */
                    USHORT    dlb,      /* download byte flag */
                    USHORT    dlt)      /* download time flag */
{
  FILECHAIN *tp;                        /* ptr to file-info */

  tp = file_element(parea, NULL);       /* alloc new element  */
  tp->wd.date = fa->wd.date;            /* copy file wrtie date */
  tp->wt.time = fa->wt.time;            /* copy file write time */
  tp->cd.date = fa->cd.date;            /* copy file create date */
  tp->ct.time = fa->ct.time;            /* copy file create time */
  tp->size  = fa->size;                 /* copy file size */
  tp->fseq  = bbsseq;                   /* copy FILES.BBS line number */
  tp->priv  = fpriv;                    /* copy privilege */
  tp->dl_b  = dlb;                      /* copy download bytes flag */
  tp->dl_t  = dlt;                      /* copy download time flag */
  strcpy(tp->fname, fa->fname);         /* copy filename to new element  */
  tp->fdesc = (desc != NULL) ? desc : NDS;  /* add pointer to desc. */
  return(tp);                           /* return pointer */
  }

/* ---------------------------------------- */
/* Add single file-entry to file-chain      */
/* returns: pointer to new element          */
/* ---------------------------------------- */
FILECHAIN *file_element(DOWNPATH *parea,       /* ptr to info this area */
#ifndef __32BIT__
                        FILEFINDBUF *cf)       /* DOS/OS file info block */
#else
                        FILEFINDBUF3 *cf)      /* OS/2 file info block   */
#endif
{
  FILECHAIN *tp;                        /* temporary-pointer             */

  tp = (FILECHAIN *)malloc(sizeof(FILECHAIN));
  if (tp == NULL) {
     fprintf(stderr, MSG_MEM, PROGNAME);  /* not enough memory           */
     exit(11);
     }
  memset(tp, '\0', sizeof(FILECHAIN));  /* whole struct zeroes    */
  tp->next_element = NULL;              /* ptr to next                   */
  tp->fseq  = 65535;                    /* default FILES.BBS line number */
  tp->priv  = HIDDEN;                   /* unless FILES.BBS proves otherw*/
  tp->fdesc = NULL;                     /* unless found in FILES.BBS     */
  tp->fpath = NULL;                     /* explicit path in FILES.BBS    */
  tp->parea = parea;                    /* copy pointer to area info     */
  if (cf != NULL) {                     /* file-system info available    */
    tp->wd.date = cf->fdateLastWrite;   /* file date */
    tp->wt.time = cf->ftimeLastWrite;   /* file time */
    tp->cd.date = cf->fdateCreation;
    tp->ct.time = cf->ftimeCreation;
    if (tp->cd.idate > tp->wd.idate)    /* creation date more recent */
      ;                                 /* nothing */
    else if (tp->cd.idate == tp->wd.idate  &&  /* eq dates but */
             tp->ct.itime >  tp->wt.itime)   /* creation time more recent */
      ;                                 /* nothing */
    else {                              /* otherwise and non-HPFS volumes*/
      tp->cd.date = tp->wd.date;
      tp->ct.time = tp->wt.time;
      }
    tp->size  = cf->cbFile;
    strncpy(tp->fname, cf->achName, MAXFN);
    strupr(tp->fname);
    }
  return(tp);
  }

/* --------------------------------- */
/* Build comment entry in file-chain */
/* --------------------------------- */
FILECHAIN *add_comment(char  *filename,      /* comment entry (EMPTY) */
                       DOWNPATH  *parea,     /* ptr to area array */
                       char    *fdesc,       /* FILES.BBS line */
                       short   int fpriv,    /* area/file privilege */
                       USHORT  bbsseq)       /* line # in FILES.BBS */
{
  FILECHAIN *tp;                        /* ptr to file-info */
  char   *fd;                           /* ptr to file description */
  char   buftmp[MAXDESC];               /* work buffer */
  short int  k;                         /* counter */

  strncpy(buftmp, fdesc, MAXDESC);      /* save desc in buftmp */
  for (k=0; buftmp[k]!='\0'; k += 1) {  /* scan work buffer */
    if (buftmp[k]=='\r' || buftmp[k]=='\n') { /* for CR/LF */
      buftmp[k] = '\0';                 /* replace by end-of-string */
      break;
      }
    }
  tp = file_element(parea, NULL);       /* alloc new element */
  strncpy(tp->fname, filename, MAXFN);  /* EMPTY for comment */
  tp->priv = fpriv;                     /* privilege  */
  tp->fseq = bbsseq;                    /* FILES.BBS line number */
  tp->cmt  = 1;                         /* indicate as comment element */
  fd = malloc(k + 1);                   /* obtain memory for desc */
  if (fd == NULL) {
    fprintf(stderr, MSG_MEM, PROGNAME);    /* not enough memory */
    exit(12);
    }
  else {
    strcpy(fd, buftmp);                 /* store description             */
    tp->fdesc = fd;                     /* pointer to desc               */
    }
  return(tp);                           /* return pointer                */
  }

/* ------------------------------------------------------ */
/* supply file-entry with explicit path spec in FILES.BBS */
/* ------------------------------------------------------ */
void  add_pathspec(char  *filespec,     /* ASCIIZ full filespec */
                   FILECHAIN *tp)       /* pointer to file element */
{
#ifndef __32BIT__
  FILESTATUS cf;                        /* file-info buffer              */
#else
  FILESTATUS3 cf;                       /* file-info buffer              */
#endif
  unsigned int  fn_off;                 /* offset filename in pathspec   */

  if (
#ifndef __32BIT__
      DosQPathInfo(strupr(filespec), 1, (char *)&cf, sizeof(cf), 0L)
#else
      DosQueryPathInfo(strupr(filespec), 1, &cf, sizeof(cf))
#endif
           != 0) {
    fprintf(stderr, "\tfile %s not found\n", filespec);
    return;                             /* return without info */
    }
  else {
    fn_off = split_fname(filespec);     /* path size */
    strncpy(tp->fname, filespec+fn_off, MAXFN);
    tp->fname[MAXFN-1] = '\0';          /* ensure string termination */
    strupr(tp->fname);                  /* filename in capitals */
    tp->fpath = malloc(fn_off + 1);     /* mem for path */
    if (tp->fpath != NULL) {            /* memory obtained */
      strncpy(tp->fpath, strupr(filespec), fn_off);  /* copy path */
      *(tp->fpath + fn_off) = '\0';     /* end of string */
      }
    tp->wd.date = cf.fdateLastWrite;
    tp->wt.time = cf.ftimeLastWrite;
    tp->cd.date = cf.fdateCreation;
    tp->ct.time = cf.ftimeCreation;
    if (tp->cd.idate > tp->wd.idate)    /* creation date more recent */
      ;                                 /* nothing */
    else if (tp->cd.idate == tp->wd.idate  &&  /* eq dates but */
             tp->ct.itime >  tp->wt.itime)  /* creation time more recent */
      ;                                 /* nothing */
    else {                              /* otherwise and non-HPFS volumes*/
      tp->cd.date = tp->wd.date;
      tp->ct.time = tp->wt.time;
      }
    tp->size  = cf.cbFile;
    return;                             /* back to caller */
    }
  }

/* ----------------------------------------------------------- */
/* Remove orphans from chain and release memory of these.      */
/* Returns number of removed orphans (may be all files!)       */
/* ----------------------------------------------------------- */
long int free_orphan(FILECHAIN **c0)    /* start of group */
{
  long int  orp_cnt;                    /* removed-orphan counter */
  FILECHAIN *ca, *cb;                   /* current and next chain pointer*/

  orp_cnt = 0;                          /* init removed-orphan counter */
  ca = *c0;                             /* look at first element */
  while (ca != NULL) {                  /* whole group (until break) */
    if (ca->priv >= HIDDEN) {           /* orphan */
      cb = ca->next_element;            /* pointer to next element */
      *c0 = cb;                         /* next element past first orphan */
      free(ca);                         /* drop orphan */
      ++orp_cnt;                        /* bump up orphan count */
      ca = cb;                          /* shift to new first of group */
      }
    else                                /* current first not orphan */
      break;                            /* exit from while-loop */
    }
  while (ca != NULL) {                  /* all after 1st not-orphan */
    cb = ca->next_element;              /* next element */
    if (cb != NULL && cb->priv >= HIDDEN) {  /* orphan */
      ca->next_element = cb->next_element; /* new ptr in current element */
      free(cb);                         /* free memory block */
      ++orp_cnt;                        /* one more dropped */
      }                                 /* note: do not shift current */
    else                                /* not an orphan */
      ca = cb;                          /* shift +1 current element */
    }
  return(orp_cnt);                      /* report # of removed orphans */
  }


/* ----------------------------------------------------------- */
/* Count orphans                                               */
/* ----------------------------------------------------------- */
long int count_orphans(FILECHAIN *c0)   /* start of group */
{
  long int  orp_cnt;                    /* orphan counter */
  FILECHAIN *ca;                        /* current chain pointer */

  orp_cnt = 0;                          /* init removed-orphan counter */
  ca = c0;                              /* first of group */
  while (ca != NULL) {                  /* whole group */
    if (ca->priv >= HIDDEN)             /* orphan */
      ++orp_cnt;                        /* bump up orphan count */
    ca = ca->next_element;              /* pointer to next element */
    }
  return(orp_cnt);                      /* report # of removed orphans */
  }

/* ---------------------------------------------------- */
/* Combine consecutive comment lines to a single string */
/* Returns the number of freed chain elements           */
/* ---------------------------------------------------- */
long int combine_comments(void)         /* travels along filechain */
{
  USHORT     cmt_cnt;                   /* freed elements count */
  FILECHAIN  *ca, *cb;                  /* current and next chain pointer*/
  char       *new;                      /* ptr to combined comments */
  unsigned int k,l;                     /* string lengths */
  USHORT     nextseq;                   /* next expected sequence # */

  cmt_cnt = 0;                          /* initial freed elements count  */
  if ((ca=first_element) != NULL) {     /* any elements in chain */
    nextseq = 1 + ca->fseq;             /* next consecutive line */
    while ((cb=ca->next_element) != NULL) {  /* all elements */
      if (ca->fname[0] == '\0'  &&      /* comment #1 */
          cb->fname[0] == '\0'  &&      /* comment #2 */
          cb->fseq == nextseq) {        /* consecutive FILES.BBS lines */
        k = strlen(ca->fdesc);          /* take length of 1st comment */
        l = strlen(cb->fdesc);          /* take length of 2nd comment */
        if ((new = (char *)malloc(k+l+2)) != NULL) {  /* mem obtained */
          strcpy(new,ca->fdesc);        /* copy 1st comment */
          strcat(new,"\n");             /* insert newline */
          strcat(new,cb->fdesc);        /* copy 2nd comment */
          free(ca->fdesc);              /* release 1st old comment */
          free(cb->fdesc);              /* release 2nd old comment */
          ca->fdesc = new;              /* ptr to combined desc in cur el*/
          ca->next_element = cb->next_element; /* new ptr in cur el. */
          free(cb);                     /* release 2nd chain-element */
          cmt_cnt += 1;                 /* update combination counter */
          nextseq += 1;                 /* next bbsseq# */
          }
        else {
          ca = cb;                      /* no memory: proceed */
          nextseq = 1 + ca->fseq;
          }
        }
      else {                            /* not 2 consecutive comments */
        ca = cb;                        /* shift +1 current element */
        nextseq = 1 + ca->fseq;
        }
      }
    }                                   /* only orphans */
  return(cmt_cnt);                      /* report # of removed elements  */
  }

/* ----------------------------------------- */
/* Check if string could be a filename       */
/* Only needed as distiction from comment.   */
/* Returns NULL if probably not a filespec.  */
/* ----------------------------------------- */
char *parse_fname(char *buf)            /* ptr to FILES.BBS inputline */
{
  char  c0;                             /* first character */

  c0 = buf[0];                          /* take first character on line */
  if ( strlen(buf) < 2           ||     /* empty line */
       c0 <= ' '                 ||
       c0 == '\"'                ||
      (c0 >= '+'  && c0 <= '/')  ||
      (c0 >= ':'  && c0 <= '>')  ||
      (c0 >= '['  && c0 <= ']')  ||
       c0 == '|'                 ||
       c0 >= 128)
    return(NULL);                       /* probably not a filename */
  else
    return(buf);
  }

/* ----------------------------------- */
/* Split path (if any) from filespec.  */
/* Returns offset to filename in buf.  */
/* ----------------------------------- */
unsigned int split_fname(char *buf)     /* filespecification string */
{
  char  *pFile;                         /* pointer */
  char  tempbuf[2048];                  /* work buffer */

  strcpy(tempbuf, asciiz(buf));         /* copy first 'word' */
  pFile = strrchr(tempbuf,'\\');        /* search last backslash */
  if (pFile == NULL)                    /* no directory spec found */
    pFile = strrchr(tempbuf,':');       /* search last colon */
  if (pFile == NULL)                    /* also no drive spec found */
    return(0);                          /* zero: no explicit path spec'd */
  else                                  /* explicit pathspec found */
    return(pFile - tempbuf + 1);        /* offset to filename in buf */
  }


/* ------------------------------------- */
/* Determine if area is within selection */
/* Returns zero if to be excluded        */
/* ------------------------------------- */
int  area_selection(char *name)         /* areaname */
{
  int   result;                         /* return value */
  STRCHAIN *s;                          /* pointer to chain element */

  result = 1;                           /* included unless in/ex active  */

  if ((s = incl_area) != NULL) {        /* area_include active */
    result = 0;                         /* not included unless found */
    while (s != NULL) {                 /* scan include chain */
      if (!strnicmp(s->str, name, strlen(s->str))) {  /* to be included */
        result = 1;                     /* found to be included          */
        s = excl_area;                  /* pointer to exclude string */
        if (s != NULL) {                /* exclusion also active */
          while (s != NULL) {           /* all excluded areas */
            if (!strnicmp(s->str, name, strlen(s->str))) {
              result = 0;               /* found to be excluded          */
              break;                    /* escape from exclude loop      */
              }
            s = s->next;                /* pointer to next chain element */
            }
          }
        break;                          /* escape from include loop */
        }
      s = s->next;                      /* pointer to next chain element */
      }
    }
  else if ((s = excl_area) != NULL) {   /* explicit exclusion active    */
    result = 1;                         /* included unless found */
    while (s != NULL) {                 /* whole EXclude chain */
      if (!strnicmp(s->str, name, strlen(s->str))) {
        result = 0;                     /* found to be excluded */
        break;                          /* escape from exclude loop */
        }
      s = s->next;                      /* pointer to next element */
      }
    }
  else
    result = 1;                         /* no selections: all            */
  return(result);                       /* return selection result       */
  }

/* ----------------------------------------- */
/* Determine if FILENAME is within selection */
/* Returns zero if to be excluded            */
/* ----------------------------------------- */
int  file_selection(char *name)         /* filename */
{
  STRCHAIN *s;                          /* pointer to chain element */

  if ((s = excl_file) != NULL) {        /* file-exclude active */
    while (s != NULL) {                 /* whole chain */
      if (!wild_comp(name, s->str))     /* wildcards compare */
        return(0);                      /* found: to be excluded */
      s = s->next;                      /* next chain element */
      }
    }
  return(1);                            /* file to be included */
  }


#if defined(__DEBUG__)
/* =============== */
/* Dump area array */
/* =============== */
USHORT dump_area_array(DOWNPATH _HUGE *aa,  /* pointer to array */
                       ULONG  m,        /* number of area's */
                       char *id)        /* log id */
{
  ULONG  i;
  FILE  *log;

  log = fopen("downsort.log","a");      /* append!                       */
  fprintf(log,"\n");                    /* newline                       */
  sep_line(log,'Í',79);                 /* separate from previous append */
  fprintf(log,"(%s) %s\n", today, id);  /* timestamp + id                */
  fflush(log);
  for (i=0; i<m; i++) {
    fprintf(log,"\n%4ld. bytecount=%10lu priv=%5hd anum=%hd filecount=%5lu\n",
        i+1,
        aa[i].byte_count,
        aa[i].priv,
        aa[i].anum,
        aa[i].file_count);
    fprintf(log," name %s\nename %s\npname %s\nf_bbs %s\n desc %s\n",
        aa[i].name,
        aa[i].ename,
        aa[i].pname,
        aa[i].filesbbs,
        aa[i].adesc);
    fprintf(log,"newst %s\n",
        (aa[i].newest != NULL) ? aa[i].newest->fname : "-");
    fflush(log);                        /* write buffers before continue */
    }
  fclose(log);
  return(i);
  }

/* =============== */
/* Dump file chain */
/* =============== */
ULONG  dump_file_chain(FILECHAIN *f,    /* pointer to first el. of chain */
                       char *id)        /* log id                        */
{
  FILE      *log;
  ULONG      fc;                        /* element count                 */
  FILECHAIN *fe;                        /*                               */

  log = fopen("downsort.log","a");      /* append!                       */
  fprintf(log,"\n");                    /* newline                       */
  sep_line(log,'Í',79,0);               /* separate from previous append */
  fprintf(log,"(%s) %s\n\n", today, id);  /* timestamp + id              */
  fflush(log);                          /* force physical write(s)       */
  fe = f;                               /* pointer to first element      */
  fc = 0;                               /* counter=0                     */
  while(fe != NULL) {
    if (fe->fname[0] != '\0') {
      fprintf(log,"%*.*s", MAXANAME, MAXANAME, fe->parea->name); /* area */
      fprintf(log," %s ", fe->fname);   /* filename                      */
      }
    fprintf(log,"%s\n",
                (fe->fdesc != NULL) ? fe->fdesc : EMPTY);  /* file desc  */
    fc++;
    fe = fe->next_element;
    }
  fprintf(log,"\nChain_element_count = %lu\n\n", fc);
  fclose(log);
  return(fc);
  }
#endif

