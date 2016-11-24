/*
 *  itpl2dirtree.c
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <expat.h>
#include "itpl2dirtree.h"

FILE *dfs = NULL;  // stderr or /dev/null fd

int o_dry = 0;
int o_check = 0;    // track にあるが,  playlist にない
int o_verbose = 0;
int o_debug = 0;
char *o_path = "";
char *o_rmprefix = "";
int org_rmprefixlen = 0;
int rmprefixlen = 0;
int checkrm = 0;

const char * const PLDIR = "playlist";
const char * const PLFILE = "999.m3u";

/* #define COUNT */

struct _tstr ttIdStr[_tt_last + 1] = {
  /* tag name */
  {t_key,     "key"},
  {t_integer, "integer"},
  {t_string,  "string"},
  {t_date,    "date"},
  {t_data,    "data"},
  {t_true,    "true"},
  {t_false,   "false"},

  {t_plist,   "plist"},
  {t_dict,    "dict"},
  {t_array,   "array"},
  {t_top,     "top"},

  {t_val,     "val"},
  {t_bool,    "bool"},
  {t_none,    "none"},

  /* key name */
  {t_tracks,     "Tracks"},       // dict
  {t_kind,       "Kind"},         // string
  {t_location,   "Location"},     // string
  {t_album,      "Album"},        // string
  {t_name,       "Name"},         // string
  {t_artist,     "Artist"},       // string
  {t_comments,   "Comments"},     // string
  {t_diskn,      "Disc Number"},  // integer
  {t_diskc,      "Disc Count"},   // integer
  {t_trackn,     "Track Number"}, // integer
  {t_trackc,     "Track Count"},  // integer
  {t_trackid,    "Track ID"},     // integer
  {t_totaltime,  "Total Time"},   // integer
  {t_samplerate, "Sample Rate"},  // integer
  {t_disabled,   "Disabled"},     // bool

  {t_master,   "Master"},       // bool
  {t_dkind,    "Distinguished Kind"},     // integer
  {t_folder,   "Folder"},       // bool
  {t_pid,      "Playlist Persistent ID"}, // string
  {t_ppid,     "Parent Persistent ID"},   // string

  /* key contents */
  {t_playlists, "Playlists"},          // array
  {t_playlistitems, "Playlist Items"}, // array

  {_tt_last,   ""},
};
const char *ttStr[_tt_last + 1];

struct al_hash_t *ttHash;     // tag/key string -> enum _tt
struct al_hash_t *trackHash;  // Track Id str   -> struct _track *
struct al_hash_t *ntrackHash; // Track Id str   -> count
struct al_hash_t *folderHash; // folder pid -> name

// 状態を表わす
int st_tracks = 0;    /* 0: no track, 1: key is tracks, 2: get track elements */
int st_playlists = 0; /* 0: no pl,    1: playlists,  2: playlist items */

struct _ud ud;   // user data for expat call back

void
dump(struct _ud *ud, const char *msg0, const char *msg1)
{
  int sp = ud->sp;
  struct _dstack *dp = &ud->dstack[sp];
  fprintf(stderr, "sp %d kind %s keystr '%s' %s(%s)\n",
	  sp, ttStr[dp->kind], dp->keystr, msg0, msg1);
}

char
deesc2(const char *cp) {
  return (hexint(cp[0]) << 4) | hexint(cp[1]);
}

int
numberp(const char *cp)
{
  if (!*cp) return 0;
  while (*cp) if (!isdigit(*cp++)) return 0;
  return 1;
}

char *
eschy(const char *str, char *esc)
{
  const char *cp = str;
  char *retp = (char *)calloc(strlen(str) * 2 + 1, sizeof(char));
  char *rp = retp;
  if (!rp) {
    fprintf(stderr, "eschy malloc failed\n");
    exit(1);
  }
  while (*cp) {
    if (strchr(esc, *cp)) {
      *rp++ = '\\';
    }
    *rp++ = *cp++;
  }
  *rp = '\0';
  return retp;
}

void
replace_sl(char *str) {
  char *sp;
  while ((sp = strchr(str, '/')) != NULL) {
    *sp++ = '_';
  }
}

