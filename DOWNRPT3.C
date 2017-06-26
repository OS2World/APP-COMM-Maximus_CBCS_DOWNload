
/* =============================================================== */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility     */
/*  -> DOWNRPT3.C                                                   */
/*  -> Make SYSOP lists: ORP-list, DUP-list, OK-file, all FILES.BBS */
/* ================================================================ */

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

/* prototypes of local functions */

int  filename_cmp(FILECHAIN * _HUGE *, ULONG);
int  dup_ext(char *, char *);

/* ------------------------- */
/* Produce the ORPHAN report */
/* ------------------------- */
void make_orp(FILECHAIN * _HUGE *dm,
              DOWNPATH  _HUGE *area,
              LISTPARM  *ls)            /* list specs                    */
{
  FILE   *pf;                           /* file handle                   */
  char   outfile[MAXFN];                /* file names                    */
  ULONG  i,j,fc;                        /* counters                      */
  FILECHAIN *fx;                        /* local pointer to file info    */

  sprintf(outfile,"%s.%s",ls->name,ls->ext);  /* build fname */
  if (oper_mode==VERBOSE)
    fprintf(stdout, MSG_SRT, file_total_count, area_total_count, outfile);
  switch(ls->sortflag) {                /* sort                          */
    case ALPHA:     psort(dm,0,file_total_count-1,sort_gbl); break;
    case GROUP:     psort(dm,0,file_total_count-1,sort_all); break;
    case TIMESTAMP: psort(dm,0,file_total_count-1,sort_new); break;
    default: break;
    }

  fc = preproc_area(area, dm, ls);      /* files within priv   */
  j = file_total_count - fc;            /* calc orphans */
  if (j>0) {                            /* yes, there are orphans */
    pf = fopen(outfile,WRITE);          /* output file */
    if (pf != NULL) {                   /* successful open */
      if (oper_mode != QUIET)
        fprintf(stdout, MSG_REP, outfile);
      if (oper_mode == VERBOSE)
        fprintf(stdout, MSG_REC);
      block_title(pf, 9, EMPTY, " Orphans ", ls); /* generate block title  */
      fprintf(pf, "\n");
      sep_line(pf, 'Í', 79, 0);
      fprintf(pf,"  %s   %s       %s    %-s\n", AC, FN, DT, FP);
      sep_line(pf, 'Ä', 8, 12, 9, 47, 0);
      for (i=j=0; i<file_total_count; i++) {
        fx = dm[i];                     /* copy pointer to file info */
        if (fx->priv >= HIDDEN) {       /* report "hidden" and up */
          if (oper_mode==VERBOSE && (j%25)==0) {
            fprintf(stdout, "%6lu\r", j);
            fflush(stdout);
            }
          ++j;                          /* list file-count               */
          fx->fdesc = ORPHAN;           /* assign 'description'          */
          fprintf(pf,"%-8.8s %-12.12s %s%c ",
                      fx->parea->name,
                      fx->fname,
                      f_date(fx->wd.date),
                      file_age_ind(fx));  /*age*/
          desc_part(pf,
                       (fx->fpath==NULL) ? fx->parea->pname :
                                               fx->fpath,
                       47, 47 - ls->desc_indent, ls);
          }
        }
      if (oper_mode==VERBOSE) {
        fprintf(stdout, "%6u\n", j);   /* total reported orphans        */
        fflush(stdout);
        }
      signature(pf,today);              /* fingerprint                   */
      fclose(pf);                       /* finished with .ORP file       */
      }
    else                                /* no output possible            */
      fprintf(stderr, MSG_OPO, outfile, 0);
    }
  }

