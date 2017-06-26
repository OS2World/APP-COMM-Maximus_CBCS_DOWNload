/* ============================================================= */
/*  Rob Hamerling's MAXIMUS download file scan and sort utility. */
/*  -> DOWNPAR.C                                                 */
/*  -> Parameter processing routines for DOWNSORT.               */
/* ============================================================= */

#define INCL_BASE
#define INCL_NOPMAPI
#define INCL_DOSNLS
#include <os2.h>

#include <ctype.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "..\max\mstruct.h"
#include "downsort.h"
#include "downfpro.h"

/*  Symbolic names of keyword parameters */
enum _keyword { K_CMT, K_TIT, K_DAT, K_ALL, K_NEW, K_GBL, K_BBS,
                K_FIL, K_ORP, K_PRE, K_SUB, K_BOT, K_NDS, K_ODS, K_IPF,
                K_OFF, K_AIN, K_AEX, K_DUP, K_OK,  K_IP2, K_ASQ, K_EMI,
                K_AVA, K_NDE, K_FEX, K_GRP, K_END};

/* prototypes of local functions */

void    add_ext_pair(char *, DUPCHAIN **);
void    add_file_array(char [][MAXFN], char *);
void    add_str(char *, STRCHAIN **);
void    add_title(char *, STRCHAIN **);
void    add_tok(char *, STRCHAIN **);
short int conv_priv(char [], ULONG *);
void    list_parm(short int, char *);
void    list_path(short int, char *);
short int parse_keyword(char [], char **);
void    read_cfg(char *);


/* ----------------------------------------- */
/* Process commandline and system parameters */
/* ----------------------------------------- */
void get_parm(unsigned short int k,
              char *p[])
{
  unsigned short int i;                 /* counters                      */
  LISTPARM  *ls;                        /* list specs */

#ifndef __32BIT__
  USHORT x;                             /* returned length               */
  DosGetCtryInfo(sizeof(COUNTRYINFO), &c_code, &c_info, &x);
#else
  ULONG  x;                             /* returned length               */
  DosQueryCtryInfo(sizeof(COUNTRYINFO), &c_code, &c_info, &x);
#endif
                                        /* for date format, etc          */
  sys_date(today);                      /* start timestamp               */

  for (i=1; i<k; ++i) {                 /* scan commandline              */
    if (p[i][0]=='@')                   /* config filespec               */
      strncpy(cfg_path,strupr(&p[i][1]),127); /* copy filespec           */
    else if (p[i][0] == '/' || p[i][0] == '-') { /* options              */
      switch(toupper(p[i][1])) {
        case VERBOSE   : oper_mode = VERBOSE;   break;
        case QUIET     : oper_mode = QUIET;     break;
        case HELP      : oper_mode = HELP;      break;
        case QMARK     : oper_mode = HELP;      break;
        }
      }
    }

  if (oper_mode != QUIET)
    show_welcome();                     /* display DOWNSORT 'logo' */
  if (oper_mode == HELP) {
    show_help();                        /* display help and exit */
    return;                             /* quit program */
    }

  read_cfg(cfg_path);                   /* process configuration file */

  for (i=0; i<=P_MAX; i++) {            /* all list types */
    ls = lp[i].next;                    /* pointer to list specs */
    while (ls != NULL) {                /* all list of same type */
      if (ABS_MAX_priv < ls->priv  &&   /* higher than before */
              HIDDEN+1 > ls->priv)      /* but only requested reports */
        ABS_MAX_priv = ls->priv;        /* revise if needed */
      areakeys |= ls->userkeys;         /* cumulative userkeys */
      if (keepseq!=KEEPSEQ && ls->sortflag==KEEPSEQ)
        keepseq = KEEPSEQ;              /* any KEEPSEQ: collect comments */
      ls = ls->next;                    /* next report of this type */
      }
    }

  if (oper_mode == VERBOSE)
    fprintf(stdout, MSG_PAR);           /* parameters processed */
  }