void
clear_track(struct _track *tp) {
  if (tp->kind)      { free(tp->kind);     tp->kind     = NULL; }
  if (tp->name)      { free(tp->name);     tp->name     = NULL; }
  if (tp->artist)    { free(tp->artist);   tp->artist   = NULL; }
  if (tp->comments)  { free(tp->comments); tp->comments = NULL; }
  if (tp->album)     { free(tp->album);    tp->album    = NULL; }
  if (tp->loc)       { free(tp->loc);      tp->loc      = NULL; }
  if (tp->trackid)   { free(tp->trackid);  tp->trackid  = NULL; }
  if (tp->pid)       { free(tp->pid);      tp->pid      = NULL; }
  if (tp->ppid)      { free(tp->ppid);     tp->ppid     = NULL; }
}

void
clear_track_ent(void *tp) {
  clear_track((struct _track *)tp);
  free(tp);
}

void dir(const char *name)
{
  char buf[BUFSIZE];

  if (o_dry) {
    if (o_verbose)
      fprintf(stderr, "dry: mkdir %s/%s\n", PLDIR, name);
    return;
  }

  snprintf(buf, sizeof(buf), "%s/%s", PLDIR, name);
  struct stat st;
  if (stat(PLDIR, &st) < 0) {
    mkdir(PLDIR, 0777);
  }
  if (mkdir(buf, 0777) < 0 && errno != EEXIST) {
    fprintf(stderr, "mkdir failed %d buf '%s'\n", errno, buf);
    perror("  mkdir");
  }
}

/* m3u 形式の playlist をそのディレクトリに作成する. 効果はなかった */
FILE *
mkm3u(const char *name)
{
  char buf[BUFSIZE];
  FILE *fp = NULL;

  if (o_verbose) fprintf(stderr, "mkm3u %s/%s/%s\n", PLDIR, name, PLFILE);
  if (o_dry) return fp;

  snprintf(buf, sizeof(buf), "%s/%s/%s", PLDIR, name, PLFILE);
  fp = fopen(buf, "w");
  if (fp) {
    fprintf(fp, "#EXTM3U\n\n");
  } else {
    fprintf(stderr, "cannot create m3u %s\n", buf);
  }
  return fp;
}

void
command(const char *folder, const char *pname, int seq, struct _track *rp)
{
  const char *loc = rp->loc;

  char path1[BUFSIZE]; // is the string used in creating the symbolic link
  char path2[BUFSIZE]; // is the name of the file created
  char *lc = strdup(loc);
  char *sp = lc;
  char *ep = lc + rmprefixlen; // skip prefix
  
  if (checkrm == 0) {
    if (strncmp(lc, o_rmprefix, org_rmprefixlen) != 0) {
      fprintf(stderr, "prefix string (-i option) '%s' is not prefix of Location '%s'\n",
	      o_rmprefix, lc);
      free(lc);
      exit(1);
    }
    checkrm++; // check once
  }

  while (*ep) {
    if (*ep == '%') {  // %XY -> char(0xXY)
      *sp++ = deesc2(++ep);
      ep += 2;
    } else {
      *sp++ = *ep++;
    }
  }
  *sp = '\0';
  
  char *sl = strrchr(lc, '/') + 1;
  snprintf(path1, sizeof(path1), "%s/%s", o_path, lc);
  snprintf(path2, sizeof(path2), "%s/%s/%s/%03d_%s", PLDIR, folder, pname, seq, sl);
  if (o_verbose) {
    fprintf(stderr, "path1 (contents) '%s'\n", path1);
    fprintf(stderr, "path2 (file    ) '%s'\n", path2);
  }
  
  free(lc);

  int e_path1 = 1;
  struct stat st;
  if (stat(path1, &st) < 0 && o_dry < 2) {
    fprintf(stderr, "stat no target '%s'\n", path1);
    e_path1 = 0;
  }

  if (o_dry) {
    return;
  }

  if (symlink(path1, path2) < 0 && errno != EEXIST) {
    fprintf(stderr, "symlink %d '%s' '%s'\n", errno, path2, path1);
    perror("symlink");
  }

#if defined(__linux__)
  if (e_path1)   {
    struct timespec ts[2]; // access, modified
    ts[0].tv_sec = st.st_atime;
    ts[1].tv_sec = st.st_mtime;
    if (utimensat(AT_FDCWD, path2, ts, AT_SYMLINK_NOFOLLOW) < 0) {
      fprintf(stderr, "utimensat errno %d path2 '%s'\n", errno, path2);
      perror("utimensat");
    }
  }
#else
  if (e_path1) {
    struct timeval times[2]; // access, modified
    times[0].tv_sec = st.st_atime;
    times[1].tv_sec = st.st_mtime;

    if (lutimes(path2, times) < 0) {
      fprintf(stderr, "lutimes errno %d path2 '%s'\n", errno, path2);
      perror("lutimes");
    }
  }
#endif
}

