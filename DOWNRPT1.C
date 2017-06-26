
/* ================================================================ */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility     */
/*  -> DOWNRPT1.C                                                   */
/*     Make: BBS-list, New-list, EMI-list                           */
/* ================================================================ */

/* #define __DEBUG__ */

#define INCL_BASE
#define INCL_NOPMAPI
#include <os2.h>

#include <conio.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "..\max\mstruct.h"
#include "downsort.h"
#include "downfpro.h"

/* --------------------------------------- */
/* Produce the bulletin format of BBS-list */
/* --------------------------------------- */
void make_bbs(FILECHAIN * _HUGE *dm,
              DOWNPATH  _HUGE *area,
              LISTPARM  *ls)            /* list specs */
{
  FILE  *pf;                            /* report file handle */
  char  outfile[MAXFN];                 /* file names */
  USHORT k;                             /* counters */
  ULONG  i,j,m,n,fc,fs;                 /* counters */
  ULONG  akb;                           /* bytecount in KB */
  FILECHAIN *cn, *fx;                   /* ptr to file info */
  char  *p;                             /* pointer string in strsubw */
#ifdef __DEBUG__
  ULONG starttime,stoptime; */  /* timing of sorts */
#endif

  sprintf(outfile,"%s.%s", ls->name, ls->ext);  /* build fname */
  pf = fopen(outfile,WRITE);            /* output file */
  if (pf != NULL) {                     /* successful open */
    if (oper_mode == VERBOSE)
      fprintf(stdout,MSG_SRT, file_total_count, area_total_count, outfile);
#ifdef __DEBUG__
    DosQuerySysInfo(QSV_MS_COUNT,       /* msecs since system start */
                    QSV_MS_COUNT,       /* that's all! */
                    (void *)&starttime, /* data buffer */
                    sizeof(starttime)); /* data buffer length */
#endif
    psort(dm, 0, file_total_count-1, sort_new); /* sort on date */
#ifdef __DEBUG__
    DosQuerySysInfo(QSV_MS_COUNT,       /* msecs since system start */
                  QSV_MS_COUNT,         /* that's all! */
                  (void *)&stoptime,    /* data buffer */
                  sizeof(stoptime));    /* data buffer length */
    fprintf(stdout,"sort-duration = %lu msecs\n", stoptime-starttime); */
#endif
    fc = preproc_area(area, dm, ls);    /* #files, #bytes  */
    n = rpt_count(dm, ls, &cn);         /* first 'n' to be proc'd */
    if (n > 0   &&                      /* any files for this report */
        (ls->sortflag == ALPHA  ||      /* re-sort first entries on name */
         ls->sortflag == GROUP)) {      /* synonym for this list */
      if (oper_mode == VERBOSE)
        fprintf(stdout, MSG_RST, n);    /* re-sort msg */
      psort(dm, 0, n-1, sort_gbl);      /* re-sort first 'n' entries */
      }
    if (oper_mode != QUIET)
      fprintf(stdout, MSG_REP, outfile);
    if (oper_mode == VERBOSE)
      fprintf(stdout, MSG_REC);
    ls->incl_fspec = "DOWNSORT.HDR";    /* header filename */
    file_incl(pf, ls);                  /* include bbs-header file */
    akb =  (count_bytes(area) + 512)/1024;
    fprintf(pf,"\n%c(%s) Last %c%hu %s%cnewest of a total of"
               " %c%lu%c files (%c%lu%c %cB)",
                O_CYAN,
                sys_date(today),
                O_YELLOW,
                (ls->listflag!=' ' || fc>ls->max_fil) ?
                       ls->max_fil : (USHORT)fc,
                (ls->listflag==' ') ? EMPTY :
                 ((ls->listflag=='D') ? DAYS :
                  ((ls->listflag=='W') ? WEEKS : MONTHS)),
                O_CYAN,
                O_YELLOW,
                fc,
                O_CYAN,
                O_BRIGHT+O_MAGENTA,
                (akb < 9999) ? akb : (akb+512)/1024,
                O_CYAN,
                (akb < 9999) ? 'K' : 'M');

    if ( (ls->sortflag == ALPHA  ||     /* for filename-sorted list  */
          ls->sortflag == GROUP)   &&   /* for filename-sorted list  */
            cn != NULL             &&   /* new file available */
            cn->wd.idate != 0) {        /* true date */
      fprintf(pf,"\n%19sNewest: %c%s %c%8s",
                    EMPTY, O_YELLOW, cn->fname,
                    O_GREEN,f_date(cn->wd.date) );
      fprintf(pf," %c(avail: %c%8s%c)",
                    O_CYAN,O_GREEN,
                    f_date(cn->cd.date),O_CYAN);
      }
    fprintf(pf,"\n%19s%s %c%s, %c%s",
               EMPTY, DF, DAYS_7, WK, DAYS_30, MO);
    if (ls->exclflag != EXCLPRIV)
      fprintf(pf,"\n\n%c(Your privilege-level may limit the number "
                "of files actually shown to you!)%c",O_RED,O_CYAN);
    fprintf(pf,"\n\n%c%s       %c%s    %c%s   %c%s    %c%s\n\n",
                O_YELLOW,FN,
                O_BRIGHT+O_RED,AC,
                O_MAGENTA,SZ,
                O_GREEN,DT,
                O_CYAN,DS);
    for (i=m=0; i<n; i++) {             /* first n array entries */
      fx = dm[i];
      if (rpt_coll(fx, ls, TRUE)) {     /* check if to be listed */
        if (oper_mode==VERBOSE && (m%25)==0) {
          fprintf(stdout, "%6lu\r",m);
          fflush(stdout);
          }
        m++;                            /* actually be reported */
        fprintf(pf,"%cL%c%c%-12.12s %c%-8.8s ",
                    '\20',
                    (fx->priv>SYSOP)?'S':priv_name[fx->priv-TWIT][0],
                    O_YELLOW,
                    fx->fname,
                    O_RED+O_BRIGHT,
                    fx->parea->name);
        if (fx->wd.date.day > 0) {     /* file(date) present */
          fs = (fx->size+512)/1024;  /* filesize in KB */
          fprintf(pf,"%c%4lu%c %c%s%c %c",
                    O_MAGENTA,
                    (fs<=9999) ? fs  : (fs+512)/1024,
                    (fs<=9999) ? 'K' : 'M',
                    O_GREEN,
                    f_date(fx->wd.date),
                    file_age_ind(fx),   /* file age */
                    O_CYAN);
          }
        else {                          /* file not found */
          fprintf(pf,"%c%15s %c",
                    O_MAGENTA,
                    OFFLINE,
                    O_CYAN);
          }
        if ((k=strsubw(fx->fdesc, &p, 41)) != 0) {  /* length            */
          if (ls->wrapflag != WRAP) {   /* truncate                */
            j = strlen(fx->fdesc);      /* total string length           */
            k = (USHORT)min(41L, j);    /* shortest of l1 and j          */
            }
          fprintf(pf,"%-.*s\n", k, p);  /* (1st part of) string          */
          while (k>0 && ls->wrapflag==WRAP) {  /* wrap requested   */
            if ((k=strsubw(p+k, &p, 41-ls->desc_indent)) != 0) {
              fprintf(pf,"%cL%c", '\20',
                 (fx->priv>SYSOP)?'S':priv_name[fx->priv-TWIT][0]);
              fprintf(pf,"%*s%-.*s\n",
                  79-41+ls->desc_indent,EMPTY, k, p);   /*  more      */
              }
            }
          }
        }
      }
    if (oper_mode==VERBOSE)
      fprintf(stdout, "%6lu\n",m);
    signature(pf, today);               /* fingerprint                   */
    ls->incl_fspec = "DOWNSORT.TRL";    /* trailer filename           */
    file_incl(pf, ls);                  /* include bbs-trailer file      */
    fclose(pf);                         /* finished with .BBS file       */
    }
  else                                  /* no output possible            */
    fprintf(stderr, MSG_OPO, outfile, 0);
  }