/* ------------------------------------------------------ */
/* Process first character of a privilege parameterstring */
/* Returns privilege value of first character of string.  */
/* Remainder of string will be searched for userkeys(char)*/
/* These will be returned in a 32-bits string (ULONG), if */
/* not found the bitstring will be all zeroes.            */
/* ------------------------------------------------------ */
short int conv_priv(char c[],           /* privilege string */
                    ULONG *u)           /* userkeys (to be returned) */
{
  char  cu;                             /* privilege character */
  char  *k;                             /* pointer to userkeys string */
  short int p;                          /* privilege value */

  cu = (char)toupper(c[0]);             /* get priv letter upper case */
  p = SYSOP;                            /* init to default */
  if (cu == '*')                        /* asterisk specified? */
    p = SYSOP - TWIT;                   /* default (with offset corr.) */
  else                                  /* search table */
    for (p=0; p<=HIDDEN-TWIT && priv_name[p][0] != cu; p++);  /* search */

  *u = 0L;                              /* set all keys off */
  k = strchr(c, '/');                   /* search leftmost slash */
  if (k != NULL) {                      /* found: userkeys specified */
    k = asciiz(k);                      /* make it an ASCIIZ string */
    cu = (char)toupper(*++k);           /* first key character */
    while (cu!='\0') {                  /* userkey characters */
      if (cu == '*') {                  /* all keys */
        *u = 0XFFFFFFFF;                /* returned userkey field */
        break;                          /* remaining keys unimportant */
        }
      else if (cu>='1' && cu<='8')      /* keys 1..8 */
        *u |= (1 << (cu-'1'));          /* add to userkeys */
      else if (cu>='A' && cu<='X')      /* keys A..X */
        *u |= (1 << (8 + (cu-'A')));    /* add to userkeys */
      cu = (char)toupper(*++k);         /* next key character */
      }
    }
  return(p + TWIT);                     /* return privilege value */
  }

/* =================================== */
/* Process DOWNSORT Configuration File */
/* =================================== */
void read_cfg(char ds_cfg[])
{
  FILE *prm;
  char buf[256];
  char *value;                          /* ptr to parameter value        */
  short int kwd;

  if ((prm = fopen(ds_cfg,"r")) == NULL )
    fprintf(stderr, MSG_OPI, ds_cfg, 2);  /* dummy rc 'file not found'!*/

  else {
    if (oper_mode != QUIET)
      fprintf(stdout, MSG_CFG, cfg_path);

    while (fgets(buf,sizeof(buf)-1,prm)) {  /* process 1 line at a time */

      kwd = parse_keyword(buf, &value);
      switch(kwd) {

        case K_CMT:                     /* Comment and empty lines */
          break;

        case K_TIT:                     /* block title (ALL, NEW, GBL) */
          strncpy(list_title, value, 20);
          break;

        case K_PRE:                     /* pre-title */
          add_title(value, &pre_title);
          break;

        case K_SUB:                     /* sub-title */
          add_title(value, &sub_title);
          break;

        case K_BOT:                     /* bottom line */
          add_title(value, &bot_title);
          break;

        case K_ODS:                     /* Orphan Description */
          strncpy(ORPHAN,value,45);     /* copy file text */
          break;

        case K_NDS:                     /* Missing Description */
          strncpy(NDS,value,45);        /* copy file text */
          break;

        case K_OFF:                     /* Offline Description */
          OFFLINE[0] = '\0';            /* erase default string */
          strncat(OFFLINE,value,14);    /* copy(!) text */
          strcat(OFFLINE," ");          /* 1 trailing blank */
          break;

        case K_DAT:                     /* Area.Dat */
          strncpy(areadat_path, strupr(value),80);  /* copy file spec */
          break;

        case K_AIN:                     /* AreaINclude */
          add_tok(value, &incl_area);   /* add include string  */
          break;

        case K_AEX:                     /* AreaEXclude */
          add_tok(value, &excl_area);   /* add exclude strings */
          break;

        case K_FEX:                     /* FileEXclude */
          add_str(value, &excl_file);   /* add FILE-exclude strings */
          break;

        case K_NDE:                     /* non duplicate extension pairs */
          add_ext_pair(value, &non_dup_ext);    /* add ext pairs */
          break;

        case K_ASQ:                     /* AreaSequence */
          area_seq = (char) toupper(value[0]);  /* take 1st char upperc. */
          break;

        case K_AVA:                     /* Strip AVATAR graphics */
          strip_ava = (char) toupper(value[0]);   /* set switch */
          break;

        case K_ALL:                     /* create ALL-list(s) */
          list_parm(P_ALL, value);
          break;

        case K_IPF:                     /* create IPF-list(s) */
          list_parm(P_IPF, value);
          break;

        case K_IP2:                     /* create IPF2-list(s) */
          list_parm(P_IP2, value);
          break;

        case K_BBS:                     /* create BBS-list */
          list_parm(P_BBS, value);
          break;

        case K_GBL:                     /* create GBL-list */
          list_parm(P_GBL, value);
          break;

        case K_NEW:                     /* create NEW-list(s) */
          list_parm(P_NEW,value);
          break;

        case K_EMI:                     /* create EMI-list(s) */
          list_parm(P_EMI, value);
          break;

        case K_DUP:                     /* create of DUP-list */
          list_parm(P_DUP, value);
          break;

        case K_OK:                      /* create OKfile */
          list_parm(P_OK, value);
          break;

        case K_ORP:                     /* create of ORP-list */
          list_parm(P_ORP, value);
          break;

        case K_FIL:                     /* create Files.Bbs files */
          list_path(P_FIL, value);
          break;

        case K_END:                     /* Keyword not found */
        default:                        /* undefined */
          fprintf(stderr, MSG_KWD, buf); /* keyword or value problem */
          break;
        }
      }
    fclose(prm);                        /* close downsort.cfg */
    }
  }

