/*
 *  itpl2dirtree.h
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 *
 * latest version see, https://github.com/evalquote/itpl2dirtree
 */

#ifndef ITPL_E0_H
#define ITPL_E0_H

#include <stdio.h>
#include <ctype.h>
#include <alhash.h>

#define hexint(ch) \
  (( '0' <= (ch) && (ch) <= '9') ? (ch) - '0' : \
   (('A' <= (ch) && (ch) <= 'F') ? (ch) - 'A' + 10 : \
    ('a' <= (ch) && (ch) <= 'f') ? (ch) - 'a' + 10 : 0))

extern struct al_hash_t *get_scalar_hash();
extern struct al_hash_t *get_string_hash();
extern struct al_hash_t *get_pointer_hash();
extern void print_count(struct al_hash_t *ht_count);
extern void print_ntrack(struct al_hash_t *hp, struct al_hash_t *thp);

#define BUFSIZE 4096
#define KEYSIZE 128
#define VALSIZE 8192
#define STACKSIZE 64

enum _tt {
  /* tag name */
  t_key = 0,
  t_integer,
  t_string,
  t_date,
  t_data,
  t_true,
  t_false,

  t_val,
  t_bool,

  t_plist,
  t_dict,
  t_array,
  t_top,
  t_none,

  /* key name */
  t_tracks,
  t_kind,
  t_location,
  t_album,
  t_name,
  t_artist,
  t_comments,
  t_diskn,
  t_diskc,
  t_trackn,
  t_trackc,
  t_trackid,
  t_totaltime,
  t_samplerate,
  t_disabled,

  t_master,
  t_dkind,
  t_folder,
  t_pid,
  t_ppid,
  t_playlistitems,
  t_playlists,  // <key>Playlists</key> array

  _tt_last
};

struct _tstr {
  enum _tt id;
  const char *name;
};

struct _track {
  char *trackId;
  int diskn; // number
  int diskc; // count
  int trackn;
  int trackc;
  int totaltime;
  int samplerate;
  char *kind;
  char *name;
  char *artist;
  char *comments;
  char *album;
  char *loc;
  char *trackid;
  int  disabled; // bool

  int  plseq;  // seq number in a playlist
  char *pid;   // Playlist Persistent ID
  char *ppid;  // Parent Persistent ID
  int master;  // bool
  int dkind;   // integer
  int folder;  // bool

  int skip;    // bool
};

struct _dstack {
  enum _tt kind;  // top/dict/array
  enum _tt next;  // top/key/val
  char keystr[KEYSIZE];
  char valstr[VALSIZE];
  struct _track track;
};

struct _ud {
  int valid;  // 0 between end and start
  struct al_hash_t *hp;
  struct _dstack dstack[STACKSIZE];
  int sp;     // stack pointer
};

extern struct al_hash_t *ttHash;
extern struct _tstr ttIdStr[];
extern const char *ttStr[];

extern void init_tthash();

#endif
