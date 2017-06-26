
/* ================================================================ */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility     */
/*  -> DOWNRPT2.C                                                   */
/*  -> Make all types of IPF-lists.                                 */
/* ================================================================ */

#define INCL_BASE
#define INCL_NOPMAPI
#include <os2.h>

#include <conio.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "..\max\mstruct.h"
#include "downsort.h"
#include "downfpro.h"

/* IPF-tags */
static char  AW[]   = ":artwork align=center name='DOWNSORT.BMP'.";
static char  CD[]   = ":color fc=default.";
static char  CG[]   = ":cgraphic.";
static char  CR[]   = ":color fc=red.";
static char  CY[]   = ":color fc=yellow.";
static char  DP[]   = ":docprof toc=123.";
static char  ED[]   = ":euserdoc.\n";
static char  EG[]   = ":ecgraphic.";
static char  EL[]   = ":elines.\n";
static char  ET[]   = ":etable.\n";
static char  H1[]   = ":h1.";
static char  H2[]   = ":h2";
static char  H3[]   = ":h3";
static char  OV[]   = "Overview";
static char  LI[]   = ":lines align=center.";
static char  LL[]   = ":lines align=left.";
static char  LP[]   = ":lp.";
static char  TB[]   = ":table cols='12 15 38' frame=box rules=both.\n";
static char  TI[]   = ":title.";
static char  UD[]   = ":userdoc.";

/* prototypes of local functions */

void ipf_list_head(FILE *, FILECHAIN * _HUGE *,
                   DOWNPATH _HUGE *, LISTPARM *);
void ipf_area_oview(FILE *, DOWNPATH _HUGE *, LISTPARM *);
void ipf_area_head(FILE *, FILECHAIN * _HUGE *, LISTPARM *, ULONG);
int  ipf_file_entry(FILE *, FILECHAIN *, LISTPARM *, ULONG);
int  ip2_file_entry(FILE *, FILECHAIN *, LISTPARM *);


