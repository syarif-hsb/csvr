/* This file is part of csvr.
 *
 * csvr is free software: you can redistribute it and/or modify it under the terms of the 
 * GNU General Public License as published by the Free Software Foundation, either version 
 * 2 of the License, or (at your option) any later version.
 *
 * csvr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with csvr. 
 * If not, see <https://www.gnu.org/licenses/>. 
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Macros */
#define DEFAULT_MEMORY_LEN 256
#define STRING_SUCCESS       0
#define STRING_FAILURE     (-1)

/* Struct declaration */
typedef struct string_t {
  char *l;
  size_t len;
  size_t mlen;
} STRING_ST;

typedef struct vector_t {
  STRING_ST **strs;
  size_t len;
  size_t mlen;
} VECTOR_ST;

typedef struct table_t {
  VECTOR_ST **vs;
  size_t len;
  size_t mlen;
} TABLE_ST;
/* End of Struct declaration */

/* Function declaration */
TABLE_ST*  new_table();
TABLE_ST*  new_table_s(size_t sz);
TABLE_ST*  t_append(TABLE_ST *dst, VECTOR_ST *src);
TABLE_ST*  t_concat(int n, ...);
TABLE_ST*  t_copy(TABLE_ST *src);
TABLE_ST*  parse_delimited_f(FILE *s, char d); /* Not Yet implemented */
TABLE_ST*  transpose(TABLE_ST *t);

int del_table(TABLE_ST *t);
size_t t_get_len(TABLE_ST *t);
size_t t_get_mlen(TABLE_ST *t);
size_t t_get_max_vector_len(TABLE_ST *t);
const char* t_get_str_l(TABLE_ST *t, size_t i, size_t j);
STRING_ST* t_get_str(TABLE_ST *t, size_t i, size_t j);
VECTOR_ST* t_get_vector(TABLE_ST *t, size_t index);

VECTOR_ST* new_vector();
VECTOR_ST* new_vector_s(size_t sz);
VECTOR_ST* v_append(VECTOR_ST *dst, STRING_ST *src);
VECTOR_ST* v_concat(int n, ...);
VECTOR_ST* v_copy(VECTOR_ST *src);
VECTOR_ST* parse_delimited_c(STRING_ST *s, char d);
VECTOR_ST* parse_delimited_l(STRING_ST *s, const char *d); /* Not Yet implemented */

int del_vector(VECTOR_ST *v);
size_t v_get_len(VECTOR_ST *v);
size_t v_get_mlen(VECTOR_ST *v);
const char* v_get_str_l(VECTOR_ST* v, size_t index);
STRING_ST* v_get_str(VECTOR_ST *v, size_t index);

STRING_ST* new_str(const char *l);
STRING_ST* new_str_s(const char *l, size_t sz);
STRING_ST* new_empty_str();
STRING_ST* new_empty_str_s(size_t sz);
STRING_ST* s_append_c(STRING_ST *dst, char ch);
STRING_ST* s_append_l(STRING_ST *dst, const char *src);
STRING_ST* s_concat(int n, ...);
STRING_ST* s_copy(STRING_ST *s);

int del_str(STRING_ST *s);
size_t s_get_len(STRING_ST *s);
size_t s_get_mlen(STRING_ST *s);
const char* s_get_str_l(STRING_ST *s);

static TABLE_ST*  check_table(TABLE_ST* t);
static VECTOR_ST* check_vector(VECTOR_ST* v); /* Not Yet implemented */
static STRING_ST* check_string(STRING_ST* s); /* Not Yet implemented */
/* End of Function declaration */

/* Functions */
TABLE_ST* new_table()
{
  TABLE_ST* t;
  t = new_table_s(DEFAULT_MEMORY_LEN);
  if (!t)
    return NULL;

  return t;
}

TABLE_ST* new_table_s(size_t sz)
{
  TABLE_ST *t;
  t = calloc(1, sizeof(VECTOR_ST));
  if (!t)
    return NULL;
  t->vs = calloc(sz, sizeof(VECTOR_ST*));
  if (!t->vs) {
    free(t);
    return NULL;
  }

  t->len = 0;
  t->mlen = sz;

  return t;
}

