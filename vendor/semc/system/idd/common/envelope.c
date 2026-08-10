/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

/*
 * Copyright 2015 Sony Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../iddd/logging.h"
#include "envelope.h"

int rand_key(EVP_CIPHER_CTX *ctx, unsigned char *key)
{
  if (ctx->cipher->flags & EVP_CIPH_RAND_KEY)
  {
    return EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_RAND_KEY, 0, key);
  }
  if (RAND_bytes(key, ctx->key_len) <= 0) return 0;
  return 1;
}

int encrypt_key(unsigned char *ek, const unsigned char *key, int key_len, EVP_PKEY *pubk)
{
  int ret=0;

  if (pubk->type != EVP_PKEY_RSA)
  {
    IDD_LOGE("encrypt_key failed");
    return(ret);
  }

  ret=RSA_public_encrypt(key_len,key,ek,pubk->pkey.rsa,RSA_PKCS1_PADDING);
  return(ret);
}

int decrypt_key(unsigned char *key, const unsigned char *ek, int ekl,
                EVP_PKEY *priv)
{
  int ret= -1;

  if (priv->type != EVP_PKEY_RSA)
  {
    IDD_LOGE("Public key is not RSA");
    return(ret);
  }

  ret=RSA_private_decrypt(ekl,ek,key,priv->pkey.rsa,RSA_PKCS1_PADDING);
  return(ret);
}

int seal_init(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type, unsigned char **ek,
              int *ekl, unsigned char *iv, EVP_PKEY **pubk, int npubk)
{
  unsigned char key[EVP_MAX_KEY_LENGTH] = {0};
  int i;

  if(type) {
    EVP_CIPHER_CTX_init(ctx);
    if(!EVP_EncryptInit_ex(ctx,type,NULL,NULL,NULL)) return 0;
  }
  if ((npubk <= 0) || !pubk)
    return 1;
  if (rand_key(ctx, key) <= 0)
    return 0;
  if (EVP_CIPHER_CTX_iv_length(ctx))
    RAND_pseudo_bytes(iv,EVP_CIPHER_CTX_iv_length(ctx));

  if(!EVP_EncryptInit_ex(ctx,NULL,NULL,key,iv)) return 0;

  for (i=0; i<npubk; i++)
  {
    ekl[i]=encrypt_key(ek[i],key,EVP_CIPHER_CTX_key_length(ctx),
                       pubk[i]);
    if (ekl[i] <= 0) return(-1);
  }
  return(npubk);
}

int seal_update(EVP_CIPHER_CTX *ctx, uint8_t *out, int *out_len,
                       const uint8_t *in, int in_len)
{
  return EVP_EncryptUpdate(ctx, out, out_len, in, in_len);
}

int seal_final(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl)
{
  int i;
  i = EVP_EncryptFinal_ex(ctx,out,outl);
  if (i)
    i = EVP_EncryptInit_ex(ctx,NULL,NULL,NULL,NULL);
  return i;
}

int open_init(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
              const unsigned char *ek, int ekl, const unsigned char *iv,
              EVP_PKEY *priv)
{
  unsigned char *key=NULL;
  int i,size=0,ret=0;

  if(type) {
    EVP_CIPHER_CTX_init(ctx);
    if(!EVP_DecryptInit_ex(ctx,type,NULL, NULL,NULL)) return 0;
  }

  if(!priv) return 1;

  if (priv->type != EVP_PKEY_RSA)
  {
    IDD_LOGE("Public key is not RSA");
    goto err;
  }

  size=RSA_size(priv->pkey.rsa);
  key=(unsigned char *)OPENSSL_malloc(size+2);
  if (key == NULL)
  {
    /* ERROR */
    IDD_LOGE("OPENSSL_malloc failed");
    goto err;
  }

  i=decrypt_key(key,ek,ekl,priv);
  if ((i <= 0) || !EVP_CIPHER_CTX_set_key_length(ctx, i))
  {
    /* ERROR */
    goto err;
  }
  if(!EVP_DecryptInit_ex(ctx,NULL,NULL,key,iv)) goto err;

  ret=1;
err:
  if (key != NULL) OPENSSL_cleanse(key,size);
  OPENSSL_free(key);
  return(ret);
}

int open_update(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
                const unsigned char *in, int inl)
{
  return EVP_DecryptUpdate(ctx, out, outl, in, inl);
}

int open_final(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl)
{
  int i;

  i=EVP_DecryptFinal_ex(ctx,out,outl);
  if (i)
  {
    i = EVP_DecryptInit_ex(ctx,NULL,NULL,NULL,NULL);
  }
  return(i);
}