/* ------------------ */
/* Produce a DUP-list */
/* ------------------ */
void make_dup(FILECHAIN * _HUGE *dm,
              LISTPARM  *ls)            /* list specs                    */
{
  FILE   *pf;                           /* file handle                   */
  char   outfile[MAXFN];                /* file names                    */
  ULONG  i,j;                           /* counters                      */
  FILECHAIN *fx;                        /* local pointer to file info    */

  sprintf(outfile,"%s.%s", ls->name, ls->ext);
  pf = fopen(outfile,WRITE);            /* output file                   */
  if (pf != NULL) {
    if (oper_mode == VERBOSE)
      fprintf(stdout, MSG_SRT, file_total_count, area_total_count, outfile);
    psort(dm, 0, file_total_count-1, sort_gbl);  /* filename sort        */
    if (oper_mode != QUIET)
      fprintf(stdout, MSG_REP, outfile);
    if (oper_mode == VERBOSE)
      fprintf(stdout, MSG_REC);
    block_title(pf, 12, EMPTY, " Duplicates ", ls);
    if (ls->exclflag != EXCLPRIV)
      fprintf(pf,"\n%s%s\n",
                 MP, priv_name[ls->priv - TWIT]);
    fprintf(pf,"\n%s       %s    %s   %s    %s\n",FN,AC,SZ,DT,FP);
    sep_line(pf, 'Ä', 12, 8, 5, 9, 45, 0);
    for (i=j=0; i<file_total_count; i++) {
      fx = dm[i];                       /* copy pointer to file info */
      if (rpt_coll(fx, ls, TRUE)  &&    /* check if to be listed */
            !filename_cmp(dm, i)) {     /* (pseudo) equal filenames      */
        if (oper_mode==VERBOSE && (j%5)==0) {
          fprintf(stdout, "%6lu\r", j); /* display count by 5            */
          fflush(stdout);
          }
        if (fx->fname[0] != '\0') {  /* not a comment-entry           */
          j++;                          /* count duplicates              */
          fprintf(pf,"%-12.12s %-8.8s %15s ",
                  fx->fname,
                  fx->parea->name,
                  f_size_date(fx));
          desc_part(pf,
                       (fx->fpath==NULL) ? fx->parea->pname :
                                              fx->fpath,
                       41, 41 - ls->desc_indent, ls);
          }
        }
      }
    if (oper_mode==VERBOSE)
      fprintf(stdout, "%6lu\n", j);
    signature(pf,today);                /* leave fingerprint             */
    fclose(pf);                         /* finished with .DUP file       */
    }
  else
    fprintf(stderr, MSG_OPO, outfile, 0);
  }

/* ---------------------------------------------- */
/* Compare filename                               */
/* Returns 0 if equal (or to be considered equal) */
/* ---------------------------------------------- */
int  filename_cmp(FILECHAIN * _HUGE *dm,
                  ULONG k)              /* index in dm-array            */
{
  char   f1[9],f2[9];                   /* filenames                    */
  ULONG  i;                             /* index */
  int    rc;                            /* (default) rc */
  FILECHAIN *fx,*fy;                    /* local pointers to file info */

  rc = 1;                               /* assume not duplicate */
  fx = dm[k];
  non_wild_init(8, f1, fx->fname);      /* filename of current entry    */
  f1[8] = '\0';                         /* end of string                */
  if (k > 0) {                          /* not very first           */
    i = k-1;                            /* previous entry    */
    fy = dm[i];                         /* copy pointer to file info */
    non_wild_init(8, f2, fy->fname);    /* entry before              */
    f2[8] = '\0';                       /* end of string              */
    if (!stricmp(f1, f2)) {             /* equal filename found */
      rc = 0;                           /* report as 'duplicate' */
      while (!stricmp(f1, f2)) {        /* for all equal filenames */
        if (dup_ext(fx->fname, fy->fname)) {  /* check on (pseudo) unequal */
          return(1);                    /* non-duplicate -> 'unequal' */
          }
        if (i > 0) {                    /* within array bounds */
          --i;                          /* preceding entry */
          fy = dm[i];                   /* copy pointer to file info */
          non_wild_init(8, f2, fy->fname);  /* init */
          f2[8] = '\0';                 /* end of string */
          }
        else                            /* reached head of array */
          break;                        /* escape from while loop */
        }                               /* no more 'before' duplicates */
      }                                 /* maybe any next equal */
    }
  if (k < file_total_count-1) {         /* not very last           */
    i = k+1;                            /* next entry */
    fy = dm[i];                         /* copy pointer to file info */
    non_wild_init(8, f2, fy->fname);    /* entry before              */
    f2[8] = '\0';                       /* end of string              */
    if (!stricmp(f1,f2)) {              /* next filename equal */
      rc = 0;                           /* an equal found */
      while (!stricmp(f1, f2)) {        /* for all equal filenames */
        if (dup_ext(fx->fname, fy->fname)) {  /* check on (pseudo) unequal */
          return(1);                    /* non-duplicate -> unequal */
          }
        if (i < file_total_count-1) {   /* within array bounds */
          ++i;                          /* next consecutive entry */
          fy = dm[i];                   /* copy pointer to file info */
          non_wild_init(8, f2, fy->fname);  /* next entry */
          f2[8] = '\0';                 /* end of string */
          }
        else                            /* reached tail of array */
          break;                        /* escape from while-loop */
        }                               /* no more 'after' duplicates */
      }                                 /* no next equal filename */
    }
  return(rc);                           /* 1 = not any equal filename */
  }