void
end_dict(struct _ud *udp)
{
  struct _dstack *dp = &ud.dstack[udp->sp];
  struct _track *tp = &dp->track;

  value_t dkey = -1;
  item_get(ttHash, dp->keystr, &dkey);

  udp->sp--;
  dp = &ud.dstack[udp->sp];

  fprintf(dfs, "%d dict end key %s\n", udp->sp, dp->keystr);

  if (st_tracks == 2) {
    if (0 < tp->samplerate) {
      fprintf(dfs, "isp key '%s' %d %d %d %d '%s' '%s' '%s' '%s'",
              dp->keystr, tp->diskn, tp->diskc, tp->trackn, tp->trackc,
              tp->kind, tp->name, tp->artist, tp->album);
      if (tp->comments) {
	fprintf(dfs, " '%s'", tp->comments);
      }
      fprintf(dfs, "\n");
      if (!tp->loc) {
        fprintf(stderr, "null loc2 %s %s\n", tp->name, tp->album);
      }

      int ret = item_set_pointer(trackHash, dp->keystr, (void *)tp, sizeof(struct _track));
      if (ret) fprintf(stderr, "item_set_pointer ret %d\n", ret);

      if (o_check) {
	ret = item_inc_init(ntrackHash, dp->keystr, (value_t)1, NULL);
	if (ret < 0) fprintf(stderr, "ntrackHash inc %d\n", ret);
      }
    } else {
      clear_track(tp);
    }
  }
  if (st_playlists == 1) {
    fprintf(dfs, "end_dict st_playlists 1 name %s\n", tp->name);
    clear_track(tp);
  } else if (st_playlists == 2) {
    struct _dstack *ap = &ud.dstack[udp->sp - 1];
    struct _track  *atp = &ap->track;
    fprintf(dfs, "%d end_dict st_playlists 2 id %s skip %d\n",
            udp->sp, tp->trackid, atp->skip);
    if (!atp->skip) {
      struct _track *rp;
      int ret = item_get_pointer(trackHash, tp->trackid, (void *)&rp);
      if (ret != 0) fprintf(stderr, "item_get_pointer trackHash ret %d\n", ret);

      if (o_check) {
	ret = item_inc_init(ntrackHash, tp->trackid, (value_t)1, NULL);
	if (ret < 0) fprintf(stderr, "ntrackHash inc %d\n", ret);
      }

      if (!rp->disabled) {
        if (!rp->loc) {
	  // ファイル位置がない track が存在する
          fprintf(stderr, "null loc %s %s\n", atp->name, rp->name);
	} else if (atp->ppid && atp->ppid[0] != '\0') {
	  cstr_value_t fp = NULL;
	  item_get_str(folderHash, atp->ppid, &fp);
	  command(fp, atp->name, atp->plseq++, rp);
	} else {
	  fprintf(stderr, "no parent %s\n", atp->name);
	}
      }
    }
    clear_track(tp);
  }

  if (0 < st_tracks) --st_tracks;
  dp->keystr[0] = '\0';
}