/* ------------------------------------------------------- */
/* Produce the file-request format of IPFfiles  (IPF-list) */
/* ------------------------------------------------------- */
void make_ipf(FILECHAIN * _HUGE *dm,
              DOWNPATH _HUGE *area,
              LISTPARM  *ls)
{
  FILE   *pf;                           /* file handle                   */
  char   outfile[MAXFN];                /* file names                    */
  char   ac[40];                        /* area name                     */
  ULONG  i, n, pmax;                    /* counters                      */
  USHORT l,pcnt;                        /* counters                      */
  FILECHAIN *fx;                        /* pointer to file info          */

  sprintf(outfile,"%s.%s%c",
          ls->name,
          ls->ext,
          priv_name[ls->priv-TWIT][0]);
  pf = fopen(outfile, WRITE);           /* output file                   */
  if (pf != NULL) {
    if (oper_mode == VERBOSE)
      fprintf(stdout, MSG_SRT, file_total_count, area_total_count, outfile);
    ipf_list_head(pf, dm, area, ls);    /* sort + list header         */
    if (oper_mode!=QUIET)
      fprintf(stdout, MSG_REP, outfile);
    ipf_area_oview(pf, area, ls);       /* create area overview          */
    if (oper_mode==VERBOSE)             /* keep operator awake           */
      fprintf(stdout, MSG_REC, outfile);

    fprintf(pf,"%s%s\n",H1,"File List per Area");
    fprintf(pf,"%s\n%s :hp8.%c:ehp8.%s, :hp8.%c:ehp8.%s\n%s\n",
               LI, DF, DAYS_7, WK, DAYS_30, MO, EL);
    ac[0] = '\0';                       /* null area name */
    l=0;                                /* init file counters */
    n=0;                                /* init file counters */
    for (i=0; i<file_total_count; i++) {    /* all files */
      fx = dm[i];                       /* copy pointer */
      if (rpt_coll(fx, ls, TRUE)) {     /* check if to be listed */
        if (strcmp(ac, fx->parea->name)) { /* new area collection */
          strcpy(ac, fx->parea->name);  /* set new */
          if (fx->parea->file_count > 0) { /* any files in this area */
            if (l>0)                    /* have to close open list? */
              fprintf(pf,":edl.\n");    /* close previous area-list */
            ipf_area_head(pf, dm, ls, i);  /* create area header */
            pcnt = l = 0;               /* sub-counters this area */
            }
          }
        pmax = 1 + fx->parea->file_count/ls->max_fil;
        if ((l % ls->max_fil) == 0) {   /* 'page' separations */
          if (l == 0) {                 /* only for first entry */
            if (fx->parea->file_count > ls->max_fil)  /* split  */
              fprintf(pf,":p.(part %d of %d)\n", ++pcnt, pmax);
            }
          else {                        /* only part 2 and up            */
            fprintf(pf,":edl.\n");      /* end of previous part          */
            ++pcnt;                     /* increment part counter        */
            fprintf(pf,":p.Jump to: :link reftype=hd refid=%.8sP%d.\n"
                       "part %d of %d:elink.",
                       fx->parea->name, pcnt, pcnt, pmax);
            fprintf(pf,"\n%s id=%.8sP%d.%s - (part %d of %d)\n",
                       H3, fx->parea->name, pcnt, ac, pcnt, pmax);
            fprintf(pf,":p.%s (part %d of %d)\n",
                       stripf(fx->parea->adesc), pcnt, pmax);
            }
          fprintf(pf,":dl tsize=20 compact.\n");
          fprintf(pf,":hp6.:dthd.%s:ddhd.%s:ehp6.\n",FN,DS);
          }
        ++l;                                 /* listed lines this page */
        n += ipf_file_entry(pf, fx, ls, i);  /* print entry            */
        if (oper_mode==VERBOSE && (n%25)==0) {
          fprintf(stdout, "%6lu\r",n);       /* report progress        */
          fflush(stdout);
          }
        }
      }
    fprintf(pf,":edl.\n");              /* end of file-list last area    */
    if (oper_mode==VERBOSE)             /* last value                    */
      fprintf(stdout, "%6lu\n", n);
    fprintf(pf,"%s%s\n%s\n\n",H1,"Epilog",LI);
    signature(pf,stripf(today));        /* leave fingerprint             */
    insert_title(pf, bot_title, 1);
    fprintf(pf, "%s%s", EL, ED);
    stripf(NULL);                       /* let stripf free memory        */
    fclose(pf);                         /* finished with .IPF file       */
    }
  else {
    if (oper_mode!=QUIET)
      fprintf(stderr, MSG_OPO, outfile, 0);    /* open failed          */
    }
  }

