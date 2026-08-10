/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief A sorted, serializable list, for IDD
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef LIST_H_INCLUSION_GUARD
#define LIST_H_INCLUSION_GUARD

#include <string.h>

#define IDD_RECORD_END_STR "\0\0\n"
#define IDD_RECORD_END_LEN 3

/* Returns > 0 if element 1 is larger than element 2, 0 if they are equal
 * or < 0 if element 1 is smaller than element 2.
 */
typedef int (*idd_list_compare_t)(const void* element1_p, const void* element2_p);

/* Destructor for the element. The element itself must not be free'd, but
 * all other resources it points out.
 */
typedef void (*idd_list_destruct_t)(void* element_p);

/* Called for every element in the list, when calling list_iterate.
 * 'userdata_p' will be the user data specified in that function call.
 */
typedef void (*idd_list_iterate_t)(const void* element_p, void* userdata_p);

/* Serialize the element into the file handle specified.
 * 'userdata_p' is the same value as specified in list_save.
 * Returns zero on failure and non-zero on success.
 */
typedef int (*idd_list_save_t)(FILE* fp, const void* element_p, void* userdata_p);

/* Unserialize the element and return it, allocated using malloc.
 * 'userdata_p' is the same value as specified in list_load.
 */
typedef void* (*idd_list_load_t)(char* this_record_p, char* next_record_p, void *userdata_p);

typedef struct {
  void** list_pp;
  size_t length;
  size_t capacity;
  size_t grow_by;
  idd_list_compare_t compare;
  idd_list_destruct_t destruct;
} idd_list_t;

/* Creates a list, which will initially be empty.
 * destruct may be NULL.
 */
idd_list_t* list_create(size_t initial_size,
                        size_t grow_by,
                        idd_list_compare_t compare,
                        idd_list_destruct_t destruct);

/* Adds an item to the list. The list takes ownership of the item (always)
 * Returns 0 on failure and non-zero on success.
 */
int list_add(idd_list_t* list_p, void** element_pp);

/* Destroys (free's) a list and all its elements */
void list_destroy(idd_list_t** list_pp);

/* Finds a key in a list - which will invoke the ->compare method
 * Returns NULL if the element does not exist.
 */
void* list_find(idd_list_t* list_p, void* key_p);

/* Iterates and calls fn(item, userdata_p) for every item in the list */
void list_iterate(idd_list_t* list_p, idd_list_iterate_t fn, void* userdata_p);

/* Saves list_p into a file called fname_p, using fp as serialization function
 * Returns 0 on failure and non-zero on success.
 *
 * 'userdata_p' is a user-specified pointer that will be passed to the callback.
 */
int list_save(idd_list_t* list_p, const char* fname_p, idd_list_save_t callback, void* userdata_p);

/* Loads a list from file, where every line in the list represents one item.
 * Returns 0 on failure and non-zero on success.
 *
 * 'userdata_p' is a user-specified pointer that will be passed to the callback.
 */
int list_load(idd_list_t* list_p, const char* fname_p, idd_list_load_t callback, void *userdata_p);

#endif