static void XMLCALL
element_start(void *userData, const XML_Char *tag_name, const XML_Char *atts[])
{
  struct _ud *udp = (struct _ud *)userData;
  // dump(udp, "elment_start", tag_name);

  udp->valid = 1;

  value_t nt = -1;
  int ret = item_get(ttHash, tag_name, &nt);
  if (ret != 0) fprintf(stderr, "new tag name %s\n", tag_name);

  struct _dstack *dp = &udp->dstack[udp->sp];
  switch(nt) {
  case t_dict: // start
    {
      value_t dkey = -1;
      item_get(ttHash, dp->keystr, &dkey);

      fprintf(dfs, "%d dict start st_tracks %d pl %d key '%s'\n",
              udp->sp, st_tracks, st_playlists, dp->keystr);

      if (dkey == t_tracks) {
        st_tracks = 1;
      } else if (st_tracks == 1 && numberp(dp->keystr)) {
        st_tracks = 2;
      } else {
        // no state change
      }

      if (STACKSIZE == ++udp->sp) {
        fprintf(stderr, "stack overflow\n");
        exit(1);
      }

      dp = &udp->dstack[udp->sp];
      dp->kind = nt;
      dp->next = t_none;
      if (st_tracks == 2 || st_playlists)
        fprintf(dfs, "stack %d clear track\n", udp->sp);
        bzero(&dp->track, sizeof(struct _track));
    }
    break;
  case t_array: // start
    {
      value_t dkey = -1;
      item_get(ttHash, dp->keystr, &dkey);

      fprintf(dfs, "%d %s start key '%s'\n", udp->sp, ttStr[nt], dp->keystr);

      if (dkey == t_playlists) {
        st_playlists = 1;
      } else if (st_playlists == 1 && dkey == t_playlistitems) {
        struct _track *tp = &dp->track;

        tp->skip = tp->master || tp->dkind || tp->folder || tp->ppid == NULL;

        if (!tp->skip) {
          fprintf(dfs, "array start stack %d name '%s' master %d dkind %d folder %d pid %s ppid %s\n",
                  udp->sp, tp->name, tp->master, tp->dkind, tp->folder, tp->pid, tp->ppid);

	  replace_sl(tp->name);
          if (tp->ppid && tp->ppid[0] != '\0') {
            char fbuf[BUFSIZE];
            cstr_value_t fp = NULL;
            item_get_str(folderHash, tp->ppid, &fp);
            snprintf(fbuf, sizeof(fbuf), "%s/%s", fp, tp->name);
            dir(fbuf);
          }
        }
        if (tp->folder) {
          char buf[BUFSIZE];
          if (tp->ppid && tp->ppid[0] != '\0') {
            cstr_value_t pstr = NULL;
            int ret = item_get_str(folderHash, tp->ppid, &pstr);
            if (ret == 0) {
              snprintf(buf, sizeof(buf), "%s/%s", pstr, tp->name);
            } else {
              snprintf(buf, sizeof(buf), "%s", tp->name);
            }
          } else {
            snprintf(buf, sizeof(buf), "%s", tp->name);
          }
          if (!tp->master && !tp->dkind) {
            dir(buf);
          }

          int ret = item_set_str(folderHash, tp->pid, buf);
          if (ret < 0) fprintf(stderr, "item_set_str %d pid %s\n", ret, tp->pid);
        }

        st_playlists = 2;
      }

      if (STACKSIZE == ++udp->sp) {
        fprintf(stderr, "stack overflow\n");
        exit(1);
      }
      dp = &udp->dstack[udp->sp];
      dp->kind = nt;
      dp->next = t_none;
    }
    break;

  case t_key: // start
    dp->next = t_key;
    dp->keystr[0] = '\0';
    break;

  case t_integer: // start
  case t_string:  // start
  case t_date:    // start
    dp->next = t_val;
    dp->valstr[0] = '\0';
    break;

  case t_true:   // start
  case t_false:  // start
    dp->next = t_bool;
    dp->valstr[0] = '\0';
    break;

  case t_plist:
  default:
    dp->next = t_none;
    break;
  }

#ifdef COUNTX
  ret = item_inc_init(udp->hp, name, (value_t)1, NULL);
  if (ret < 0) fprintf(stderr, "inc %d\n", ret);
#endif
}

