
/* ================================================================ */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility     */
/*  -> DOWNRPT4.C                                                   */
/*  -> Make GBL-list, ALL-list.                                     */
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

/* prototypes of local functions */
/* ------------------------------------------- */
/* Produce the file-request format of GBL-list */
/* ------------------------------------------- */
void make_gbl(FILECHAIN * _HUGE *dm,
              DOWNPATH  _HUGE *area,
              LISTPARM  *ls)            /* report privilege index */
{
  FILE   *pf;                           /* file handle */
  char   outfile[MAXFN];                /* file names */
  ULONG  i,j,fc;                        /* counters */
  ULONG  akb;                           /* bytecount in KB */
  FILECHAIN *cn;                        /* pointer to info of spec. file */
  FILECHAIN *fx;                        /* pointer to file info */

  sprintf(outfile,"%s.%s%c",
          ls->name,
          ls->ext,
          priv_name[ls->priv-TWIT][0]);
  pf = fopen(outfile,WRITE);            /* output file */
  if (pf != NULL) {
    if (oper_mode == VERBOSE)
      fprintf(stdout, MSG_SRT, file_total_count, area_total_count, outfile);
    switch(ls->sortflag) {
      case ALPHA:
      case GROUP:     psort(dm,0,file_total_count-1,sort_gbl); break;
      case TIMESTAMP: psort(dm,0,file_total_count-1,sort_new); break;
      default: break;
      }
    fc = preproc_area(area, dm, ls);
    cn = NULL;                          /* no assigned */
    for (i=0; i<file_total_count; ++i) {   /* stop at end of files */
      fx = dm[i];                       /* copy pointer to file info */
      if (rpt_coll(fx, ls, TRUE))       /* check if to be listed */
        cn = new_acq(fx, cn);           /* keep pointer to most recent */
      }
    if (oper_mode != QUIET)
      fprintf(stdout, MSG_REP, outfile);
    if (oper_mode==VERBOSE)
      fprintf(stdout, MSG_REC);
    insert_title(pf, pre_title, 0);
    block_title(pf, 20, EMPTY, list_title, ls);
    file_incl(pf, ls);                  /* insert user-'logo' */
    insert_title(pf, sub_title, 0);
    akb = (count_bytes(area) + 512)/1024;    /* KBytes total */
    fprintf(pf,"\n(%s) Available: %lu files (%lu %cB)",
                  sys_date(today),
                  fc,
                  (akb < 9999) ? akb : (akb + 512)/1024,
                  (akb < 9999) ? 'K' : 'M');
    if (ls->exclflag != EXCLPRIV)
      fprintf(pf,"\n%19s%s%s",
               EMPTY, MP, priv_name[ls->priv-TWIT]);
    if (cn != NULL  &&
        cn->wd.idate != 0) {            /* true date */
      fprintf(pf,"\n%19sNewest: %s %8s",
               EMPTY,cn->fname,f_date(cn->wd.date));
      fprintf(pf," (avail: %8s)",f_date(cn->cd.date));
      }
    fprintf(pf,"\n%19s%s %c%s, %c%s",
               EMPTY, DF, DAYS_7, WK, DAYS_30, MO);
    fprintf(pf,"\n\n%s       %s    %s   %s    %s\n",FN,AC,SZ,DT,DS);
    sep_line(pf, 'Ä', 12, 8, 5, 9, 41, 0);
    for (i=j=0; i<file_total_count; i++) {
      fx = dm[i];                       /* copy pointer to file info */
      if (rpt_coll(fx, ls, TRUE)) {     /* check if to be listed */
        if (oper_mode==VERBOSE && (j%25)==0) {
          fprintf(stdout, "%6lu\r", j);
          fflush(stdout);
          }
        if (fx->fname[0] != '\0') {  /* not a comment-entry */
          j++;
          fprintf(pf,"%-12.12s %-8.8s %15s ",
                  fx->fname,
                  fx->parea->name,
                  f_size_date(fx));
          desc_part(pf, fx->fdesc, 41, 41-ls->desc_indent, ls);
          }
        }
      }
    if (oper_mode==VERBOSE)
      fprintf(stdout, "%6lu\n", j);
    signature(pf,today);                /* leave fingerprint */
    insert_title(pf, bot_title, 0);
    fclose(pf);                         /* finished with .GBL file */
    }
  else
    fprintf(stderr, MSG_OPO, outfile, 0);      /* no GBL list! */
  }