/* ------------------------------------------------------- */
/* Produce the file-request format of IP2-files (IPF-list) */
/* ------------------------------------------------------- */
void make_ip2(FILECHAIN * _HUGE *dm,
              DOWNPATH _HUGE *area,
              LISTPARM  *ls)
{
  FILE    *pf;                          /* file handle                  */
  char    outfile[MAXFN];               /* file names                   */
  char    ac[40];                       /* area name                    */
  ULONG   i, n, pmax;                   /* counters                     */
  USHORT  l, pcnt;                      /* counters                     */
  FILECHAIN *fx;                        /* local pointer to file info   */

  sprintf(outfile,"%s.%s%c",
          ls->name,
          ls->ext,
          priv_name[ls->priv-TWIT][0]);
  pf = fopen(outfile,WRITE);            /* output file                   */
  if (pf != NULL) {
    if (oper_mode == VERBOSE)
      fprintf(stdout, MSG_SRT, file_total_count, area_total_count, outfile);
    ipf_list_head(pf, dm, area, ls);    /* sort + list header         */
    if (oper_mode!=QUIET)
      fprintf(stdout, MSG_REP, outfile);
    ipf_area_oview(pf, area, ls);       /* create area overview          */
    if (oper_mode==VERBOSE)             /* keep operator awake           */
      fprintf(stdout, MSG_REC, outfile);

    fprintf(pf,"%s%s\n",H1,"File List per Area");
    fprintf(pf,"%s\n%s :hp8.%c:ehp8.%s, :hp8.%c:ehp8.%s\n%s\n",
               LI, DF, DAYS_7, WK, DAYS_30, MO, EL);
    ac[0] = '\0';                       /* null area name                */
    l = 0;                              /* init file counters            */
    n = 0;                              /* init file counters            */
    for (i=0; i<file_total_count; i++) {    /* all files                 */
      fx = dm[i];                       /* copy pointer to file info     */
      if (rpt_coll(fx, ls, TRUE)) {     /* check if to be listed */
        if (strcmp(ac, fx->parea->name)) { /* new area collection          */
          strcpy(ac, fx->parea->name);   /* set new                        */
          if (fx->parea->file_count > 0) { /* any files in this area       */
            if (l>0)                      /* have to close open list?      */
              fprintf(pf,ET);             /* close previous table          */
            ipf_area_head(pf, dm, ls, i);   /* create area header         */
            pcnt = l = 0;                 /* init sub-counters this area   */
            }
          }
        pmax = 1 + fx->parea->file_count/ls->max_fil;
        if ((l % ls->max_fil) == 0) {   /* 'page' separations      */
          if (l == 0) {                 /* only for first entry          */
            if (fx->parea->file_count > ls->max_fil)  /* split  */
              fprintf(pf,":p.(part %d of %d)\n", ++pcnt, pmax);
            }
          else {                        /* only part 2 and up            */
            fprintf(pf,ET);             /* end of previous table         */
            ++pcnt;                     /* increment part counter        */
            fprintf(pf,":p.Jump to: :link reftype=hd refid=%.8sP%d.\n"
                       "part %d of %d:elink.",
                       fx->parea->name, pcnt, pcnt, pmax);
            fprintf(pf,"\n%s id=%.8sP%d.%s - (part %d of %d)\n",
                       H3, fx->parea->name, pcnt, ac, pcnt, pmax);
            fprintf(pf,":p.%s (part %d of %d)\n",
                       stripf(fx->parea->adesc), pcnt, pmax);
            }
/*        fprintf(pf,":font facename=Helv size=10x8.\n");  */
          fprintf(pf,TB);
          }
        ++l;                            /* listed lines (incl comments) */
        n += ip2_file_entry(pf, fx, ls);       /* print entry           */
        if (oper_mode==VERBOSE && (n%25)==0) {
          fprintf(stdout, "%6lu\r",n);  /* report progress              */
          fflush(stdout);
          }
        }
      }
    fprintf(pf,ET);                     /* end of table this area       */
    if (oper_mode==VERBOSE)             /* last value                   */
      fprintf(stdout, "%6lu\n", n);
    fprintf(pf,"%s%s\n%s\n\n",H1,"Epilog",LI);
    signature(pf,stripf(today));        /* leave fingerprint            */
    insert_title(pf, bot_title, 1);
    fprintf(pf,"%s%s",EL,ED);
    stripf(NULL);                       /* let stripf free memory       */
    fclose(pf);                         /* finished with .IPF file      */
    }
  else {
    if (oper_mode!=QUIET)
      fprintf(stdout, MSG_OPO, outfile, 0);    /* open failed         */
    }
  }