/* ----------------------------------------------------- */
/* Test for files to be considered NOT duplicates:       */
/* To be called when filenames are equal if the pair of  */
/* extension is specified in the NON_DUP_EXT table.      */
/* If the pair is in the table the files will not be     */
/* reported as duplicate -> return(1) here.              */
/* Otherwise return(0) -> consider as equal files        */
/* ----------------------------------------------------- */
int  dup_ext(char *f1, char *f2)        /* pointers to filenames        */
{
  char *p1, *p2;                        /* pointers to ext */
  DUPCHAIN *t;                          /* pointer to non-dup ext chain */

  p1 = strrchr(f1, '.');                /* offset dot in fn 1 */
  p2 = strrchr(f2, '.');                /* offset dot in fn 2 */
  if (p1!=NULL && p2!=NULL) {           /* both have an extension */
    ++p1; ++p2;                         /* 1st char after dots */
    if (stricmp(p1, p2)) {              /* extensions are not equal */
      t = non_dup_ext;                  /* pointer to chain */
      if (t != NULL) {                  /* any non-dup ext present */
        do {
          if (!stricmp(p1, t->ext1)  &&  /* first equal to first */
              !stricmp(p2, t->ext2))    /* second to second */
            return(1);                  /* considered unequal*/
          else if (!stricmp(p1, t->ext2)  &&  /* first to second */
             !stricmp(p2, t->ext1))     /* second to first */
            return(1);                  /* considered unequal */
          t = t->next;                  /* next element */
          } while (t != NULL);
        }
      }
    }
  return(0);                            /* files (pseudo) equal */
  }

