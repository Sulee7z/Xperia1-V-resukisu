/*
 * Copyright 2010 Sony Corporation
 */

#ifndef CRYPTO_H_INCLUSION_GUARD
#define CRYPTO_H_INCLUSION_GUARD

#include <openssl/evp.h>
#include "idd_config.h"

/* Encryption context.
 *
 * Note: The key_size below is in bits.
 */
typedef struct {
  EVP_CIPHER_CTX ctx;
  unsigned char key[IDD_MAX_PUBLIC_KEY_SIZE / 8];
  size_t key_size;
  unsigned char iv[IDD_SYMMETRIC_KEY_SIZE / 8];
} idd_encryption_t;

/* Initialize paths to encryption and verification certificates
   with defaults.
*/
void crypto_init();

/* Check if the encryption key file is accessible */
int is_encryption_active();

/* Check if the verification key file is accessible */
int is_verification_active();

/* Returns the encrypted size of the plain text of size 'in_size' */
size_t get_encrypted_size(size_t in_size);

/* Verifies the configuration file specified, using the provided signature
 *
 * Returns 1 on success and 0 on failure
 */
int crypto_verify_config(const char* filename_p, const char* signature_p);

/* Initializes the encryption context.
 * After having called this function, the members
 * in enc_p are valid, i.e. can the encrypted key and
 * the IV be extracted.
 */
int encrypt_init(idd_encryption_t* enc_p);

/* Encrypts the file 'in_fp' and writes to 'out_fp'
 *
 * Returns 1 on success and 0 on failure.
 */
int encrypt_file(FILE* in_fp, FILE* out_fp, idd_encryption_t* enc_p);

/* Frees the encryption context.
 */
void encrypt_destroy(idd_encryption_t* enc_p);

#endif
