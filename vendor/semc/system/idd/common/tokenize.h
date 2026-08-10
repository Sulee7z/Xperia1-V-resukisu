/*
 * Copyright 2017 Sony Corporation
 */

#ifndef TOKENIZE_H_INCLUSION_GUARD
#define TOKENIZE_H_INCLUSION_GUARD

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
char* tokenize_r(char *str, const char *delim, char **saveptr);

#endif