TABLE_ST* t_append(TABLE_ST *dst, VECTOR_ST *src)
{
  size_t len = dst->len;
  size_t mlen = dst->mlen;

  len += 1;
  if (len > mlen) {
    VECTOR_ST **tmp = calloc(len + 1, sizeof(VECTOR_ST*));
    if (!tmp)
      return NULL;

    memcpy(tmp, dst->vs, mlen * sizeof(VECTOR_ST*));
    mlen = len;
    free(dst->vs);
    dst->vs = tmp;
  }

  dst->vs[len - 1] = src;
  dst->len = len;
  dst->mlen = mlen;

  return dst;
}

TABLE_ST* t_concat(int n, ...)
{
  va_list ap, aq;
  va_start(ap, n);
  va_copy(aq, ap);

  size_t total_len = 0;
  for(int i = 0; i < n; i++) {
    TABLE_ST *tmp = va_arg(ap, TABLE_ST*);
    if (!tmp)
      continue;
    total_len += tmp->len;
  }
  va_end(ap);

  TABLE_ST *t;
  t = new_table_s(total_len);
  if (!t)
    return NULL;

  for (int i = 0; i < n; i++) {
    TABLE_ST *tmp = va_arg(aq, TABLE_ST*);
    for (int j = 0; j < tmp->len; j++)
      t_append(t, v_copy(tmp->vs[j]));
  }
  va_end(aq);

  return t;
}

TABLE_ST* t_copy(TABLE_ST *src)
{
  size_t len = src->len;
  size_t mlen = src->mlen;

  TABLE_ST *dst;
  dst = new_table_s(mlen);
  if (!dst)
    return NULL;

  for (size_t i = 0; i < len; i++) {
    dst->vs[i] = v_copy(src->vs[i]);
  }

  dst->len = len;
  dst->mlen = mlen;

  return dst;
}

TABLE_ST* transpose(TABLE_ST *src)
{
  if (check_table(src) == NULL)
    return NULL;

  size_t len = src->len;
  size_t max_length = 0;

  for (size_t i = 0; i < len; i++) {
    max_length = max_length > src->vs[i]->len ? max_length : src->vs[i]->len;
  }

  TABLE_ST *dst = new_table(max_length);

  for (size_t i = 0; i < max_length; i++) {
    t_append(dst, new_vector());
  }

  for (size_t i = 0; i < len; i++) {
    for (size_t j = 0; j < max_length && j < src->vs[i]->len; j++) {
      if (src->vs[i]->strs[j] == NULL)
        continue;
      v_append(dst->vs[j], s_copy(src->vs[i]->strs[j]));
    }
  }

  del_table(src);

  return dst;
}

int del_table(TABLE_ST *t)
{
  if (check_table(t) == NULL)
    return STRING_FAILURE;

  for (int i = 0; i < t->len; i++) {
    if (!t->vs[i])
      continue;
    del_vector(t->vs[i]);
  }

  free(t->vs);
  free(t);

  return STRING_SUCCESS;
}

size_t t_get_len(TABLE_ST *t)
{
  if (!t)
    return STRING_FAILURE;

  return t->len;
}

size_t t_get_mlen(TABLE_ST *t)
{
  if (!t)
    return STRING_FAILURE;

  return t->mlen;
}

size_t t_get_max_vector_len(TABLE_ST *t)
{
  size_t max_length = 0;
  
  for (size_t i = 0; i < t->len; i++)
    max_length = max_length > t->vs[i]->len ? max_length : t->vs[i]->len;

  return max_length;
}

STRING_ST* t_get_str(TABLE_ST *t, size_t i, size_t j)
{
  if (!t)
    return NULL;
  if (!t->vs)
    return NULL;
  if (!t->vs[i])
    return NULL;
  if (!t->vs[i]->strs)
    return NULL;
  if (!t->vs[i]->strs[j])
    return NULL;

  return t->vs[i]->strs[j];
}

VECTOR_ST* t_get_vector(TABLE_ST *t, size_t index)
{
  if (!t)
    return NULL;
  if (!t->vs)
    return NULL;

  return t->vs[index];
}

const char* t_get_str_l(TABLE_ST* t, size_t i, size_t j)
{
  if (!t)
    return NULL;
  if (!t->vs)
    return NULL;
  if (!t->vs[i])
    return NULL;
  if (!t->vs[i]->strs)
    return NULL;
  if (!t->vs[i]->strs[j])
    return NULL;

  return v_get_str_l(t->vs[i], j);
}

VECTOR_ST* new_vector()
{
  VECTOR_ST *v;
  v = new_vector_s(DEFAULT_MEMORY_LEN);
  if (!v)
    return NULL;

  return v;
}

