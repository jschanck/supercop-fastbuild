#include "fips202.h"
#include "params.h"
#include "owcpa.h"

#include "randombytes.h"
#include "crypto_kem.h"

#include <string.h>
#include <stdint.h>

/* returns 0 for equal strings, 1 for non-equal strings */
static int verify(const unsigned char *a, const unsigned char *b, size_t len)
{
  uint64_t r;
  size_t i;
  r = 0;
  
  for(i=0;i<len;i++)
    r |= a[i] ^ b[i];
 
  r = (-r) >> 63;
  return r;
}

/* b = 1 means mov, b = 0 means don't mov*/
static void cmov(unsigned char *r, const unsigned char *x, size_t len, unsigned char b)
{
  size_t i;

  b = -b;
  for(i=0;i<len;i++)
    r[i] ^= b & (x[i] ^ r[i]);
}

// API FUNCTIONS 
int crypto_kem_keypair(unsigned char *pk, unsigned char *sk)
{
  size_t i;
  owcpa_keypair(pk, sk);
  for(i=0;i<NTRU_OWCPA_PUBLICKEYBYTES;i++)
    sk[i+NTRU_OWCPA_SECRETKEYBYTES] = pk[i];

  return 0;
}

int crypto_kem_enc(unsigned char *c, unsigned char *k, const unsigned char *pk)
{
  unsigned char m[NTRU_OWCPA_MSGBYTES];
  unsigned char m_seed[NTRU_SEEDBYTES];
  unsigned char buf[NTRU_COINBYTES + NTRU_SHAREDKEYBYTES + NTRU_OWCPA_MSGBYTES];
  unsigned char *c1 = buf;
  int i;

  randombytes(m_seed, NTRU_SEEDBYTES);
  owcpa_samplemsg(m, m_seed);

  shake128(buf, NTRU_COINBYTES + NTRU_SHAREDKEYBYTES + NTRU_OWCPA_MSGBYTES,
           m, NTRU_OWCPA_MSGBYTES);

  owcpa_enc(c, m, pk, c1);

  for (i = 0; i < NTRU_SHAREDKEYBYTES; i++) {
    k[i] = buf[NTRU_SHAREDKEYBYTES + i];
  }
  for (i = 0; i < NTRU_OWCPA_MSGBYTES; i++) {
    c[NTRU_OWCPA_BYTES + i] = buf[NTRU_COINBYTES + NTRU_SHAREDKEYBYTES + i];
  }

  return 0;
}

int crypto_kem_dec(unsigned char *k, const unsigned char *c, const unsigned char *sk)
{
  int i, fail;
  unsigned char m[NTRU_OWCPA_MSGBYTES];
  unsigned char cmp[NTRU_BYTES];
  unsigned char buf[NTRU_COINBYTES + NTRU_SHAREDKEYBYTES + NTRU_OWCPA_MSGBYTES];
  unsigned char tmpk[NTRU_SHAREDKEYBYTES];
  unsigned char *c1 = buf;

  owcpa_dec(m, c, sk);

  shake128(buf, NTRU_COINBYTES + NTRU_SHAREDKEYBYTES + NTRU_OWCPA_MSGBYTES,
           m, NTRU_OWCPA_MSGBYTES);

  owcpa_enc(cmp, m, sk+NTRU_OWCPA_SECRETKEYBYTES, c1);

  for (i = 0; i < NTRU_OWCPA_MSGBYTES; i++) {
    cmp[NTRU_OWCPA_BYTES + i] = buf[NTRU_COINBYTES + NTRU_SHAREDKEYBYTES + i];
  }

  fail = verify(c, cmp, NTRU_BYTES);

  // if(fail) printf("error: reencryption produces a different result\n");

  for (i = 0; i < NTRU_SHAREDKEYBYTES; i++) {
    k[i] = buf[NTRU_SHAREDKEYBYTES + i];
    m[i] = 0;
  }
  cmov(k, m, NTRU_SHAREDKEYBYTES, fail);

  return fail;
}
