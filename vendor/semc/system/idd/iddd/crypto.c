/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Iddd crypto functions
 *
 * By using encryption/verification we shall make it difficult for an
 * operator (or anyone who e.g. sets up a WLAN base station) to listen
 * to the data we produce, or to 'fake' a configuration that the
 * clients will download and execute.
 *
 * Configuration files that we download have to be cryptographically
 * signed, and we verify with a public key (conf.pem) we have if the
 * data seems to come from the correct source.
 *
 * For reports that we generate, we compress them and then encrypt
 * them using a public key (reports.pem) and put the reports in the
 * output directory.
 *
 * In many cases, the two certificates can be the same, but
 * it's possible to have two different ones, which can be useful since
 * configuration data generation might be more important to protect
 * than reading the generated reports.
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <unistd.h>
#define LOG_TAG "iddd/crypto"
#include "logging.h"
#include "crypto.h"
#include "config.h"
#include "envelope.h"


#define BUFFER_SIZE 1024

/* Path to the public key used for verifying the
 * integrity of downloaded configuration files.
 * Filename is included in the path.
 * When verification is disabled this path is set to
 * the empty string.
 */
static char conf_public_key_fpath[PATH_MAX] = {0};

/* Path to the public key used for encrypting reports
 * Filename is included in the path.
 * When encryption is disabled this path is set to
 * the empty string.
 */
static char report_public_key_fpath[PATH_MAX] = {0};

/**
 * Activate encryption with the key found in the given path by
 * assigning the global variable 'report_public_key_fpath'.
 *
 * @param pathname_p
 */
static void set_encryption_key_path(const char* pathname_p)
{
  snprintf(report_public_key_fpath, PATH_MAX, "%s%s", pathname_p, REPORT_PUBLIC_KEY_FNAME);
}

/**
 * Activate verification with the key found in the given path by
 * assigning the global variable 'conf_public_key_fpath'.
 *
 * @param pathname_p
 */
static void set_verification_key_path(const char* pathname_p)
{
  snprintf(conf_public_key_fpath, PATH_MAX, "%s%s", pathname_p, CONF_PUBLIC_KEY_FNAME);
}

/**
 * Initialize paths to encryption and verification certificates with
 * defaults.
 *
 */
void crypto_init()
{
  set_encryption_key_path(PUBLIC_KEY_DIR);
  set_verification_key_path(PUBLIC_KEY_DIR);
}

/**
 * Check if the encryption key file is accessible.
 *
 * @return 0 if successful
 */
int is_encryption_active()
{
  return (access(report_public_key_fpath, R_OK) == 0);
}

/**
 * Check if the verification key file is accessible.
 *
 * @return 0 if successful
 */
int is_verification_active()
{
  return (access(conf_public_key_fpath, R_OK) == 0);
}

/**
 * Reads a public key from a PEM file.
 *
 * @param filename_p PEM file name
 *
 * @return EVP_PKEY*
 */
static EVP_PKEY* get_public_key(const char* filename_p)
{
  FILE* fp;
  EVP_PKEY* key_p = NULL;

  fp = fopen(filename_p, "rb");
  if (fp != NULL)
  {
    key_p = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);
  }

  return key_p;
}

/**
 * Verifies the content of a file against a signature file. Only
 * called from crypto_verify_config() i.e. only used for
 * configurations.
 *
 * @param file_fp opened file to verify
 * @param sig_fp opened  file containing the signature
 * @param pub_key_p public key (as read from conf_public_key_fpath)
 *
 * @return 1 for a correct signature, 0 for failure and -1 if some other error occurred.
 */
static int verify_data(FILE* file_fp, FILE* sig_fp, EVP_PKEY* pub_key_p)
{
  int result = 0;

  EVP_MD_CTX* ctx_p = EVP_MD_CTX_create();
  if (ctx_p == NULL)
  {
    IDD_LOGE("Failed to create EVP context");
  }
  else
  {
    int rv = EVP_VerifyInit_ex(ctx_p, EVP_sha1(), NULL);
    if (rv == 0)
    {
      IDD_LOGE("Failed to initialize EVP");
    }
    else
    {
      unsigned char buf[BUFFER_SIZE] = {0};
      size_t bytes_read = fread(buf, 1, BUFFER_SIZE, file_fp);
      while (bytes_read > 0)
      {
        EVP_VerifyUpdate(ctx_p, buf, bytes_read);
        bytes_read = fread(buf, 1, BUFFER_SIZE, file_fp);
      }

      assert((BUFFER_SIZE*8) >= IDD_MAX_PUBLIC_KEY_SIZE);
      bytes_read = fread(buf, 1, BUFFER_SIZE, sig_fp);

      result = EVP_VerifyFinal(ctx_p, buf, bytes_read, pub_key_p);
    }
    EVP_MD_CTX_destroy(ctx_p);
  }

  return result;
}