/* ---------------------------------------------------------------- */
/* Produce the FILES.BBS files for all area's                       */
/* Sort on name within priv-group, date or as in input FILES.BBS    */
/* Call them FILESBBS.xx  (where 'xx' is a 2-character area-name).  */
/* Put them in the directory indicated by AREA.DAT for 'listfile'.  */
/* ---------------------------------------------------------------- */
void make_fil(FILECHAIN * _HUGE *dm,
              DOWNPATH  _HUGE *area,
              LISTPARM  *ls)
{
  FILE   *pf;                           /* file handle */
  char   outfile[MAXPATH];              /* file spec new FILES.bbs */
  char   oldfile[MAXPATH];              /* file spec old FILES.bbs */
  char   ac[40];                        /* area name */
  ULONG  i;                             /* counters */
  ULONG  akb;                           /* bytecount in KB */
  USHORT j,m;                           /* counter(s) */
  int    c_priv;                        /* privilege */
  FILECHAIN *fx;                        /* local pointer to file info */

  if (oper_mode == VERBOSE)
    fprintf(stdout, MSG_SRT, file_total_count, area_total_count,
             "FILES.BBS-files");

  switch(ls->sortflag) {
    case ALPHA:
    case GROUP:     psort(dm,0,file_total_count-1,sort_fil);  break;
    case TIMESTAMP: psort(dm,0,file_total_count-1,sort_al2);  break;
    case KEEPSEQ:   psort(dm,0,file_total_count-1,sort_akp);  break;
    default: break;
    }

  if (oper_mode != QUIET)
    fprintf(stdout, MSG_REP, "new FILES.BBS files");
  preproc_area(area, dm, ls);           /* count files, bytes  */

  pf = NULL;                            /* no file open yet */
  ac[0] = '\0';                         /* init with null-string */
  for (i=0; i<file_total_count; i++) {
    fx = dm[i];                         /* copy pointer */
    if (strcmp(ac, fx->parea->name)) {  /* new area group */
      if (pf != NULL)                   /* end of previous group */
        fclose(pf);                     /* finished */
      strcpy(ac, fx->parea->name);      /* new area */
      c_priv = fx->parea->priv;         /* new AREA-priv */
                                        /* generate new "FILES.BBS" */
      if (strlen(filesbbs_path) > 0) {  /* path parameter specified  */
        strcpy(outfile,filesbbs_path);  /* copy path */
        if (max_aname <= 3) {           /* short areanames */
          strcat(outfile,ls->name);     /* standard filename */
          strcat(outfile,DOT);          /* separator */
          strncat(outfile, ac, 3);      /* extension: 3 chars areaname   */
          }
        else {                          /* long areanames */
          strcat(outfile,ac);           /* filename: areaname */
          strcat(outfile,DOT);          /* separator */
          strcat(outfile, ls->ext);     /* standard extension */
          }
        }
      else if (strlen(fx->parea->filesbbs) > 0) { /* "ListFile" spec */
        strcpy(outfile,fx->parea->filesbbs);
        strcpy(oldfile,outfile);        /* backup file */
        for (j=strlen(oldfile), m=1;
                 (j-m)>0 && m<5 && outfile[j-m]!='.'; ++m); /* search '.'*/
        if (m>=5 || (j-m)<=0)           /* no extension found: */
          m=0;                          /* concat to end of name */
        strcpy(oldfile+j-m,DOT);        /* add separator */
        strcat(oldfile,BAK);            /* backup file extension */
        unlink(oldfile);                /* erase old backup file */
        rename(outfile,oldfile);        /* rename current to backup */
        }
      else {                            /* default directory */
        strcpy(outfile,fx->parea->pname);  /* path to download dir */
        strcat(outfile,ls->name);       /* add filename */
        strcat(outfile,DOT);            /* add separator */
        strcpy(oldfile,outfile);        /* backup file */
        strcat(oldfile,BAK);            /* backup file extension */
        strcat(outfile,ls->ext);        /* add BBS-extension */
        unlink(oldfile);                /* erase old backup file */
        rename(outfile,oldfile);        /* rename current to backup */
        }

      if (oper_mode == VERBOSE)         /* progress reporting */
        fprintf(stdout, MSG_REP, outfile);
      else if (oper_mode != QUIET) {
        fprintf(stdout, DOT);
        fflush(stdout);
        }
      pf = fopen(outfile,WRITE);
      if (pf != NULL) {
        akb = (fx->parea->byte_count + 512)/1024;
        if (max_aname <= 3) {           /* short areanames */
          fprintf(pf,"%s\f\n%s%s º %-.*s\n",
                    FILPREFX, FILPREFX, strnblk(ac,3,ls->tfont,LINE1),
                    79-3-strlen(strnblk(ac,3,ls->tfont,LINE1)),
                    fx->parea->adesc);
          fprintf(pf,"%s%s º Available: %lu files (%lu %cB)\n",
                    FILPREFX,strnblk(ac,3,ls->tfont,LINE2),
                    fx->parea->file_count,         /* area filecount  */
                    (akb < 9999) ? akb : (akb + 512)/1024,
                    (akb < 9999) ? 'K' : 'M');
          fprintf(pf,"%s%s º",
                      FILPREFX, strnblk(ac,3,ls->tfont,LINE3));
          if (ls->exclflag != EXCLPRIV)
            fprintf(pf," Privilege: %-.9s",
                      priv_name[fx->parea->priv-TWIT]);  /* area priv */
          fprintf(pf,"\n");
          fprintf(pf,"%s%s º ",
                      FILPREFX, strnblk(ac,3,ls->tfont,LINE4));
          if (fx->parea->newest != NULL  &&    /* newest file */
              fx->parea->newest->wd.idate != 0) { /* not null-date */
            fprintf(pf,"Newest: %s %8s",
                      fx->parea->newest->fname,
                      f_date(fx->parea->newest->wd.date));
            fprintf(pf," (avail: %8s)",
                      f_date(fx->parea->newest->cd.date));
            }
          fprintf(pf,"\n");
          }
        else {                          /* long areanames */
          fprintf(pf,"%s\f\n", FILPREFX);
          block_title(pf, 8, FILPREFX, ac, ls);
          fprintf(pf,"%s", FILPREFX);  /* start separator line */
          sep_line(pf, 'Ä', 78, 0);
          fprintf(pf,"%s %-.77s\n", FILPREFX, fx->parea->adesc);
          fprintf(pf,"%s Available: %lu files (%lu %cB)\n",
                      FILPREFX, fx->parea->file_count,
                      (akb < 9999) ? akb : (akb + 512)/1024,
                      (akb < 9999) ? 'K' : 'M');
          if (ls->exclflag != EXCLPRIV)
            fprintf(pf,"%s Privilege: %-.9s\n",
                      FILPREFX, priv_name[fx->parea->priv-TWIT]);
          if (fx->parea->newest != NULL  &&     /* newest present */
              fx->parea->newest->wd.idate != 0) { /* not null-date */
            fprintf(pf,"%s Newest: %s %8s",
                      FILPREFX, fx->parea->newest->fname,
                      f_date(fx->parea->newest->wd.date));
            fprintf(pf," (avail: %8s)\n",
                      f_date(fx->parea->newest->cd.date));
            }
          }
        fprintf(pf,"%s", FILPREFX);
        sep_line(pf, 'Ä', 78, 0);
        file_incl(pf, ls);              /* insert user-'logo' */
        fprintf(pf,"%s%s      %s    %s     %s\n%s",
                    FILPREFX, FN, SZ, DT, DS, FILPREFX);
        sep_line(pf, 'Ä', 11, 7, 9, 47, 0);
        }
      else                              /* failed to open new FILES.BBS  */
        fprintf(stderr, MSG_OPO, outfile, 3); /* ??? */
      }                                 /* endif */
    if (pf != NULL) {                   /* check for open file */
      if (fx->priv <= ls->priv) {       /* specified reporting lvl */
        if (fx->priv > c_priv) {        /* higher priv group within area */
          c_priv = fx->priv;            /* set new */
          fprintf(pf,"%c%c\n", '\20',
                     (c_priv>SYSOP) ? 'S' : priv_name[c_priv-TWIT][0]);
          }
        if (fx->fname[0] != '\0') {     /* filename present */
          if (fx->fpath != NULL)        /* explicit pathspec */
            fprintf(pf,"%s%s ", fx->fpath,  /* path */
                                fx->fname); /* filename */
          else
            fprintf(pf,"%-12.12s", fx->fname); /* filename */
          if (ls->longflag==LONGLIST)   /* 'long' format req'd */
            fprintf(pf,"  %15s",        /* file size + date  */
                        f_size_date(fx));
          if (fx->dl_b==1 || fx->dl_t==1) {  /* download flags */
            fprintf(pf," /");
            if (fx->dl_b==1)            /* free bytes flag */
              fprintf(pf,"b");
            if (fx->dl_t==1)            /* free time flag */
              fprintf(pf,"t");
            }
          fprintf(pf," %-s\n", fx->fdesc);  /* description as 1 piece */
          }
        else if(ls->sortflag == KEEPSEQ)  /* comment, '/K' spec'd  */
          fprintf(pf,"%-s\n", fx->fdesc);
        }
      }
    }
  if (pf != NULL)                       /* end of last FILES.BBS         */
    fclose(pf);                         /* finished with FILES.bbs file  */
  }                                     /* end */

