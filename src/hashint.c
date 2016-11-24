/*
 *  hashint.c
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 */

#include <stdio.h>
#include "itpl2dirtree.h"

void
init_tthash()
{
  ttHash = get_scalar_hash();
  al_set_hash_err_msg(ttHash, "ttHash:");
  enum _tt tidx = 0;
  for (; tidx < _tt_last; ++tidx) {
    enum _tt id = ttIdStr[tidx].id;
    if (id == _tt_last) break;
    int ret = item_set(ttHash, ttIdStr[tidx].name, id);
    if (ret != 0) fprintf(stderr, "ttHash add %d\n", ret);
    ttStr[id] = ttIdStr[tidx].name;
  }
}

struct al_hash_t *
get_scalar_hash()
{
  struct al_hash_t *hp = NULL;
  int ret = al_init_hash(HASH_TYPE_SCALAR, AL_DEFAULT_HASH_BIT, &hp);

  if (ret < 0) {
    fprintf(stderr, "init scalar hash %d\n", ret);
    return NULL;
  }
  return hp;
}

struct al_hash_t *
get_string_hash()
{
  struct al_hash_t *hp = NULL;
  int ret = al_init_hash(HASH_TYPE_STRING, AL_DEFAULT_HASH_BIT, &hp);

  if (ret < 0) {
    fprintf(stderr, "init string hash %d\n", ret);
    return NULL;
  }
  return hp;
}

struct al_hash_t *
get_pointer_hash()
{
  struct al_hash_t *hp = NULL;
  int ret = al_init_hash(HASH_TYPE_POINTER, AL_DEFAULT_HASH_BIT, &hp);

  if (ret < 0) {
    fprintf(stderr, "init pointer hash %d\n", ret);
    return NULL;
  }
  return hp;
}

void
print_count(struct al_hash_t *ht_count)
{
  struct al_hash_iter_t *itr;
  const char *ikey;
  value_t v;

  int ret = al_hash_iter_init(ht_count, &itr, AL_SORT_COUNTER_DIC|AL_SORT_VALUE|AL_ITER_AE);
  if (ret < 0)
    fprintf(stderr, "itr init %d\n", ret);

  while (0 <= (ret = al_hash_iter(itr, &ikey, &v))) {
      printf("%ld\t%s\n", v, ikey);
  }
  if (ret == 0) { // invoke _end() manually
    al_hash_iter_end(itr);
  }
}

void
print_ntrack(struct al_hash_t *hp, struct al_hash_t *thp)
{
  struct al_hash_iter_t *itr;
  const char *ikey;
  value_t v;

  int ret = al_hash_iter_init(hp, &itr, AL_SORT_DIC|AL_ITER_AE);
  // int ret = al_hash_iter_init(hp, &itr, AL_ITER_AE);
  if (ret < 0)
    fprintf(stderr, "itr init %d\n", ret);

  while (0 <= (ret = al_hash_iter(itr, &ikey, &v))) {
    if (v == 1) {
      struct _track *rp;
      int ret = item_get_pointer(thp, ikey, (void *)&rp);
      if (ret != 0) fprintf(stderr, "item_get_pointer trackHash ret %d\n", ret);

      fprintf(stderr, "track %s %s %s %s\n", ikey, rp->artist, rp->name, rp->album);
    }
  }
  if (ret == 0) { // invoke _end() manually
    al_hash_iter_end(itr);
  }
}