static void XMLCALL
element_end(void *userData, const XML_Char *name)
{
  struct _ud *udp = (struct _ud *)userData;
  udp->valid = 0;

  value_t nt = -1;
  int ret = item_get(ttHash, name, &nt);
  if (ret != 0) fprintf(stderr, "new tag name %s\n", name);

  struct _dstack *dp = &udp->dstack[udp->sp];

  value_t dkey = -1;
  if (st_tracks == 2 || st_playlists == 1 || st_playlists == 2) {
    item_get(ttHash, dp->keystr, &dkey);
  }

  switch(nt) {
  case t_dict: // end
    end_dict(udp);
    break;

  case t_array: // end
    udp->sp--;
    dp = &udp->dstack[udp->sp];
    fprintf(dfs, "%d %s end key '%s'\n", udp->sp, ttStr[nt], dp->keystr);
    if (st_playlists == 2) {
      --st_playlists;
    } else if (st_playlists == 1) {
      --st_playlists;
    }

    break;

  case t_key: // end
    dp->valstr[0] = '\0';
    break;
  case t_integer: // end
  case t_string:  // end
    if (dp->kind == t_dict) {
      struct _dstack *dpu = &udp->dstack[udp->sp - 1];
      fprintf(dfs, "%d dict key '%s' val %s %s ukey %s\n",
              udp->sp, dp->keystr, ttStr[nt], dp->valstr, dpu->keystr);
    } else if (dp->kind == t_array) {
      struct _dstack *dpu = &udp->dstack[udp->sp - 1];
      fprintf(dfs, "%d array val %s ukey '%s'\n",
              udp->sp, dp->valstr, dpu->keystr);
    }
    if (st_tracks == 2) {
      int ii = 0;
      if (nt == t_integer) ii = atoi(dp->valstr);

      struct _track *tp = &dp->track;
      switch(dkey) {
      case t_diskn:      tp->diskn      = ii; break;
      case t_diskc:      tp->diskc      = ii; break;
      case t_trackn:     tp->trackn     = ii; break;
      case t_trackc:     tp->trackc     = ii; break;
      case t_totaltime:  tp->totaltime  = ii; break;
      case t_samplerate: tp->samplerate = ii; break;
      case t_kind:       tp->kind     = strdup(dp->valstr); break;
      case t_name:       tp->name     = strdup(dp->valstr); break;
      case t_artist:     tp->artist   = strdup(dp->valstr); break;
      case t_comments:   tp->comments = strdup(dp->valstr); break;
      case t_album:      tp->album    = strdup(dp->valstr); break;
      case t_location:   tp->loc      = strdup(dp->valstr); break;
      default: ;
      }
    }
    if (st_playlists == 1) {
      int ii = 0;
      if (nt == t_integer) ii = atoi(dp->valstr);

      struct _track *tp = &dp->track;
      switch(dkey) {
      case t_name:   tp->name   = strdup(dp->valstr); break;
      case t_pid:    tp->pid    = strdup(dp->valstr); break;
      case t_ppid:   tp->ppid   = strdup(dp->valstr); break;
      case t_dkind:  tp->dkind  = ii; break;
      default: ;
      }
    } else if (st_playlists == 2) {
      struct _track *tp = &dp->track;
      switch(dkey) {
      case t_trackid: tp->trackid = strdup(dp->valstr); break;
      default: ;
      }
    }
    dp->keystr[0] = '\0';
    break;
  case t_true:  // end
  case t_false: // end
    fprintf(dfs, "%d bool st_playlists %d key '%s' val %s %d\n",
            udp->sp, st_playlists, dp->keystr, ttStr[nt], nt == t_true);

    if (st_tracks == 2) {
      struct _track *tp = &dp->track;
      switch(dkey) {
      case t_disabled: tp->disabled = nt == t_true; break;
      default: ;
      }
    }
    if (st_playlists == 1) {
      struct _track *tp = &dp->track;
      switch(dkey) {
      case t_folder: tp->folder = nt == t_true; break;
      case t_master: tp->master = nt == t_true; break;
      default: ;
      }
    }

    dp->keystr[0] = '\0';
    break;

  case t_plist:
  default:
    dp->keystr[0] = '\0';
    break;
  }
}

static void XMLCALL
char_handler(void *userData, const XML_Char *s, int len)
{
  struct _ud *udp = (struct _ud *)userData;
  if (!udp->valid) return;
  if (len == 1 && *s == '\n') return;

  struct _dstack *dp = &udp->dstack[udp->sp];
  if (dp->next == t_none || dp->next == t_bool) return;

  char *vp = NULL;
  if (dp->next == t_key) vp = dp->keystr;
  else if (dp->next == t_val) vp = dp->valstr;
  else {
    fprintf(stderr, "char_handler unk next\n");
    exit(1);
  }

  int idx = strlen(vp);
  if (VALSIZE <= len + idx) {
    fprintf(stderr, "VALSIZE len %d idx %d sum %d\n", len, idx, len + idx);
    exit(1);
  }
  memcpy(vp + idx, s, len);
  vp[idx + len] = '\0';
}

