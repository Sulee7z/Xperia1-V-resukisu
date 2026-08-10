/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief A sorted, serializable list, for IDD
 *
 * Yet another list? Well, unfortunately: yes. We need a container (just about
 * any data structure) which we can search very quickly in. So we actually want a map.
 *
 * And it must be serializable/unserializable, for easy storage and loading.
 *
 * This is what we came up with.
 *
 * The list contains pointers to user-defined elements. The pointers does not
 * even have to be valid pointers values - as long as the user-supplied
 * functions for comparing and deleting can handle them. NULL is not allowed
 * however as that is used for indicating errors, e.g. when loading lists.
 * Also, all elements must be considered unique, meaning that the expression
 * compare(e1, e2) != 0 must be fulfilled for all elements in the list.
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef __USE_GNU
#define __USE_GNU  // for memmem()
#endif
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "list.h"
#include "config.h"

#define LOG_TAG "iddd-list"
#include "logging.h"


/* The maximum list size. Used when loading data from file to make
 * sure that we don't read corrupt data, which we interpret as valid
 * elements, and read forever. A safety in other words.
 *
 * The reason why it's not in config.h is that to avoid references
 * to that file, so that the list can be tested isolated from the
 * rest.
 */
#define MAX_LIST_SIZE 20000

idd_list_t* list_create(size_t initial_capacity,
                        size_t grow_by,
                        idd_list_compare_t compare,
                        idd_list_destruct_t destruct)
{
  idd_list_t* list_p = malloc(sizeof(*list_p));
  if (list_p != NULL)
  {
    list_p->list_pp = malloc(initial_capacity * sizeof(*list_p->list_pp));
    list_p->capacity = list_p->list_pp != NULL ? initial_capacity : 0;
    list_p->length = 0;
    list_p->grow_by = grow_by;
    list_p->compare = compare;
    list_p->destruct = destruct;
  }
  return list_p;
}

/* Searches for an item in the list and returns the position where it is,
 * or should be if it's not present (good candidate for insertion).
 *
 * *found_p will be set to 0 if the element was not found, and non-zero
 * otherwise.
 */
static size_t list_search(idd_list_t* list_p, const void* key_p, int* found_p)
{
  size_t low = 0;
  size_t high = list_p->length;

  while (low < high)
  {
    size_t mid = low + ((high - low) / 2);
    int cmp = list_p->compare(key_p, list_p->list_pp[mid]);
    if (cmp == 0)
    {
      *found_p = 1;
      return mid;
    }
    else if (cmp > 0)
    {
      low = mid + 1;
    }
    else
    {
      high = mid;
    }
  }
  *found_p = 0;
  return low;
}

/* Deallocates an element, calling its destructor if available. */
static inline void list_free_element(idd_list_t* list_p, void** element_pp)
{
  if (*element_pp != NULL)
  {
    if (list_p->destruct != NULL)
    {
      list_p->destruct(*element_pp);
    }
    free(*element_pp);
    *element_pp = NULL;
  }
}

/* Increases the capacity of the list (not its length) if needed to make
 * room for yet another element.
 *
 * Returns 0 on failure, and non-zero on success.
 */
static inline int list_grow(idd_list_t* list_p)
{
  if (list_p->length == list_p->capacity)
  {
    size_t new_capacity = list_p->capacity + list_p->grow_by;
    void** new_list_pp = realloc(list_p->list_pp, new_capacity * sizeof (*new_list_pp));
    if (new_list_pp == NULL)
    {
      IDD_LOGW("Failed to grow list to %zu elements", new_capacity);
      return 0;
    }
    list_p->list_pp = new_list_pp;
    list_p->capacity = new_capacity;
  }
  return 1;
}

int list_add(idd_list_t* list_p, void** element_pp)
{
  size_t insert_idx;
  int found;

  if (*element_pp == NULL)
  {
    IDD_LOGW("list_add: Element is NULL!");
    return 0;
  }

  if (!list_grow(list_p))
  {
    list_free_element(list_p, element_pp);
    return 0;
  }

  insert_idx = list_search(list_p, *element_pp, &found);

  if (found)
  {
    // We don't allow duplicates.
    IDD_LOGW("list_add: Duplicate found!");
    return 0;
  }

  if (insert_idx < list_p->length)
  {
    memmove(&list_p->list_pp[insert_idx + 1],
            &list_p->list_pp[insert_idx],
            (list_p->length - insert_idx) * sizeof(*list_p->list_pp));
  }

  list_p->list_pp[insert_idx] = *element_pp;
  list_p->length++;
  return 1;
}