/* ------------------------------------------- */
/* Produce the file-request format of NEW-list */
/* ------------------------------------------- */
void make_new(FILECHAIN * _HUGE *dm,
              DOWNPATH  _HUGE *area,
              LISTPARM  *ls)            /* list specs                   */
{
  FILE     *pf;                         /* file handle                   */
  char     outfile[MAXFN];              /* file names                    */
  ULONG    i,m,n,fc;                    /* counters                      */
  ULONG    akb;                         /* bytecount in KB               */
  FILECHAIN *cn, *fx;                   /* ptr to file info              */
  char     aname[MAXANAME];             /* area code/name */

  sprintf(outfile,"%s.%s%c",
          ls->name,
          ls->ext,
          priv_name[ls->priv - TWIT][0]);
  pf = fopen(outfile,WRITE);            /* output file                   */
  if (pf != NULL) {                     /* opened!                       */
    if (oper_mode == VERBOSE)
      fprintf(stdout, MSG_SRT, file_total_count, area_total_count, outfile);
    psort(dm, 0, file_total_count-1, sort_new);
    fc = preproc_area(area, dm, ls);    /* count file, bytes */
    cn = NULL;                          /* no assigned */
    n = rpt_count(dm, ls, &cn);         /* first 'n' to be proc'd */
    if (n > 0) {                        /* any files for this report */
      if  (ls->sortflag == ALPHA) {     /* resort first entries on name  */
        if (oper_mode == VERBOSE)
          fprintf(stdout, MSG_RST, n);  /* re-sort msg */
        psort(dm, 0, n-1, sort_gbl);    /* sort first 'n' entries */
        }
      else if (ls->sortflag == GROUP) {  /* resort first entries on area */
        if (oper_mode == VERBOSE)
          fprintf(stdout, MSG_RST, n);  /* re-sort msg */
        psort(dm, 0, n-1, sort_all);    /* sort first 'n' entries */
        }
      }
    if (oper_mode != QUIET)
      fprintf(stdout, MSG_REP, outfile);
    if (oper_mode==VERBOSE)
      fprintf(stdout, MSG_REC);
    insert_title(pf, pre_title, 0);
    block_title(pf, 20, EMPTY, list_title, ls);
    file_incl(pf, ls);                  /* insert user-'logo'            */
    insert_title(pf, sub_title, 0);
    akb =  (count_bytes(area) + 512)/1024;
    fprintf(pf,"\n(%s) Last %hu %snewest of a total of %lu files (%lu %cB)",
                 sys_date(today),
                (ls->listflag!=' ' || fc>ls->max_fil) ?
                       ls->max_fil : (USHORT)fc,
                (ls->listflag==' ') ? EMPTY :
                 ((ls->listflag=='D') ? DAYS :
                  ((ls->listflag=='W') ? WEEKS : MONTHS)),
                 fc,
                 (akb < 9999) ? akb : (akb-512)/1024,
                 (akb < 9999) ? 'K' : 'M');
    if (ls->exclflag != EXCLPRIV)
      fprintf(pf,"\n%19s%s%s", EMPTY, MP, priv_name[ls->priv-TWIT]);
    if ((ls->sortflag == ALPHA  ||      /* for filename-sorted list only */
         ls->sortflag == GROUP) &&      /* synonym for ALPHA here */
         cn != NULL             &&      /* newest file */
         cn->wd.idate != 0) {           /* true date */
      fprintf(pf,"\n%19sNewest: %s %8s",
                 EMPTY, cn->fname, f_date(cn->wd.date));
      fprintf(pf," (avail: %8s)",
                 f_date(cn->cd.date));
      }
    fprintf(pf,"\n%19s%s %c%s, %c%s\n\n",
               EMPTY, DF, DAYS_7, WK, DAYS_30, MO);
    if (ls->sortflag != GROUP) {
      fprintf(pf,"%s       %s    %s   %s    %s\n",FN,AC,SZ,DT,DS);
      sep_line(pf, 'Ä', 12, 8, 5, 9, 41, 0);
      }
    aname[0] = '\0';                    /* initial areaname is empty */
    for (i=m=0; i<n; i++) {
      fx = dm[i];                       /* copy pointer */
      if (rpt_coll(fx, ls, TRUE)) {     /* check if to be listed */
        if (oper_mode==VERBOSE && (m%25)==0) {
          fprintf(stdout, "%6lu\r", m);
          fflush(stdout);
          }
        ++m;                            /* actually listed               */
        if (ls->sortflag == GROUP) {    /* areagroup */
          if (strcmp(aname, fx->parea->name)) {  /* new area */
            strcpy(aname, fx->parea->name);      /* remember */
            fprintf(pf,"\n");           /* insert space */
            sep_line(pf,'Í', 79, 0);    /* separator line */
            fprintf(pf," %s  -  %-.64s\n",
                        aname,
                        fx->parea->adesc);
            sep_line(pf,'Ä', 79, 0);    /* separator line */
            }
          fprintf(pf,"%-12.12s %15s ",
                      fx->fname,
                      f_size_date(fx));
          desc_part(pf, fx->fdesc, 50, 50-ls->desc_indent, ls);
          }
        else {
          fprintf(pf,"%-12.12s %-8.8s %15s ",
                      fx->fname,
                      fx->parea->name,
                      f_size_date(fx));
          desc_part(pf, fx->fdesc, 41, 41-ls->desc_indent, ls);
          }
        }
      }
    if (oper_mode==VERBOSE)
      fprintf(stdout,"%6lu\n",m);
    signature(pf,today);                /* leave fingerprint             */
    insert_title(pf, bot_title, 0);
    fclose(pf);                         /* finished with .NEW file       */
    }
  else
    fprintf(stderr, MSG_OPO, outfile, 0);
  }