/* ------------------------------------------------------- */
/* Produce the file-request format of ALLfiles  (ALL-list) */
/* ------------------------------------------------------- */
void make_all(FILECHAIN * _HUGE *dm,
              DOWNPATH  _HUGE *area,
              LISTPARM  *ls)
{
  FILE     *pf;                         /* file handle */
  char     outfile[MAXFN];              /* file names */
  char     ac[40];                      /* area name */
  ULONG    i, j, n, fc;                 /* counters */
  USHORT   k;                           /* counters */
  ULONG    akb;                         /* bytecount in KB */
  ULONG    lock;                        /* MAX lock pattern */
  DOWNPATH _HUGE *area2;                /* copy of area-array for sort */
  FILECHAIN *fx, *cn;                   /* pointer to file info */

  sprintf(outfile,"%s.%s%c",
          ls->name,
          ls->ext,
          priv_name[ls->priv-TWIT][0]);
  pf = fopen(outfile,WRITE);            /* output file */
  if (pf != NULL) {
    if (oper_mode == VERBOSE)
      fprintf(stdout, MSG_SRT, file_total_count, area_total_count, outfile);
    if (ls->listflag != ' ') {          /* limited by period */
      psort(dm, 0, file_total_count-1, sort_new);
      cn = NULL;                        /* no newest assigned yet */
      n = rpt_count(dm, ls, &cn);       /* first 'n' to be proc'd */
      if (n>0 && oper_mode == VERBOSE)
        fprintf(stdout, MSG_RST, n);  /* re-sort msg */
      }
    else                                /* really all files */
      n = file_total_count;             /* set list-count */
    if (n > 0) {
      switch(ls->sortflag) {
        case ALPHA:
        case GROUP:     psort(dm, 0, n-1, sort_all); break;
        case TIMESTAMP: psort(dm, 0, n-1, sort_al2); break;
        case KEEPSEQ:   psort(dm, 0, n-1, sort_akp); break;
        default: break;
        }
      }
    fc = preproc_area(area, dm, ls);
    if (oper_mode!=QUIET)
      fprintf(stdout, MSG_REP, outfile);
    if (oper_mode==VERBOSE)
      fprintf(stdout, MSG_REC);
    insert_title(pf, pre_title, 0);
    block_title(pf, 20, EMPTY, list_title, ls);
    file_incl(pf, ls);                  /* insert user-'logo' */
    insert_title(pf, sub_title, 0);
    akb = (count_bytes(area) + 512)/1024;    /* KBytes total */
    if (ls->listflag != ' ')            /* limited by period */
      fprintf(pf,"\n(%s) Last %hu %snewest of a total of %lu files (%lu %cB)\n",
                   sys_date(today),
                  (ls->listflag!=' ' || fc>ls->max_fil) ?
                         ls->max_fil : (USHORT)fc,
                  (ls->listflag==' ') ? EMPTY :
                   ((ls->listflag=='D') ? DAYS :
                    ((ls->listflag=='W') ? WEEKS : MONTHS)),
                   fc,
                   (akb < 9999) ? akb : (akb-512)/1024,
                   (akb < 9999) ? 'K' : 'M');
    else
      fprintf(pf,"\n(%s) Available: %lu files in %hu areas (%lu %cB)\n",
                 sys_date(today),
                 fc,
                 count_areas(area, ls->priv),
                 (akb < 9999) ? akb : (akb + 512)/1024,
                 (akb < 9999) ? 'K' : 'M');
    if (ls->exclflag != EXCLPRIV)
      fprintf(pf,"%19s%s%s\n", EMPTY, MP, priv_name[ls->priv-TWIT]);
    fprintf(pf,"%19s%s %c%s, %c%s\n",
               EMPTY, DF, DAYS_7, WK, DAYS_30, MO);
    ac[0] = '\0';                       /* null area name */
    j = 0;                              /* count listed files */
    for (i=0; i<n; i++) {               /* all files to be listed */
      fx = dm[i];                       /* copy pointer to file info */
      if (rpt_coll(fx, ls, TRUE)) {     /* check if to be listed */
        if (strcmp(ac,fx->parea->name)) {  /* new area */
          strcpy(ac, fx->parea->name);  /* store new areaname */
          if (fx->parea->file_count > 0) { /* any listable files in area */
            fprintf(pf,"\n\n");
            sep_line(pf, 'Í', 79, 0);
            akb = (fx->parea->byte_count + 512)/1024;
            if (max_aname <= 3) {       /* short areanames */
              fprintf(pf,"%s º %-.60s\n",
                          strnblk(ac,3,ls->tfont,LINE1),fx->parea->adesc);
              fprintf(pf,"%s º Available: %lu files (%lu %cB)\n",
                        strnblk(ac,3,ls->tfont,LINE2),
                        fx->parea->file_count,
                        (akb < 9999) ? akb : (akb + 512)/1024,
                        (akb < 9999) ? 'K' : 'M');
              fprintf(pf,"%s º",
                          strnblk(ac,3,ls->tfont,LINE3));
              if (ls->exclflag != EXCLPRIV) {
                fprintf(pf," Privilege: %-.9s",
                          priv_name[fx->parea->priv-TWIT]); /* area priv */
                lock = fx->parea->filelock;       /* get lock */
                if (lock!=0) {
                  fprintf(pf,"%c",'/');   /* locks */
                  for (k=0; k<32; k++) {
                    if (lock & 0x00000001)
                      fprintf(pf,"%c",(k<8)?('1'+k):('A'+k-8));
                    lock >>= 1;
                    }
                  }
                fprintf(pf, "\n");
                }
              else
                fprintf(pf,"\n");
              fprintf(pf,"%s º",strnblk(ac,3,ls->tfont,LINE4));
              }
            else {                      /* long areanames */
              block_title(pf, 8, EMPTY, ac, ls);
              sep_line(pf, 'Ä', 79, 0);
              fprintf(pf," %-.78s\n", fx->parea->adesc);
              fprintf(pf," Available: %lu files (%lu %cB)\n",
                       fx->parea->file_count,
                       (akb < 9999) ? akb : (akb + 512)/1024,
                       (akb < 9999) ? 'K' : 'M');
              if (ls->exclflag != EXCLPRIV) {
                fprintf(pf," Privilege: %-.9s",
                          priv_name[fx->parea->priv-TWIT]); /* area priv */
                lock = fx->parea->filelock;       /* get lock */
                if (lock!=0) {
                  fprintf(pf,"/");      /* locks follow */
                  for (k=0; k<32; k++) {
                    if (lock & 0x00000001)
                      fprintf(pf,"%c",(k<8)?('1'+k):('A'+k-8));
                    lock >>= 1;
                    }
                  }
                fprintf(pf, "\n");
                }
              }
            if (fx->parea->newest != NULL  &&    /* newest file */
                fx->parea->newest->wd.idate != 0) {  /* true date */
              fprintf(pf," Newest: %s %8s ",
                          fx->parea->newest->fname,
                          f_date(fx->parea->newest->wd.date));
              fprintf(pf," (avail: %8s)",
                          f_date(fx->parea->newest->cd.date));
              }
            fprintf(pf, "\n");
            sep_line(pf, 'Ä', 79, 0);
            fprintf(pf,"%s      %s   %s    %s\n",FN,SZ,DT,DS);
            sep_line(pf, 'Ä', 12, 5, 9, 50, 0);
            }
          }
        if (fx->parea->file_count > 0) { /* any listable files in area */
          if (fx->fname[0] != '\0') {   /* filename present */
            ++j;                        /* count only file entries */
            if (oper_mode==VERBOSE && (j%25)==0) {
              fprintf(stdout, "%6lu\r", j);  /* keep SYSOP awake */
              fflush(stdout);
              }
            fprintf(pf,"%-12.12s %15s ",
                       fx->fname,
                       f_size_date(fx));
            desc_part(pf, fx->fdesc, 50, 50-ls->desc_indent, ls);
            }
          else if (ls->sortflag == KEEPSEQ) {  /* '/K' specified */
            fprintf(pf, "%-s\n",
              (strip_ava == 'Y') ? strava(fx->fdesc) : fx->fdesc);
            }
          }
        }
      }
    if (oper_mode==VERBOSE)             /* last value */
      fprintf(stdout, "%6lu\n", j);
                                        /* Area Summary report */
#ifndef __32BIT__
    area2 = (DOWNPATH huge *)halloc(area_total_count, sizeof(DOWNPATH));
#else
    area2 = (DOWNPATH *)malloc(area_total_count * sizeof(DOWNPATH));
#endif
                                        /* dup area-array for sorting! */
    if (area2 == NULL) {                /* memory not obtained? */
      if (oper_mode!=QUIET)
        fprintf(stderr, "Not enough memory for summary report, skipped.\n");
      }
    else {
      for (j=0; j<area_total_count; j++)
        memmove(&area2[j],&area[j],sizeof(DOWNPATH));
      qsort(area2, area_total_count, sizeof(DOWNPATH), sort_summ);
      if (oper_mode==VERBOSE)
        fprintf(stdout, MSG_REP, SUMM);
      fprintf(pf,"\n\n");
      sep_line(pf, 'Í', 79, 0);
      block_title(pf, 12, EMPTY, SUMM, ls);
      sep_line(pf, 'Í', 79, 0);
      fprintf(pf,"%-8s %-54s %5s %8s\n",AC,DS,FS,BY);
      sep_line(pf, 'Ä', 8, 54, 5, 9, 0);
      for (j=0; j<area_total_count; j++)
        if (area2[j].file_count) {
          fprintf(pf,"%-8.8s %-54.54s %5lu %8luK\n",
                     area2[j].name,
                     area2[j].adesc,
                     area2[j].file_count,
                     (area2[j].byte_count+512)/1024);
          }
      sep_line(pf, 'Ä', 63, 5, 9, 0);
      fprintf(pf,"%63s %5lu %8luK\n","Total:",
                 count_files(area2),
                 (count_bytes(area2)+512)/1024);
#ifndef __32BIT__
      hfree(area2);                     /* free copy of area array */
#else
      free(area2);                      /* free copy of area array */
#endif
      }

    signature(pf,today);                /* leave fingerprint */
    insert_title(pf, bot_title, 0);
    fclose(pf);                         /* finished with .ALL file */
    }
  else
    fprintf(stderr, MSG_OPO, outfile, 0);      /* open failed */
  }

