/*
 * crypto_kem/try.c version 20170713
 * D. J. Bernstein
 * Public domain.
 * Auto-generated by trygen.py; do not edit.
 */

#include "crypto_kem.h"
#include "try.h"

const char *primitiveimplementation = crypto_kem_IMPLEMENTATION;

#define TUNE_BYTES 1536
#ifdef SMALL
#define MAXTEST_BYTES 128
#else
#define MAXTEST_BYTES 4096
#endif
#ifdef SMALL
#define LOOPS 8
#else
#define LOOPS 64
#endif

static unsigned char *p;
static unsigned char *s;
static unsigned char *k;
static unsigned char *c;
static unsigned char *t;
static unsigned char *p2;
static unsigned char *s2;
static unsigned char *k2;
static unsigned char *c2;
static unsigned char *t2;
#define plen crypto_kem_PUBLICKEYBYTES
#define slen crypto_kem_SECRETKEYBYTES
#define klen crypto_kem_BYTES
#define clen crypto_kem_CIPHERTEXTBYTES
#define tlen crypto_kem_BYTES

void preallocate(void)
{
#ifdef RAND_R_PRNG_NOT_SEEDED
  RAND_status();
#endif
}

void allocate(void)
{
  unsigned long long alloclen = 0;
  if (alloclen < TUNE_BYTES) alloclen = TUNE_BYTES;
  if (alloclen < MAXTEST_BYTES) alloclen = MAXTEST_BYTES;
  if (alloclen < crypto_kem_PUBLICKEYBYTES) alloclen = crypto_kem_PUBLICKEYBYTES;
  if (alloclen < crypto_kem_SECRETKEYBYTES) alloclen = crypto_kem_SECRETKEYBYTES;
  if (alloclen < crypto_kem_BYTES) alloclen = crypto_kem_BYTES;
  if (alloclen < crypto_kem_CIPHERTEXTBYTES) alloclen = crypto_kem_CIPHERTEXTBYTES;
  if (alloclen < crypto_kem_BYTES) alloclen = crypto_kem_BYTES;
  p = alignedcalloc(alloclen);
  s = alignedcalloc(alloclen);
  k = alignedcalloc(alloclen);
  c = alignedcalloc(alloclen);
  t = alignedcalloc(alloclen);
  p2 = alignedcalloc(alloclen);
  s2 = alignedcalloc(alloclen);
  k2 = alignedcalloc(alloclen);
  c2 = alignedcalloc(alloclen);
  t2 = alignedcalloc(alloclen);
}

void predoit(void)
{
  crypto_kem_keypair(p,s);
}

void doit(void)
{
  crypto_kem_enc(c,k,p);
  crypto_kem_dec(t,c,s);
}

void test(void)
{
  unsigned long long loop;
  
  for (loop = 0;loop < LOOPS;++loop) {
    
    output_prepare(p2,p,plen);
    output_prepare(s2,s,slen);
    if (crypto_kem_keypair(p,s) != 0) fail("crypto_kem_keypair returns nonzero");
    checksum(p,plen);
    checksum(s,slen);
    output_compare(p2,p,plen,"crypto_kem_keypair");
    output_compare(s2,s,slen,"crypto_kem_keypair");
    
    output_prepare(c2,c,clen);
    output_prepare(k2,k,klen);
    memcpy(p2,p,plen);
    double_canary(p2,p,plen);
    if (crypto_kem_enc(c,k,p) != 0) fail("crypto_kem_enc returns nonzero");
    checksum(c,clen);
    checksum(k,klen);
    output_compare(c2,c,clen,"crypto_kem_enc");
    output_compare(k2,k,klen,"crypto_kem_enc");
    input_compare(p2,p,plen,"crypto_kem_enc");
    
    output_prepare(t2,t,tlen);
    memcpy(c2,c,clen);
    double_canary(c2,c,clen);
    memcpy(s2,s,slen);
    double_canary(s2,s,slen);
    if (crypto_kem_dec(t,c,s) != 0) fail("crypto_kem_dec returns nonzero");
    if (memcmp(t,k,klen) != 0) fail("crypto_kem_dec does not match k");
    checksum(t,tlen);
    output_compare(t2,t,tlen,"crypto_kem_dec");
    input_compare(c2,c,clen,"crypto_kem_dec");
    input_compare(s2,s,slen,"crypto_kem_dec");
    
    double_canary(t2,t,tlen);
    double_canary(c2,c,clen);
    double_canary(s2,s,slen);
    if (crypto_kem_dec(t2,c2,s2) != 0) fail("crypto_kem_dec returns nonzero");
    if (memcmp(t2,t,tlen) != 0) fail("crypto_kem_dec is nondeterministic");
    
    double_canary(t2,t,tlen);
    double_canary(c2,c,clen);
    double_canary(s2,s,slen);
    if (crypto_kem_dec(c2,c2,s) != 0) fail("crypto_kem_dec with c=t overlap returns nonzero");
    if (memcmp(c2,t,tlen) != 0) fail("crypto_kem_dec does not handle c=t overlap");
    memcpy(c2,c,clen);
    if (crypto_kem_dec(s2,c,s2) != 0) fail("crypto_kem_dec with s=t overlap returns nonzero");
    if (memcmp(s2,t,tlen) != 0) fail("crypto_kem_dec does not handle s=t overlap");
    memcpy(s2,s,slen);
  }
}