/* =============================== */
/* Process xxxFileList parameters. */
/* =============================== */
void list_parm(short int k,             /* list type */
               char  *v)                /* parm string */
{
  short int  t,j;                       /* intermediate int value */
  char *nv;                             /* moving string pointer */
  char c;                               /* single input character */
  LISTPARM *ls, *ols;                   /* list specs */

  ols = &lp[k];                         /* pointer to default list specs */
  while (ols->next != NULL)             /* search for last */
    ols = ols->next;                    /* get pointer to next */
  ls = (LISTPARM *)malloc(sizeof(struct _listparm));  /* mem for specs */
  if (ls == NULL) {                     /* not obtained */
              /* ===> error message */
    return;                             /* ignore this list request */
    }
  ols->next = ls;                       /* chain pointer to new specs */
  memcpy((void *)ls, (void *)&lp[k], sizeof(struct _listparm)); /* dflts */
  ls->next = NULL;                      /* current end-of-chain */

  strupr(v);                            /* whole string uppercase */
  ls->priv = conv_priv(v, &ls->userkeys);   /* determine priv/userkeys  */
  if ((nv = next_word(v)) != NULL  &&   /* 1st expected: filename */
       strcmp(nv,"*") != 0)             /* not asterisk */
    strncpy(ls->name, asciiz(nv), 8);   /* customised filename */
  while ((nv = next_word(nv)) != NULL) { /* remaining parm(s) */
    if (nv[0] == '/' || nv[0] == '-') { /* option flag */
      switch(nv[1]) {
        case TRUNC     : ls->wrapflag = TRUNC;     break;
        case WRAP      : ls->wrapflag = WRAP;      break;
        case ALPHA     : ls->sortflag = ALPHA;     break;
        case GROUP     : ls->sortflag = GROUP;     break;
        case TIMESTAMP : ls->sortflag = TIMESTAMP; break;
        case KEEPSEQ   : if (k==K_ALL || k!=K_IPF || k!=K_IP2)
                           ls->sortflag = KEEPSEQ;   /* only some lists */
                         break;
        case LONGLIST  : ls->longflag = LONGLIST;  break;
        case RUBBDATE  : ls->rubbflag = ~RUBBDATE;  break;
        case FONT      : t = atoi(nv+2);
                         if (t >= FONT0 && t <= FONT4) /* range check    */
                           ls->tfont = t;
                         break;
        case CONT      : t = atoi(nv+2);
                         ls->desc_indent = t;
                         break;
        case EXCLPRIV  : ls->exclflag = EXCLPRIV;  break;
        case INCLUDE   : ls->incl_fspec =
                                  (char *)malloc(strlen(asciiz(nv+2))+1);
                         if (ls->incl_fspec == NULL) {
                           fprintf(stderr, MSG_MEM, PROGNAME);  /* mem   */
                           exit(14);
                           }
                         else
                           strcpy(ls->incl_fspec, asciiz(nv+2));
                         break;
        default        : break;         /* invalid parm: ignored */
        }
      }
    else if (0 < atoi(nv)) {            /* numeric: 'NEW'-list length */
      ls->max_fil = atoi(nv);           /* (for IPF-list: pagesize) */
      ls->listflag = ' ';               /* set to (nullify previous) per.*/
      j = strlen(asciiz(nv)) - 1;       /* offset to last character */
      c = nv[j];                        /* last character */
      if (c == 'D' || c == 'W' || c == 'M')
        ls->listflag = c;               /* copy listflag */
      }
    }
  }