/* Adds an element at the end of the list. For maintaining the integrity
 * of the list, the inserted element must be larger or equal to all other
 * elements, as defined by its comparison function.
 *
 * Returns zero on failure and non-zero on success.
 */
static int list_add_unsorted(idd_list_t* list_p, void** element_pp)
{
  if (!list_grow(list_p))
  {
    list_free_element(list_p, element_pp);
    return 0;
  }

  list_p->list_pp[list_p->length] = *element_pp;
  list_p->length++;
  return 1;
}

void list_destroy(idd_list_t** list_pp)
{
  size_t i;
  idd_list_t* list_p = *list_pp;
  for (i = 0; i < list_p->length; i++)
  {
    list_free_element(list_p, &list_p->list_pp[i]);
  }
  free(list_p->list_pp);
  free(list_p);
  *list_pp = NULL;
}

void* list_find(idd_list_t* list_p, void* key_p)
{
  int found;
  size_t idx = list_search(list_p, key_p, &found);
  return found ? list_p->list_pp[idx] : NULL;
}

void list_iterate(idd_list_t* list_p, idd_list_iterate_t fn, void* userdata_p)
{
  size_t i;
  for (i = 0; i < list_p->length; i++)
  {
    fn(list_p->list_pp[i], userdata_p);
  }
}

/* When the list is saved, the elements are saved one by one to the file. Since
 * the elements are sorted in the list we save from, we can just read the entries
 * from file and expect them to be in the correct order.
 */
int list_load(idd_list_t* list_p, const char* fname_p, idd_list_load_t callback, void* userdata_p)
{
  FILE* fp = fopen(fname_p, "rb");
  int ret = 0;

  if (fp != NULL)
  {
    int safety_iterator;
    char entry[1024] = {0};
    for (safety_iterator = 0; safety_iterator < MAX_LIST_SIZE; safety_iterator++)
    {
      long bytes_read = (long)fread(entry, 1, sizeof(entry), fp);
      char* entry_end_p;
      void* element_p;

      if (bytes_read < IDD_RECORD_END_LEN)
      {
        // This happens when we get EOF
        ret = 1;
        break;
      }

      entry_end_p = memmem(entry, bytes_read, IDD_RECORD_END_STR, IDD_RECORD_END_LEN);
      if ((entry_end_p == NULL) || ((entry_end_p - entry) < 0))
      {
        IDD_LOGE("Corrupt data when reading list");
        break;
      }

      element_p = callback(entry, entry_end_p, userdata_p);
      if (element_p != NULL)
      {
        if (!list_add_unsorted(list_p, (void**)&element_p))
        {
          IDD_LOGE("Out of memory when adding element to list");
          break;
        }
      }

      if (fseek(fp, -(bytes_read - (entry_end_p - entry) - IDD_RECORD_END_LEN), SEEK_CUR) == -1)
      {
        IDD_LOGE("Failed to seek in file");
        break;
      }
    }
    fclose(fp);
  }
  return ret;
}

int list_save(idd_list_t* list_p, const char* fname_p, idd_list_save_t callback, void* userdata_p)
{
  char temp_fname[PATH_MAX] = {0};
  snprintf(temp_fname, PATH_MAX, "%s%s", fname_p, TEMPORARY_FILE_SUFFIX);

  FILE* fp = fopen(temp_fname, "wb");
  size_t i;

  if (fp == NULL)
  {
    IDD_LOGE("Failed to open list file for writing: %s", strerror(errno));
    return 0;
  }

  for (i = 0; i < list_p->length; i++)
  {
    if (!callback(fp, list_p->list_pp[i], userdata_p))
    {
      IDD_LOGE("Failed to save data to list file.");
      fclose(fp);
      unlink(temp_fname);
      return 0;
    }
  }

  fclose(fp);

  if (rename(temp_fname, fname_p) != 0)
  {
    IDD_LOGE("Failed to rename file %s to %s: %s", temp_fname, fname_p, strerror(errno));
    unlink(temp_fname);
    return 0;
  }

  return 1;
}