VECTOR_ST* new_vector_s(size_t sz)
{
  VECTOR_ST *v;
  v = calloc(1, sizeof(VECTOR_ST));
  if (!v)
    return NULL;
  v->strs = calloc(sz, sizeof(STRING_ST*));
  if (!v->strs) {
    free(v);
    return NULL;
  }

  v->len = 0;
  v->mlen = sz;

  return v;
}

VECTOR_ST* v_append(VECTOR_ST *dst, STRING_ST *src)
{
  size_t len = dst->len;
  size_t mlen = dst->mlen;

  len += 1;
  if (len > mlen) {
    STRING_ST **tmp = calloc(len + 1, sizeof(STRING_ST*));
    if (!tmp)
      return NULL;

    memcpy(tmp, dst->strs, mlen * sizeof(STRING_ST*));
    mlen = len;
    free(dst->strs);
    dst->strs = tmp;
  }

  dst->strs[len - 1] = src;
  dst->len = len;
  dst->mlen = mlen;

  return dst;
}

VECTOR_ST* v_concat(int n, ...)
{
  va_list ap, aq;
  va_start(ap, n);
  va_copy(aq, ap);

  size_t total_len = 0;
  for(int i = 0; i < n; i++) {
    VECTOR_ST *tmp = va_arg(ap, VECTOR_ST*);
    if (!tmp)
      continue;
    total_len += tmp->len;
  }
  va_end(ap);

  VECTOR_ST *v;
  v = new_vector_s(total_len);
  if (!v)
    return NULL;

  for (int i = 0; i < n; i++) {
    VECTOR_ST *tmp = va_arg(aq, VECTOR_ST*);
    for (int j = 0; j < tmp->len; j++)
      v_append(v, s_copy(tmp->strs[j]));
  }
  va_end(aq);

  return v;
}

VECTOR_ST* v_copy(VECTOR_ST *src)
{
  size_t len = src->len;
  size_t mlen = src->mlen;

  VECTOR_ST *dst;
  dst = new_vector_s(mlen);
  if (!dst)
    return NULL;

  for (size_t i = 0; i < len; i++) {
    dst->strs[i] = s_copy(src->strs[i]);
  }

  dst->len = len;
  dst->mlen = mlen;

  return dst;
}

VECTOR_ST* parse_delimited_c(STRING_ST *s, char d)
{
  VECTOR_ST *v;
  v = new_vector();

  char *sl = s->l;
  char *l = calloc(DEFAULT_MEMORY_LEN, sizeof(char));

  int i = 0;
  int append = 0; /* Whether continue appending last str */
  char c;
  while ((c = *sl++)) {
    if (c == d || i == (DEFAULT_MEMORY_LEN - 1)) {
      if (append)
        s_append_l(v_get_str(v, v_get_len(v) - 1), l);
      else
        v_append(v, new_str(l));
      memset(l, '\0', DEFAULT_MEMORY_LEN);

      append = !(i - (DEFAULT_MEMORY_LEN - 1));
      i = 0;

      continue;
    }

    l[i] = c;
    i++;
  }
  if (append)
    s_append_l(v_get_str(v, v_get_len(v) - 1), l);
  else
    v_append(v, new_str(l));

  free(l);

  return v;
}

VECTOR_ST* parse_delimited_l(STRING_ST *s, const char *d)
{
  return NULL;
}

int del_vector(VECTOR_ST *v)
{
  if (!v)
    return STRING_FAILURE;
  if (!v->strs)
    return STRING_FAILURE;

  for (int i = 0; i < v->len; i++) {
    if (!v->strs[i])
      continue;
    del_str(v->strs[i]);
  }

  free(v->strs);
  free(v);

  return STRING_SUCCESS;
}

size_t v_get_len(VECTOR_ST *v)
{
  if (!v)
    return STRING_FAILURE;

  return v->len;
}

size_t v_get_mlen(VECTOR_ST *v)
{
  if (!v)
    return STRING_FAILURE;

  return v->mlen;
}

STRING_ST* v_get_str(VECTOR_ST *v, size_t index)
{
  if (!v)
    return NULL;
  if (!v->strs)
    return NULL;
  if (!v->strs[index])
    return NULL;

  return v->strs[index];
}

const char* v_get_str_l(VECTOR_ST* v, size_t index)
{
  if (!v)
    return NULL;
  if (!v->strs)
    return NULL;
  if (!v->strs[index])
    return NULL;

  return s_get_str_l(v->strs[index]);
}