/**
 * Verifies a config file using a file containing a private signature.
 *
 * @param filename_p config file name
 * @param signature_p signature file name
 *
 * @return 0 on failure and 1 on success.
 */
int crypto_verify_config(const char* filename_p, const char* signature_p)
{
  int result = 0;
  FILE* file_fp = fopen(filename_p, "rb");
  if (file_fp == NULL)
  {
    IDD_LOGE("Failed to verify file - file can't be opened.");
  }
  else
  {
    FILE* sig_fp = fopen(signature_p, "rb");
    if (sig_fp == NULL)
    {
      IDD_LOGE("Failed to open signature file.");
    }
    else
    {
      EVP_PKEY* pub_key_p = get_public_key(conf_public_key_fpath);
      if (pub_key_p == NULL)
      {
        IDD_LOGE("Failed to load public key.");
      }
      else
      {
        result = verify_data(file_fp, sig_fp, pub_key_p);

        EVP_PKEY_free(pub_key_p);
      }
      fclose(sig_fp);
    }
    fclose(file_fp);
  }
  return result;
}

/**
 * Encrypts the data it can read from in_fp using 128-bit AES-CBC and writes
 * the data to out_fp.
 *
 * The key is generated by this function (randomly), and will be encrypted
 * using the RSA 'report' key. The result will be written to enc_p->key
 *
 * Returns 1 on success and 0 on failure.
 */
int encrypt_init(idd_encryption_t* enc_p)
{
  int result = 0;
  EVP_PKEY* pub_key_p = get_public_key(report_public_key_fpath);
  if (pub_key_p != NULL)
  {
    unsigned char* ek_p = enc_p->key;
    int ekl = 1;
    memset(enc_p, 0, sizeof(*enc_p));
    enc_p->key_size = EVP_PKEY_bits(pub_key_p);
    result = seal_init(&enc_p->ctx, EVP_aes_128_cbc(), &ek_p, &ekl, enc_p->iv, &pub_key_p, 1);
    EVP_PKEY_free(pub_key_p);
  }
  else
  {
    IDD_LOGE("Failed to open report key");
  }
  return result;
}

/**
 * Frees the encryption context
 *
 * @param enc_p
 */
void encrypt_destroy(idd_encryption_t* enc_p)
{
  EVP_CIPHER_CTX_cleanup(&enc_p->ctx);
}

/**
 * Encrypts the file 'in_fp' and writes to 'out_fp'.
 *
 * @param in_fp
 * @param out_fp
 * @param enc_p encryption context
 *
 * @return 1 on success and 0 on failure
 */
int encrypt_file(FILE* in_fp, FILE* out_fp, idd_encryption_t* enc_p)
{
  unsigned char inbuf[BUFFER_SIZE] = {0};
  unsigned char outbuf[BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH] = {0};
  int outlen;

  for (;;)
  {
    int inlen = (int)fread(inbuf, 1, sizeof(inbuf), in_fp);
    if (inlen <= 0)
    {
      break;
    }

    if (!seal_update(&enc_p->ctx, outbuf, &outlen, inbuf, inlen))
    {
      IDD_LOGE("seal_update failed");
      return 0;
    }
    if ((int)fwrite(outbuf, 1, outlen, out_fp) < outlen)
    {
      IDD_LOGE("encrypt_file(): fwrite() failed, %s", strerror(errno));
    }
  }

  // Final chunk. Padding will be done here.
  if (!seal_final(&enc_p->ctx, outbuf, &outlen))
  {
    IDD_LOGE("seal_final failed");
    return 0;
  }
  if ((int)fwrite(outbuf, 1, outlen, out_fp) < outlen)
  {
    IDD_LOGE("encrypt_file(): fwrite() failed, %s", strerror(errno));
  }

  return 1;
}

/**
 * Returns the encrypted size of the plain text of size 'in_size'.
 *
 * @param in_size
 *
 * @return
 */
size_t get_encrypted_size(size_t in_size)
{
  /* This code assumes a lot of what's has been documented in OpenSSL.
   * We use 128 bit AES (that we define), and it encrypts in blocks, so
   * it pads the data using PKCS. If the input data is a multiple of
   * the block size, another block will generated.
   */
  return (in_size/(IDD_SYMMETRIC_KEY_SIZE/8) + 1) * (IDD_SYMMETRIC_KEY_SIZE/8);
}