/* ------------------------------ */
/* Produce the BinkleyTerm OKFile */
/* (area's in downpath sequence!) */
/* ------------------------------ */
void make_ok(FILECHAIN * _HUGE *dm,
             DOWNPATH  _HUGE *area,
             LISTPARM  *ls)
{
  FILE   *pf;                           /* file handle */
  char   outfile[MAXFN];                /* file names */
  ULONG  i;                             /* file counters */
  USHORT j,k;                           /* area counters */
  FILECHAIN *fx;                        /* local pointer to file info */

  sprintf(outfile,"%s.%s%c",
          ls->name,
          ls->ext,
          priv_name[ls->priv-TWIT][0]);
  pf = fopen(outfile,WRITE);            /* output file */
  if (pf != NULL) {
    if (oper_mode != QUIET)
      fprintf(stdout, MSG_REP, outfile);
    file_incl(pf, ls);                  /* insert magic filenames */
    preproc_area(area, dm, ls);         /* count files, bytes */
    if (ls->longflag == LONGLIST) {     /* LONG list requested */
      for (i=0; i<file_total_count; i++) {     /* all files in chain */
        fx = dm[i];                     /* copy file pointer */
        if (rpt_coll(fx, ls, TRUE))     /* check if to be listed */
          fprintf(pf, "@%s %s%s\n",
                      fx->fname,
                      (fx->fpath==NULL) ? fx->parea->pname :
                                              fx->fpath,
                      fx->fname);
        }
      }
    else {                              /* short OK-list requested */
      for (j=k=0; j<area_total_count; j++) { /* all area's in array */
        if (area[j].priv <= ls->priv &&      /* area within privilege*/
            area[j].file_count > 0) {            /* and at least 1 file  */
          if (j<1 || stricmp(area[j].pname,area[k].pname)) { /* not yet */
            fprintf(pf, "%s*.*\n", area[j].pname);
            k = j;                      /* index of last 'printed' */
            }
          }
        }
      }
    fclose(pf);                         /* finished with .ALL file */
    }
  else
    fprintf(stderr, MSG_OPO, outfile, 0);      /* open failed */
  }