/* =============================== */
/* Process FILFilePath parameters. */
/* =============================== */
void list_path(short int k,             /* list type (should be P_FIL) */
               char  *v)                /* parm string */
{
  short int  t,j;                       /* intermediate int value        */
  char *nv;                             /* moving string pointer         */
  LISTPARM *ls, *ols;                   /* list specs                    */

  ols = &lp[k];                         /* pointer to default list specs */
  while (ols->next != NULL)             /* search for last */
    ols = ols->next;                    /* get pointer to next */
  ls = (LISTPARM *)malloc(sizeof(struct _listparm));  /* mem for specs */
  if (ls == NULL) {                     /* not obtained */
              /* ===> error message */
    return;                             /* ignore this list request */
    }
  ols->next = ls;                       /* chain pointer to new specs */
  memcpy((void *)ls, (void *)&lp[k], sizeof(struct _listparm)); /* dflts */
  ls->next = NULL;                      /* current end-of-chain */

  strupr(v);                            /* all upper case                */
  ls->priv = conv_priv(v, &ls->userkeys);
  nv = v;                               /* copy pointer                 */
  while ((nv = next_word(nv)) != NULL) {  /* remaining prms*/
    if (nv[0] == '/' || nv[0] == '-') {  /* option flag    */
      switch(nv[1]) {
        case ALPHA     : ls->sortflag = ALPHA;     break;
        case GROUP     : ls->sortflag = GROUP;     break;
        case TIMESTAMP : ls->sortflag = TIMESTAMP; break;
        case KEEPSEQ   : ls->sortflag = KEEPSEQ;   break;
        case EXCLPRIV  : ls->exclflag = EXCLPRIV;  break;
        case LONGLIST  : ls->longflag = LONGLIST;  break;
        case FONT      : t = atoi(nv+2);
                         if (t >= FONT0 && t <= FONT4) /* range check    */
                           ls->tfont = t;
                         break;
        case INCLUDE   : ls->incl_fspec =
                            (char *)malloc(strlen(asciiz(nv+2))+1);
                         if (ls->incl_fspec == NULL) {
                           fprintf(stderr,MSG_MEM,PROGNAME);
                           exit(14);
                           }
                         else
                           strcpy(ls->incl_fspec, asciiz(nv+2));
                         break;
        default        : break;
        }
      }
    else {                      /* assume 'nv' is pathname    */
      strncpy(filesbbs_path, asciiz(nv), 80); /* spec'd PATH    */
      j = strlen(filesbbs_path);
      if (j>0 && filesbbs_path[j-1] != '\\')
        filesbbs_path[j] = '\\';    /* add backslash if needed   */
      }
    }
  }

/* =============================================== */
/* Build and/or extend a chain of characterstrings */
/* Chain element consists of 2 pointers:           */
/*   - ptr to next element (or NULL in last)       */
/*   - ptr to character string                     */
/* =============================================== */
void  add_str(char *v,                  /* title string */
              STRCHAIN **h)             /* ptr to ptr to head of chain */
{
  STRCHAIN *s, *t;                      /* chain pointers */

  t = (STRCHAIN *)malloc(sizeof(struct _strchain)); /* new chain element */
  if (t == NULL) {
    fprintf(stderr,MSG_MEM, PROGNAME);  /* not enough memory          */
    exit(14);
    }
  t->next = NULL;                       /* end of chain */
  s = *h;                               /* pointer to head of chain */
  if (s != NULL) {                      /* not first lement */
    while (s->next != NULL)             /* search last entry */
      s = s->next;
    s->next = t;                        /* build chain */
    }
  else                                  /* first element */
    *h = t;                             /* start of chain */
  t->str = (char *)malloc(strlen(v)+2); /* obtain memory */
  if (t->str == NULL) {
    fprintf(stderr,MSG_MEM, PROGNAME);  /* not enough memory */
    exit(14);
    }
  else
    strcpy(t->str, v);                  /* copy string */
  }

/* ==================================== */
/* Tokenize a string (parameter line)   */
/* and add each token to a string chain */
/* ==================================== */
void  add_tok(char  *v,
              STRCHAIN **h)
{
  while (v != NULL) {                   /* all tokens in string */
    add_str(asciiz(v), h);              /* add to string */
    v = next_word(v);                   /* search next code              */
    }
  }

/* ============================================ */
/* Translate title-line and add to string chain */
/* ============================================ */
void  add_title(char  *v,
                STRCHAIN **h)
{
  int   i;
  if (v != NULL) {                   /* value present */
    for (i=0; v[i]; i++)             /* until end-of string */
      v[i] = (char)((v[i] == '~') ? ' ' : v[i]);  /* copy+translate */
    add_str(v, h);                      /* add to string */
    }
  }

