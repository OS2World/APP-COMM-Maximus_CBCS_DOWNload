/* ============================================================= */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility  */
/*  -> DOWNSORT.C                                                */
/*  -> Mainline                                                  */
/*                                                               */
/*  When compiled with IBM C Set/2 compiler, a 32-bit OS/2       */
/*  version will be generated (via compiler variable __32BIT__). */
/*  Note: the 'migration' option is still necessary (-Sm), and   */
/*        for Maximus' mstruct.h the -D__386__ is also needed.   */
/*  When compiled by MicroSoft C compiler 6.00a a 16-bit program */
/*  will be generated for OS/2 or DOS.                           */
/* ============================================================= */

#define INCL_BASE
#define INCL_NOPMAPI
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "..\max\mstruct.h"
#include "downsort.h"
#include "downfpro.h"

/* prototypes of local functions */

USHORT   collect_area(DOWNPATH _HUGE **);
ULONG    collect_file(unsigned int, DOWNPATH _HUGE *);
void     get_parm(int, char *[]);
FILECHAIN * _HUGE *prep_sort(ULONG, FILECHAIN *);
void  make_bbs(FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);
void  make_all(FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);
void  make_dup(FILECHAIN * _HUGE *,                   LISTPARM *);
void  make_fil(FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);
void  make_gbl(FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);
void  make_ipf(FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);
void  make_ip2(FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);
void  make_new(FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);
void  make_ok( FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);
void  make_orp(FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);
void  make_emi(FILECHAIN * _HUGE *, DOWNPATH _HUGE *, LISTPARM *);

/* ====================== */
/*   M A I N    L I N E   */
/* ====================== */
int  main(int argc, char *argv[])
{
  DOWNPATH  _HUGE *area;                /* pointer to area-info array */
  FILECHAIN * _HUGE *dm;                /* pointer to file-sort array */
  LISTPARM  *ls;                        /* pointer to list specs      */
  long     start_time,run_time;         /* for execution time meas.   */

  start_time = time(NULL);              /* system time at start       */
  sprintf(list_title,"%s%c%c%c",PROGNAME,VERSION,SUBVERS,SUFFIX);
                                        /* build default title        */
  get_parm(argc, argv);                 /* system and oper. parameters*/
                                        /* and display welcome msg    */
  area_total_count = collect_area(&area);   /* build area array       */
  if (area_total_count <= 0) {          /* no area's included         */
    printf(MSG_ZF, "-area");
    printf(MSG_ZP, PROGNAME);
    DosExit(0, 8);
    }

  if (oper_mode == VERBOSE)
    fprintf(stdout, "Collecting information from %u file-area's\n",
              area_total_count);
  file_total_count = collect_file(area_total_count, area);
  if (file_total_count == 0) {          /* no files                   */
    fprintf(stdout, MSG_ZF, "");
    fprintf(stdout, MSG_ZP, PROGNAME);
    DosExit(0, 10);
    }

  dm = prep_sort(file_total_count, first_element); /* make sort array */

  ls = lp[P_ORP].next;
  while (ls != NULL) {                                 /* ORPHAN-list */
    if (ls->priv <= HIDDEN)
      make_orp(dm, area, ls);
    ls = ls->next;
    }

  ls = lp[P_DUP].next;
  while (ls != NULL) {                                 /* DUP-list    */
    if (ls->priv <= HIDDEN)
      make_dup(dm, ls);
    ls = ls->next;
    }

  ls = lp[P_OK].next;
  while (ls != NULL) {                                 /* OKFile      */
    if (ls->priv <= HIDDEN)
      make_ok(dm, area, ls);
    ls = ls->next;
    }

  ls = lp[P_BBS].next;
  while (ls != NULL) {                                 /* BBS-list    */
    if (ls->priv <= HIDDEN)
      make_bbs(dm, area, ls);
    ls = ls->next;
    }

  ls = lp[P_NEW].next;
  while (ls != NULL) {                                 /* NEW-list    */
    if (ls->priv <= HIDDEN)
      make_new(dm, area, ls);
    ls = ls->next;
    }

  ls = lp[P_EMI].next;
  while (ls != NULL) {                                 /* EMI-list    */
    if (ls->priv <= HIDDEN)
      make_emi(dm, area, ls);
    ls = ls->next;
    }

  ls = lp[P_GBL].next;
  while (ls != NULL) {                                 /* GBL-list    */
    if (ls->priv <= HIDDEN)
      make_gbl(dm, area, ls);
    ls = ls->next;
    }

  ls = lp[P_ALL].next;
  while (ls != NULL) {                                 /* ALL-list    */
    if (ls->priv <= HIDDEN)
      make_all(dm, area, ls);
    ls = ls->next;
    }

  ls = lp[P_IPF].next;
  while (ls != NULL) {                                 /* IPF-list    */
    if (ls->priv <= HIDDEN)
      make_ipf(dm, area, ls);
    ls = ls->next;
    }

  ls = lp[P_IP2].next;
  while (ls != NULL) {                                 /* IPF2-list   */
    if (ls->priv <= HIDDEN)
      make_ip2(dm, area, ls);
    ls = ls->next;
    }

  ls = lp[P_FIL].next;                                 /* LAST!!!!!   */
  while (ls != NULL) {                                 /* FILES.BBS   */
    if (ls->priv <= HIDDEN)
      make_fil(dm, area, ls);
    ls = ls->next;
    }

  if (oper_mode != QUIET) {
    printf("\n%s version %c.%c%c by %s ",
             PROGNAME,VERSION,SUBVERS,SUFFIX,AUTHOR);
    run_time = time(NULL) - start_time;   /* execution time in seconds */
    printf("completed in %ld minutes and %ld seconds.\n\n",
          run_time/60,run_time%60);     /* report execution time */
    }
  else
    printf("\n");

  return(0);                            /* Automatic release all storage! */
  }