/* ------------------------ */
/* generate IPF list header */
/* ------------------------ */
void ipf_list_head(FILE *pf,
                   FILECHAIN * _HUGE *dm,
                   DOWNPATH _HUGE *area,
                   LISTPARM  *ls)
{
  char   s[MAXDESC];                    /* work buffer                   */

  switch(ls->sortflag) {
    case ALPHA:
    case GROUP:     psort(dm, 0, file_total_count-1, sort_all); break;
    case TIMESTAMP: psort(dm, 0, file_total_count-1, sort_al2); break;
    case KEEPSEQ:   psort(dm, 0, file_total_count-1, sort_akp); break;
    default: break;
    }
  preproc_area(area, dm, ls);
  fprintf(pf,"%s\n%s%s\n%s\n", UD, TI, stripf(list_title), DP);
  fprintf(pf,"%s%s\n%s\n", H1, stripf(list_title),LI);
  insert_title(pf, pre_title, 1);
  fprintf(pf,"%s%s\n%s\n", EL, CG, CR);
  block_title(pf, 20, EMPTY, list_title, ls),
  fprintf(pf,"%s\n%s\n", CD, EG);
  fprintf(pf,"%s\n", CD);               /* ensure default colors */
  fprintf(pf,"%s\n", LI);
  insert_title(pf, sub_title, 1);
  fprintf(pf,"%s%s%s%s%s\n", EL, H1, "About ", PROGNAME, " program");
  fprintf(pf,"\n%s\n", AW);
  fprintf(pf,"\n%s\n%s  Version %c.%c%c\n",
                   LI, PROGNAME, VERSION, SUBVERS, SUFFIX);
  fprintf(pf,"\nby %s\n\n%s\n", AUTHOR, CITY);
  sprintf(s,"\n%s\n%s\n", PHONE, FIDO);
  fprintf(pf,"%s\n", stripf(s));
  fprintf(pf,"%s",EL);
  }

/* ------------------------------- */
/* generate IPF list area overview */
/* ------------------------------- */
void ipf_area_oview(FILE *pf,
                   DOWNPATH _HUGE *area,
                   LISTPARM  *ls)
{
  char     s[MAXDESC];                  /* pointer to line buffer        */
  USHORT   j;                           /* counter                       */
  DOWNPATH _HUGE *area2;                /* copy of area-array for sort   */
  DOWNPATH *a1, *a2;                    /* local pointer to area info    */

  if (oper_mode==VERBOSE)
    fprintf(stdout, MSG_REP, OV);
#ifndef __32BIT__
  area2 = (DOWNPATH huge *)halloc(area_total_count, sizeof(DOWNPATH));
#else
  area2 = (DOWNPATH *)malloc(area_total_count * sizeof(DOWNPATH));
#endif
                                        /* dup area-array for sorting!   */
  if (area2 == NULL) {                  /* memory not obtained?          */
    if (oper_mode!=QUIET)
      fprintf(stderr, "Not enough memory for summary report, skipped.\n");
    }
  else {

    for (j=0; j<area_total_count; j++) {
      a1 = &area[j];
      a2 = &area2[j];
      memmove(a2, a1, sizeof(DOWNPATH));
      }
    qsort(area2, area_total_count, sizeof(DOWNPATH), sort_summ);
    fprintf(pf, "%s%s\n",H1,OV);
    fprintf(pf, "%s\n%s\n",CG,CR);
    block_title(pf, 8, EMPTY, OV, ls);
    fprintf(pf, "%s\n%s\n",CD,EG);
    fprintf(pf, "%s\nlist creation date: %s\n",
                LI,stripf(sys_date(today)));
    if (ls->exclflag != EXCLPRIV)
      fprintf(pf, "%s%s\n", MP, priv_name[ls->priv-TWIT]);
    fprintf(pf, "%s%s\n:hp2.\n%-8s %-35s %5s %8s\n",EL,CG,AC,DS,FS,BY);
    sep_line(pf, 'Ä', 44, 5, 9, 0);
    for (j=0; j<area_total_count; j++) {
      a2 = &area2[j];                   /* pointer to info of 1 area */
      if (a2->file_count) {
        fprintf(pf,":link reftype=hd refid=A$%.8s.%-8.8s:elink.",
                   a2->name,
                   a2->name);
        sprintf(s," %-35.35s %5lu %8luK",
                   a2->adesc,
                   a2->file_count,
                   (a2->byte_count+512)/1024);
        fprintf(pf,"%s\n",stripf(s));
        }
      }
    sep_line(pf, 'Ä', 44, 5, 9, 0);
    fprintf(pf,"%44s %5lu %8luK\n:ehp2.\n%s\n",
               "Total:",
               count_files(area2),
               (count_bytes(area2)+512)/1024,
               EG);
#ifndef __32BIT__
    hfree(area2);                       /* free copy of area array       */
#else
    free(area2);                        /* free copy of area array       */
#endif
    }
  }