STRING_ST* new_str(const char *s)
{
  STRING_ST *str = new_empty_str();
  if (!str)
    return NULL;

  s_append_l(str, s);

  return str;
}

STRING_ST* new_str_s(const char *s, size_t str_len)
{
  STRING_ST *str = new_empty_str_s(str_len + 1);
  if (!str)
    return NULL;

  s_append_l(str, s);

  return str;
}

STRING_ST* new_empty_str()
{
  STRING_ST *str = new_empty_str_s(DEFAULT_MEMORY_LEN);
  if (!str)
    return NULL;

  return str;
}

STRING_ST* new_empty_str_s(size_t sz)
{
  STRING_ST *str;
  str = calloc(1, sizeof(STRING_ST));
  if (!str)
    return NULL;

  str->l = calloc(sz, sizeof(sz));
  if (!str->l) {
    free(str);
    return NULL;
  }

  str->len = 0;
  str->mlen = sz;

  return str;
}

STRING_ST* s_append_c(STRING_ST *dst, char ch)
{
  if (!dst)
    return NULL;
  if (!dst->l)
    return NULL;

  size_t len = dst->len;
  size_t mlen = dst->mlen;

  /* string is full */
  if (len == (mlen - 1)) {
    char *tmp = calloc(mlen + DEFAULT_MEMORY_LEN, sizeof(char));
    if (!tmp)
      return NULL;

    memcpy(tmp, dst->l, mlen * sizeof(char));
    mlen += DEFAULT_MEMORY_LEN;
    free(dst->l);
    dst->l = tmp;
  }

  dst->l[len] = ch;
  dst->l[len + 1] = '\0';

  dst->len= len + 1;
  dst->mlen = mlen;

  return dst;
}

STRING_ST* s_append_l(STRING_ST *dst, const char *src)
{
  if (!dst)
    return NULL;
  if (!dst->l)
    return NULL;

  size_t len = dst->len;
  size_t mlen = dst->mlen;

  size_t n = 0;
  while (src[n] != '\0')
    n++;
  size_t olen = len;
  len += n;

  /* new string exceeds current memory */
  if (len >= mlen) {
    char *tmp = calloc(len + 1, sizeof(char));
    if (!tmp)
      return NULL;

    memcpy(tmp, dst->l, mlen * sizeof(char));
    mlen = len + 1;
    free(dst->l);
    dst->l = tmp;
  }

  size_t i;
  for (i = 0; i < n; i++) {
    dst->l[i + olen] = src[i];
  }
  dst->l[len] = '\0';

  dst->len = len;
  dst->mlen = mlen;

  return dst;
}

STRING_ST* s_concat(int n, ...)
{
  va_list ap, aq;
  va_start(ap, n);
  va_copy(aq, ap);

  size_t total_len = 0;
  for (int i = 0; i < n; i++) {
    STRING_ST *tmp = va_arg(ap, STRING_ST*);
    if (!tmp)
      continue;
    total_len += tmp->len;
  }
  va_end(ap);

  STRING_ST *new_str;
  new_str = new_empty_str_s(total_len);

  for (int i = 0; i < n; i++) {
    STRING_ST *tmp = va_arg(aq, STRING_ST*);
    if (tmp && tmp->l)
      s_append_l(new_str, tmp->l);
  }
  va_end(aq);

  return new_str;
}

STRING_ST* s_copy(STRING_ST *src)
{
  size_t len = src->len;
  size_t mlen = src->mlen;

  STRING_ST *dst;
  dst = new_empty_str_s(mlen);
  if (!dst)
    return NULL;

  memcpy(dst->l, src->l, mlen * sizeof(char));
  dst->len = len;
  dst->mlen = mlen;

  return dst;
}

int del_str(STRING_ST *str)
{
  if (!str->l)
    return STRING_FAILURE;
  free(str->l);

  if (!str)
    return STRING_FAILURE;
  free(str);

  return STRING_SUCCESS;
}

size_t s_get_len(STRING_ST *str)
{
  if (!str)
    return STRING_FAILURE;
  return str->len;
}

size_t s_get_mlen(STRING_ST *str)
{
  if (!str)
    return STRING_FAILURE;
  return str->mlen;
}

const char* s_get_str_l(STRING_ST *str)
{
  if (!str)
    return NULL;
  if (!str->l)
    return NULL;

  return str->l;
}

static TABLE_ST* check_table(TABLE_ST* t)
{
  if (!t)
    return NULL;
  if (!t->vs)
    return NULL;

  return t;
}