/* ------------------------------------------- */
/* Produce the file-request format of EMI-list */
/* This is a more compact variant of NEW-list. */
/* File date, time and size are exact.         */
/* ------------------------------------------- */
void make_emi(FILECHAIN * _HUGE *dm,
              DOWNPATH  _HUGE *area,
              LISTPARM  *ls)            /* list specs                    */
{
  FILE     *pf;                         /* file handle                   */
  char     outfile[MAXFN];              /* file names                    */
  ULONG    i,m,n;                       /* counters                      */
  FILECHAIN  *cn, *fx;                  /* ptr to file info              */

  sprintf(outfile,"%s.%s%c",
          ls->name,
          ls->ext,
          priv_name[ls->priv-TWIT][0]);
  pf = fopen(outfile,WRITE);            /* output file */
  if (pf != NULL) {                     /* opened! */
    if (oper_mode == VERBOSE)
      fprintf(stdout,MSG_SRT,file_total_count,area_total_count,outfile);
    psort(dm, 0, file_total_count-1, sort_new);
    preproc_area(area, dm, ls);         /* count file, bytes */
    cn = NULL;                          /* no assigned */
    n = rpt_count(dm, ls, &cn);         /* first 'n' to be proc'd */
    if (n > 0    &&                     /* resort first entries on name */
        (ls->sortflag == ALPHA  ||      /* resort first entries on name */
         ls->sortflag == GROUP)) {      /* synonym in EMI-list */
      if (oper_mode == VERBOSE)
        fprintf(stdout, MSG_RST, n);    /* re-sort msg */
      psort(dm, 0, n-1, sort_gbl);      /* sort first 'n' entries */
      }
    if (oper_mode != QUIET)
      fprintf(stdout, MSG_REP, outfile);
    if (oper_mode==VERBOSE)
      fprintf(stdout, MSG_REC);
    file_incl(pf, ls);                  /* insert user text */
    fprintf(pf,"\n(%s) Last %hu %semissions",
                 sys_date(today),
                 ls->max_fil,
                (ls->listflag==' ') ? EMPTY :
                 ((ls->listflag=='D') ? DAYS :
                  ((ls->listflag=='W') ? WEEKS : MONTHS)));
    fprintf(pf,"\n\n--%s-- --%s--  -%s-  -%s-  ---%s---\n",
                  FN,DT,TM,BY,DS);
    for (i=m=0; i<n; i++) {
      fx = dm[i];                       /* copy pointer to file info */
      if (rpt_coll(fx, ls, TRUE)) {     /* check if to be listed */
        if (oper_mode==VERBOSE && (m%25)==0)
          fprintf(stdout, "%6lu\r",m);
        ++m;                            /* file list-count               */
        fprintf(pf,"%-12.12s %8.8s  %6.6s %8lu  ",
              fx->fname,
              f_date(fx->wd.date),
              f_time(fx->wt.time),
              fx->size);
        desc_part(pf, fx->fdesc, 39, 66-ls->desc_indent, ls);
        }
      }
    if (oper_mode==VERBOSE)
      fprintf(stdout, "%6lu\n", m);
    fprintf(pf,"\n-- List created with %s %c.%c%c by %s --\n\n",
                PROGNAME,VERSION,SUBVERS,SUFFIX,AUTHOR);
    fclose(pf);                         /* finished with .EMI file       */
    }
  else
    fprintf(stderr, MSG_OPO, outfile, 0);
  }

