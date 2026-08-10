/*
 * Copyright 2017 Sony Corporation
 */

#include <stdlib.h>

/**
 * Split a string to a sequence of tokens.
 * If the string has a sequence of delimiter, empty tokens returns.
 * If the string has a delimiter at the end, an empty token returns.
 * The usage of arguments is the same as strtok_r().
 * On the first call, str should point to the string to be parsed,
 * and the value of saveptr is ignored.
 * In subsequent calls, str should be NULL, and saveptr should be unchanged
 * since the previous call.
 *
 * @param str string to be parsed on the first call, that must be null-terminated.
 * @param delim delimiter to split a string, that must be null-terminated.
 * @param saveptr position of lastly scanned delimiter, that is given
 *                for subsequent calls that parse the same string
 *
 * @return pointer to next token if it exists. Otherwise NULL.
 */
char* tokenize_r(char *str, const char *delim, char **saveptr)
{
  char *token = NULL;

  if (str == NULL && *saveptr == NULL)
  {
    goto EXIT;
  }

  if (str == NULL && *saveptr != NULL)
  {
    str = *saveptr;
  }

  token = str;

  while (*str != 0)
  {
    char *d;
    for (d = (char *)delim; *d != 0; ++d)
    {
      if (*str == *d)
      {
        *str = 0;
        *saveptr = str + 1;
        goto EXIT;
      }
    }
    ++str;
  }
  /* When comes here, there is no next token. */
  *saveptr = NULL;
EXIT:
  return token;
}