static void mygetopt(int, char *[]);
static void usage(char *file);

int
main(int argc, char *argv[])
{
  mygetopt(argc, argv);

  char buf[BUFSIZE];
  int eofflag;
  size_t len;
  XML_Parser parser;
  int ret;

  org_rmprefixlen = rmprefixlen = strlen(o_rmprefix);
  if (rmprefixlen <= 0) {
    usage(argv[0]);
  }
  if (o_rmprefix[rmprefixlen - 1] != '/') {
    rmprefixlen++;
  }

  if (o_debug) {
    dfs = stderr;
  } else {
    dfs = fopen("/dev/null", "r");
  }
  if (!dfs) {
    fprintf(stderr, "cannot open /dev/null\n");
  }

  ud.valid = 1;
  ud.sp = 0;
  ud.dstack[0].kind = t_array;
  ud.dstack[0].next = t_none;
  ud.dstack[0].keystr[0] = '\0';
  ud.dstack[0].valstr[0] = '\0';

#ifdef COUNT
  struct al_hash_t *ht_count = get_scalar_hash();
  if (!ht_count) exit(1);
  al_set_hash_err_msg(ht_count, "ht_count:");
  ud.hp = ht_count;
#endif

  init_tthash();

  folderHash = get_string_hash();

  ntrackHash = get_scalar_hash();
  trackHash = get_pointer_hash();
  al_set_pointer_hash_parameter(trackHash, NULL, clear_track_ent, NULL, NULL);

  if ((parser = XML_ParserCreate(NULL)) == NULL) {
    fprintf(stderr, "parser creation error\n");
    exit(1);
  }

  XML_SetUserData(parser, (void *)&ud);
  XML_SetElementHandler(parser, element_start, element_end);
  XML_SetCharacterDataHandler(parser, char_handler);

  do {
    len = fread(buf, sizeof(char), BUFSIZE, stdin);
    if (ferror(stdin)) {
      fprintf(stderr, "file error\n");
      break;
    }
    eofflag = feof(stdin);

    /* XML parse */
    if ((XML_Parse(parser, buf, (int)len, eofflag)) == 0) {
      fprintf(stderr, "parser error\n");
      break;
    }
  } while (!eofflag);

  // al_out_hash_stat(trackHash, "trackHash");
  // al_out_hash_stat(ntrackHash, "ntrackHash");
  if (o_check) {
    print_ntrack(ntrackHash, trackHash);
  }

#ifdef COUNT
  print_count(ht_count);
  ret = al_free_hash(ht_count);
  if (ret < 0) fprintf(stderr, "free ht_count %d\n", ret);
#endif

  ret = al_free_hash(trackHash);
  if (ret < 0) fprintf(stderr, "free trackHash %d\n", ret);

  ret = al_free_hash(ntrackHash);
  if (ret < 0) fprintf(stderr, "free ntrackHash %d\n", ret);

  ret = al_free_hash(folderHash);
  if (ret < 0) fprintf(stderr, "free folderHash %d\n", ret);

  ret = al_free_hash(ttHash);
  if (ret < 0) fprintf(stderr, "free ttHash %d\n", ret);

  XML_ParserFree(parser);

  return 0;
}

#define optstr(var)\
  if (*++str) var = str;\
  else if (++i < argc) var = argv[i];\
  else usage(argv[0]);

#define optint(var)\
  if (*++str) var = atoi(str);\
  else if (++ i <argc) var = atoi(argv[i]);\
  else usage(argv[0]);

static void
mygetopt (int argc, char *argv[])
{
  int i;
  for (i = 1; i < argc; i++) {
    char *str = argv[i];
    if (*str != '-') {
      //
    } else {
      switch(*++str) {
      case 'p': optstr(o_path); break;
      case 'i': optstr(o_rmprefix); break;
      case 'c': o_check = 1; break;
      case 'n': o_dry++ ; break;
      case 'd': o_debug = 1; break;
      case 'v': o_verbose = 1; break;
      default:  usage(argv[0]);
      }
    }
  }
  return;
}

static void
usage(char *file)
{
  fprintf(stderr, "%s [-n] -p path -i prefix \n", file);
  exit(1);
}

// Local Variables:
// coding: utf-8
// End:
