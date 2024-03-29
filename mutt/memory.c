/**
 * @file
 * Memory management wrappers
 *
 * @authors
 * Copyright (C) 2017-2023 Richard Russon <rich@flatcap.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page mutt_memory Memory management wrappers
 *
 * "Safe" memory management routines.
 *
 * @note If any of the allocators fail, the user is notified and the program is
 *       stopped immediately.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "exit.h"
#include "logging2.h"
#include "message.h"

/**
 * mutt_mem_calloc - Allocate zeroed memory on the heap
 * @param nmemb Number of blocks
 * @param size  Size of blocks
 * @retval ptr Memory on the heap
 *
 * @note On error, this function will never return NULL.
 *       It will print an error and exit the program.
 *
 * The caller should call mutt_mem_free() to release the memory
 */
void *mutt_mem_calloc(size_t nmemb, size_t size)
{
  if ((nmemb == 0) || (size == 0))
    return NULL;

  void *p = calloc(nmemb, size);
  if (!p)
  {
    mutt_error(_("Out of memory")); // LCOV_EXCL_LINE
    mutt_exit(1);                   // LCOV_EXCL_LINE
  }
  return p;
}

/**
 * mutt_mem_free - Release memory allocated on the heap
 * @param ptr Memory to release
 */
void mutt_mem_free(void *ptr)
{
  if (!ptr)
    return;
  void **p = (void **) ptr;
  if (*p)
  {
    free(*p);
    *p = NULL;
  }
}

/**
 * mutt_mem_malloc - Allocate memory on the heap
 * @param size Size of block to allocate
 * @retval ptr Memory on the heap
 *
 * @note On error, this function will never return NULL.
 *       It will print an error and exit the program.
 *
 * The caller should call mutt_mem_free() to release the memory
 */
void *mutt_mem_malloc(size_t size)
{
  if (size == 0)
    return NULL;

  void *p = malloc(size);
  if (!p)
  {
    mutt_error(_("Out of memory")); // LCOV_EXCL_LINE
    mutt_exit(1);                   // LCOV_EXCL_LINE
  }
  return p;
}

/**
 * mutt_mem_realloc - Resize a block of memory on the heap
 * @param ptr Memory block to resize
 * @param size New size
 *
 * @note On error, this function will never return NULL.
 *       It will print an error and exit the program.
 *
 * If the new size is zero, the block will be freed.
 */
void mutt_mem_realloc(void *ptr, size_t size)
{
  if (!ptr)
    return;

  void **p = (void **) ptr;

  if (size == 0)
  {
    if (*p)
    {
      free(*p);
      *p = NULL;
    }
    return;
  }

  void *r = realloc(*p, size);
  if (!r)
  {
    mutt_error(_("Out of memory")); // LCOV_EXCL_LINE
    mutt_exit(1);                   // LCOV_EXCL_LINE
  }

  *p = r;
}

/**
 * mutt_mem_realloc_zero - Resize a block of memory on the heap
 * @param ptr Memory block to resize
 * @param cur_size Current size
 * @param new_size New size
 *
 * @note On error, this function will never return NULL.
 *       It will print an error and exit the program.
 *
 * If the new size is zero, the block will be freed.
 */
void mutt_mem_realloc_zero(void *ptr, size_t cur_size, size_t new_size)
{
  if (!ptr)
    return;

  mutt_mem_realloc(ptr, new_size);

  unsigned char *cp = *(unsigned char **) ptr;

  if (!cp || (new_size == 0) || (new_size <= cur_size))
    return;

  memset(cp + cur_size, '\0', new_size - cur_size);
}