/* ------------------------- */
/* generate IPF area heading */
/* ------------------------- */
void ipf_area_head(FILE *pf,
                   FILECHAIN * _HUGE *dm,
                   LISTPARM  *ls,
                   ULONG     i)
{
  ULONG  akb;                           /* area contents in KB */
  FILECHAIN *fx;

  fx = dm[i];
  fprintf(pf,":p.Continue with filearea"
          ":link reftype=hd refid=A$%.8s. %s :elink.&colon. ",
           fx->parea->name,
           fx->parea->name);
  fprintf(pf,"%-45.45s\n", stripf(fx->parea->adesc));
  fprintf(pf,"%s id=A$%.8s.%s - %s\n%s\n",
          H2,
          fx->parea->name,
          fx->parea->name,
          stripf(fx->parea->adesc),CG);
  akb = (fx->parea->byte_count + 511)/1024;  /* rounded to KB */
  if (max_aname <= 3) {                 /* short areanames */
    fprintf(pf,"%s%s%s º :hp2.%s:ehp2.\n",
            CR,
            strnblk(fx->parea->name,3,ls->tfont,LINE1),
            CD,
            stripf(fx->parea->adesc));
    fprintf(pf,"%s%s%s º :hp2.Available: %lu files (%lu %cB):ehp2.\n",
             CR, strnblk(fx->parea->name,3,ls->tfont,LINE2), CD,
             fx->parea->file_count,
             (akb < 9999) ? akb : (akb+511)/1024,
             (akb < 9999) ? 'K' : 'M');
    fprintf(pf,"%s%s%s º",CR,strnblk(fx->parea->name,3,ls->tfont,LINE3),CD);
    if (ls->exclflag != EXCLPRIV)
      fprintf(pf," :hp2.Privilege: %-.9s:ehp2.\n",
                  priv_name[fx->parea->priv-TWIT]);  /* area priv     */
    else
      fprintf(pf,"\n");
    fprintf(pf,"%s%s%s º",CR,strnblk(fx->parea->name,3,ls->tfont,LINE4),CD);
    }
  else {                                /* long areanames                */
    fprintf(pf,"%s\n", CR);
    block_title(pf, 20, EMPTY, fx->parea->name, ls),
    fprintf(pf,"%s\n\n", CD);
    fprintf(pf," :hp2.%s:ehp2.\n", stripf(fx->parea->adesc));
    fprintf(pf," :hp2.Available: %lu files (%lu %cB):ehp2.\n",
             fx->parea->file_count,
             (akb < 9999) ? akb : (akb+511)/1024,
             (akb < 9999) ? 'K' : 'M');
    if (ls->exclflag != EXCLPRIV)
      fprintf(pf," :hp2.Privilege: %-.9s:ehp2.\n",
                  priv_name[fx->parea->priv-TWIT]);  /* area priv */
    }
  if (fx->parea->newest != NULL  &&  /* newest file */
      fx->parea->newest->wd.idate != 0) {   /* true date */
    fprintf(pf," :hp2.Newest:");
    fprintf(pf," %s",stripf(fx->parea->newest->fname));
    fprintf(pf," %s",stripf(f_date(fx->parea->newest->wd.date)));
    fprintf(pf," (avail: %s):ehp2.\n",
                stripf(f_date(fx->parea->newest->cd.date)));
    }
  fprintf(pf,"%s\n",EG);
  }