/* ================================ */
/* Process Non Duplicate Extensions */
/* ================================ */
void  add_ext_pair(char *v,            /* ptr to first ext. */
                   DUPCHAIN **h)       /* ptr to ptr first element */
{
  short int j;                          /* counters */
  DUPCHAIN *s, *t;                      /* pointers */

  s = *h;                               /* pointer to head of chain */
  if (s != NULL)                        /* not first element */
    while (s->next != NULL)             /* search last entry */
      s = s->next;
  while (v != NULL) {                   /* any ext specified */
    t = (DUPCHAIN *)malloc(sizeof(struct _dupchain)); /* new chain element */
    if (t == NULL) {
      fprintf(stderr,MSG_MEM, PROGNAME);  /* not enough memory */
      exit(14);
      }
    t->next = NULL;                     /* end of chain */

    if (*h != NULL)                     /* not very first element */
      s->next = t;                      /* build chain */
    else                                /* first element */
      *h = t;                           /* start of chain */

    for (j=0; v[j] != ' ' && v[j] != '\0' && j < 3; ++j)
      t->ext1[j] = v[j];                /* copy dup-string to element */
    for ( ; j < 4; ++j)
      t->ext1[j] = '\0';                /* trailing zero's */
    v = next_word(v);                   /* next extension */
    if (v != NULL) {                    /* second of pair specified */
      for (j=0; v[j] != ' ' && v[j] != '\0' && j < 3; ++j)
        t->ext2[j] = v[j];              /* copy dup-string to element */
      v = next_word(v);                 /* next extension */
      }
    else                                /* no second of pair specified */
      j = 0;
    for ( ; j < 4; ++j)
      t->ext2[j] = '\0';                /* trailing zero's */
    s = t;                              /* shift 1 element */
    }
  }

/* ===================================== */
/* Find the number of the config options */
/* Returns the keyword symbolic number,  */
/* and pointer to the keyword value.     */
/* ===================================== */
short int parse_keyword(char line[],
                        char **value)
{
  short int i,p;                        /* counters                      */

  static struct parse_config {          /* Keyword Parameter Spec        */
       unsigned char id;                /* parameter identifier          */
       unsigned char len;               /* keyword length                */
       char *key;                       /* keyword                       */
       } cfg[] = {                      /* table of keyword parameters   */
                                        /* NOTE: most significant first! */
           {K_ALL, 11, "AllFileList"},
           {K_DAT,  7, "AreaDat"},
           {K_AEX, 11, "AreaEXclude"},
           {K_AIN, 11, "AreaINclude"},
           {K_ASQ,  9, "AreaOrder"},
           {K_AVA,  8, "AVAstrip"},
           {K_BBS, 11, "BbsFileList"},
           {K_BOT, 10, "BottomLine"},
           {K_DUP, 11, "DupFileList"},
           {K_EMI, 11, "EmiFileList"},
           {K_FEX, 11, "FileEXclude"},
           {K_FIL, 11, "FilFilePath"},
           {K_GBL, 11, "GblFileList"},
           {K_IPF, 11, "IpfFileList"},
           {K_IP2, 11, "Ip2FileList"},
           {K_NEW, 11, "NewFileList"},
           {K_NDE,  9, "NonDupEXT"},
           {K_OK,  10, "OKFileList"},
           {K_NDS, 12, "NotFoundDesc"},
           {K_OFF, 11, "OfflineDesc"},
           {K_ORP, 11, "OrpFileList"},
           {K_ODS, 10, "OrphanDesc"},
           {K_PRE,  8, "PreTitle"},
           {K_SUB,  8, "SubTitle"},
           {K_TIT,  5, "Title"},
           {K_GRP,  9, "Areagroup"},
           {K_END,  0, ""},             /* end of table: keyw. not found */
             };

  *value = NULL;                        /* init: default return          */

  for (i=0; line[i]==' '; ++i);         /* skip leading blanks           */
  line = line+i;                        /* new local pointer             */

  if (line[0] == '%'  ||                /* comment                       */
      line[0] == ';'  ||                /* comment                       */
      line[0] == '*'  ||                /* comment                       */
      line[0] == '\r' ||                /* CR character                  */
      line[0] == '\n' ||                /* LF character                  */
      line[0] == '\0')                  /* end of string                 */
    return(K_CMT);                      /* return as comment line        */

  for (i=0; cfg[i].id < K_END &&        /* search keyword                */
           strnicmp(line,cfg[i].key,cfg[i].len) != 0 ; i++);
  p = cfg[i].id;                        /* return identification value   */

  *value = next_word(line);             /* search value string           */
  if (*value == NULL)                   /* no keyword value              */
    return(K_END);                      /* treat as invalid keyword      */

  i = strlen(*value);                   /* length of value string        */
  if ((*value)[i-1] == '\n'  ||         /* line feed or                  */
      (*value)[i-1] == '\r')            /* carriage return               */
    (*value)[i-1] = '\0';               /* replaced by end of string     */

  return(p);                            /* return the keyword number     */
  }