/* ----------------------- */
/* generate IPF file entry */
/* ----------------------- */
int  ipf_file_entry(FILE *pf,
                    FILECHAIN *fx,
                    LISTPARM  *ls,
                    ULONG     i)
{
  int   rc;
  int   k;
  char  ageflag, *strptr;

  rc = 0;
  if (fx->fname[0] != '\0') {           /* filename present           */
    fprintf(pf, ":dt.:link refid=F$%lu reftype=fn.%-s:elink.",
               i, stripf(fx->fname));   /* filename                   */
    ageflag = file_age_ind(fx);         /* file age flag    */
    if (ageflag!=' ')
      fprintf(pf,":hp8.%c:ehp8.", ageflag);
    fprintf(pf,"\n:dd.");
    k = strsubw(fx->fdesc, &strptr, 240);
    if (k>0)
      fprintf(pf, (fx->size) ? "%s\n" : ":hp1.%s:ehp1.\n",
           stripf(strptr));             /* description                   */
    fprintf(pf,":fn id=F$%lu.%s:efn.\n",
           i,                           /* file number                   */
           f_size_date(fx));
    rc = 1;                             /* file entry printed            */
    }
  else if(ls->sortflag == KEEPSEQ)  /* '/K' specified              */
    fprintf(pf,"%s%s%-s%s%s",
            LL, CY,
            (strip_ava == 'Y') ? strava(fx->fdesc) : fx->fdesc,
            CD, EL);
  return(rc);                           /* report result                 */
  }

/* ----------------------- */
/* generate IPF file entry */
/* ----------------------- */
int  ip2_file_entry(FILE *pf,
                    FILECHAIN *fx,
                    LISTPARM  *ls)
{
  int rc;

  rc = 0;
  if (fx->fname[0] != '\0') {           /* filename present              */
    fprintf(pf,":row.:c.%s:c.%s",
           stripf(fx->fname),           /* filename                      */
           f_size_date(fx));
    fprintf(pf, (fx->size) ? ":c.%s\n" : ":c.:hp1.%s:ehp1.\n",
           stripf(fx->fdesc));          /* description                   */
    rc = 1;                             /* file entry printed            */
    }
  else if(ls->sortflag == KEEPSEQ)      /* '/K' specified              */
    fprintf(pf,"%s%s%-s%s%s",
            LL, CY,
            (strip_ava == 'Y') ? strava(fx->fdesc) : fx->fdesc,
            CD, EL);
  return(rc);                           /* report result                 */
  }

/* ---------------------------------------------------- */
/* function to 'neutralise' IPFC conflicting characters */
/* ---------------------------------------------------- */
char *stripf(char *s)
{
  int i,j,k;

  static char colon[] = "&colon.";
  static char amp[]   = "&amp.";
  static char grave[] = "&rbl.";        /* translated to required BLANK  */
  static int  maxline = MAXDESC;        /* length of work-buffer         */
  static char *t = NULL;                /* pointer to work-buffer        */

  if (s == NULL && t != NULL) {         /* no input-string               */
    free(t);                            /* free-up memory                */
    t = NULL;                           /* for next time use             */
    return(s);
    }

  if (t == NULL)                        /* no memory yet                 */
    t = malloc(maxline);                /* get it now                    */

  if (t != NULL) {
    for (i=j=0; s[i] && j<maxline-10; ++i) { /* keep some slack          */
      switch(s[i]) {
        case ':': for (k=0;  colon[k]; )
                    t[j++] = colon[k++];
                  break;
        case '`': for (k=0;  grave[k]; )
                    t[j++] = grave[k++];
                  break;
        case '&': for (k=0;  amp[k]; )
                    t[j++] = amp[k++];
                  break;
        default:  t[j++] = s[i];
                  break;
        }
      }
    t[j] = '\0';                        /* end of string                 */
    return(t);                          /* pointer to converted line     */
    }
  else
    return(s);                          /* no conversion done            */
  }
