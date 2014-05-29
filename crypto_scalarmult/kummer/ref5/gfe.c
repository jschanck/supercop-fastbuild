#include <stdio.h>
#include "gfe.h"

void gfe_invert(gfe *r, const gfe *x)
{
  gfe x2,x3,x6,x12,x15,x30,x_5_0, x_10_0, x_20_0, x_40_0, x_80_0, x_120_0, x_125_0, t;
  int i;

  gfe_square(&x2, x);                     /*  2 */
  gfe_mul(&x3,&x2,x);                     /*  3 mult */
  gfe_square(&x6,&x3);                    /*  6 */
  gfe_square(&x12,&x6);                   /*  12 */
  gfe_mul(&x15,&x12,&x3);                 /*  15 mult */
  gfe_square(&x30, &x15);                 /*  30 */
  gfe_mul(&x_5_0, &x30, x);               /*  31 = 2^5-1 mult */

  gfe_square(&t, &x_5_0);
  for(i=6;i<10;i++) gfe_square(&t, &t);   /*  2^10-2^5 */
  gfe_mul(&x_10_0,&t,&x_5_0);             /*  2^10-1 mult */

  gfe_square(&t, &x_10_0);
  for(i=11;i<20;i++) gfe_square(&t, &t);  /*  2^20-2^10 */
  gfe_mul(&x_20_0,&t,&x_10_0);            /*  2^20-1 mult */

  gfe_square(&t, &x_20_0);
  for(i=21;i<40;i++) gfe_square(&t, &t);  /*  2^40-2^20 */
  gfe_mul(&x_40_0,&t,&x_20_0);            /*  2^40-1 mult */

  gfe_square(&t, &x_40_0);
  for(i=41;i<80;i++) gfe_square(&t, &t);  /*  2^80-2^40 */
  gfe_mul(&x_80_0,&t,&x_40_0);            /*  2^80-1 mult */

  gfe_square(&t, &x_80_0);
  for(i=81;i<120;i++) gfe_square(&t, &t); /*  2^120-2^40 */
  gfe_mul(&x_120_0,&t,&x_40_0);           /*  2^80-1 mult */

  gfe_square(&t, &x_120_0);
  for(i=121;i<125;i++) gfe_square(&t, &t);/*  2^120-2^40 */
  gfe_mul(&x_125_0, &t, &x_5_0);

  gfe_square(&t, &x_125_0);               /* 2^126-2^1 */
  gfe_square(&t, &t);                     /* 2^127-2^2 */
  gfe_mul(r,&t,x);                        /* 2^127-3 */
}

void gfe_unpack(gfe *r, const unsigned char b[16])
{
  r->v[0]  = b[0];
  r->v[0] |= ((crypto_int32)b[1] << 8);
  r->v[0] |= ((crypto_int32)b[2] << 16);
  r->v[0] |= (((crypto_int32)b[3]&3) << 24);

  r->v[1]  = ((crypto_int32)b[3] >> 2);
  r->v[1] |= ((crypto_int32)b[4] << 6);
  r->v[1] |= ((crypto_int32)b[5] << 14);
  r->v[1] |= (((crypto_int32)b[6]&7) << 22);

  r->v[2]  = ((crypto_int32)b[6] >> 3);
  r->v[2] |= ((crypto_int32)b[7] << 5);
  r->v[2] |= ((crypto_int32)b[8] << 13);
  r->v[2] |= (((crypto_int32)b[9]&31) << 21);

  r->v[3]  = ((crypto_int32)b[9] >> 5);
  r->v[3] |= ((crypto_int32)b[10] << 3);
  r->v[3] |= ((crypto_int32)b[11] << 11);
  r->v[3] |= (((crypto_int32)b[12]&63) << 19);

  r->v[4]  = ((crypto_int32)b[12] >> 6);
  r->v[4] |= ((crypto_int32)b[13] << 2);
  r->v[4] |= ((crypto_int32)b[14] << 10);
  r->v[4] |= (((crypto_int32)b[15]&127) << 18);
}

void gfe_pack(unsigned char r[16], const gfe *x)
{
  crypto_int32 c;
  gfe t = *x;

  t.v[0] += ((1<<28)-4);
  c = t.v[0] >> 26;
  t.v[1] = t.v[1] + c;
  c <<= 26;
  t.v[0]  = t.v[0] - c;
  
  t.v[1] += ((1<<27)-4);
  c = t.v[1] >> 25;
  t.v[2] = t.v[2] + c;
  c <<= 25;
  t.v[1]  = t.v[1] - c;

  t.v[2] += ((1<<28)-4);
  c = t.v[2] >> 26;
  t.v[3] = t.v[3] + c;
  c <<= 26;
  t.v[2]  = t.v[2] - c;

  t.v[3] += ((1<<27)-4);
  c = t.v[3] >> 25;
  t.v[4] = t.v[4] + c;
  c <<= 25;
  t.v[3]  = t.v[3] - c;
  
  t.v[4] += ((1<<27)-4);
  c = t.v[4] >> 25;
  t.v[0] += c;
  c <<= 25;
  t.v[4] -= c;

  c = t.v[0] >> 26;
  t.v[1] += c;
  c <<= 26;
  t.v[0] -= c;

  c = t.v[1] >> 25;
  t.v[2] += c;
  c <<= 25;
  t.v[1] -= c;

  c = t.v[2] >> 26;
  t.v[3] += c;
  c <<= 26;
  t.v[2] -= c;

  c = t.v[3] >> 25;
  t.v[4] += c;
  c <<= 25;
  t.v[3] -= c;

  c = t.v[4] >> 25;
  t.v[0] += c;
  c <<= 25;
  t.v[4] -= c;

  c = t.v[0] >> 26;
  t.v[1] += c;
  c <<= 26;
  t.v[0] -= c;

  r[ 0] =  t.v[0]     & 0xff;
  r[ 1] = (t.v[0]>>8) & 0xff;
  r[ 2] = (t.v[0]>>16)& 0xff;
  r[ 3] = (t.v[0]>>24) | ((t.v[1] & 0x3f) << 2);
  r[ 4] = (t.v[1]>>6) & 0xff;
  r[ 5] = (t.v[1]>>14)& 0xff;
  r[ 6] = (t.v[1]>>22) | ((t.v[2] & 0x1f) << 3);
  r[ 7] = (t.v[2]>>5) & 0xff;
  r[ 8] = (t.v[2]>>13) & 0xff;
  r[ 9] = (t.v[2]>>21) | ((t.v[3] & 0x7) << 5);
  r[10] = (t.v[3]>>3) & 0xff;
  r[11] = (t.v[3]>>11) & 0xff;
  r[12] = (t.v[3]>>19) | ((t.v[4] & 0x3) << 6);
  r[13] = (t.v[4]>>2) & 0xff;
  r[14] = (t.v[4]>>10) & 0xff;
  r[15] = (t.v[4]>>18) & 0x7f;
}

static void gfe_add(gfe *r, const gfe *x, const gfe *y)
{
  r->v[0] = x->v[0] + y->v[0];
  r->v[1] = x->v[1] + y->v[1];
  r->v[2] = x->v[2] + y->v[2];
  r->v[3] = x->v[3] + y->v[3];
  r->v[4] = x->v[4] + y->v[4];
}

static void gfe_sub(gfe *r, const gfe *x, const gfe *y)
{
  r->v[0] = x->v[0] - y->v[0];
  r->v[1] = x->v[1] - y->v[1];
  r->v[2] = x->v[2] - y->v[2];
  r->v[3] = x->v[3] - y->v[3];
  r->v[4] = x->v[4] - y->v[4];
}


void gfe_hadamard(gfe r[4])
{
  gfe a,b,c,d;

  gfe_add(&a,&r[0],&r[1]);
  gfe_add(&b,&r[2],&r[3]);
  gfe_sub(&c,&r[0],&r[1]);
  gfe_sub(&d,&r[2],&r[3]);

  gfe_add(&r[0],&a,&b);
  gfe_sub(&r[1],&a,&b);
  gfe_add(&r[2],&c,&d);
  gfe_sub(&r[3],&c,&d);
}

typedef signed long long i64;

void gfe_mul(gfe *r, const gfe *a, const gfe *b)
{

  i64 t[5],c;

  crypto_int32 _2a_[5];
  _2a_[1] = a->v[1]<<1;
  _2a_[2] = a->v[2]<<1;
  _2a_[3] = a->v[3]<<1;
  _2a_[4] = a->v[4]<<1;

  t[0] = (i64)a->v[0]*b->v[0] + (i64)_2a_[1]*b->v[4] + (i64)_2a_[2]*b->v[3] + (i64)_2a_[3]*b->v[2] + (i64)_2a_[4]*b->v[1];
  t[1] = (i64)a->v[0]*b->v[1] + (i64)a->v[1]*b->v[0] + (i64)a->v[2]*b->v[4] + (i64)_2a_[3]*b->v[3] + (i64)a->v[4]*b->v[2];
  t[2] = (i64)a->v[0]*b->v[2] + (i64)_2a_[1]*b->v[1] + (i64)a->v[2]*b->v[0] + (i64)_2a_[3]*b->v[4] + (i64)_2a_[4]*b->v[3];
  t[3] = (i64)a->v[0]*b->v[3] + (i64)a->v[1]*b->v[2] + (i64)a->v[2]*b->v[1] + (i64)a->v[3]*b->v[0] + (i64)a->v[4]*b->v[4];
  t[4] = (i64)a->v[0]*b->v[4] + (i64)_2a_[1]*b->v[3] + (i64)a->v[2]*b->v[2] + (i64)_2a_[3]*b->v[1] + (i64)a->v[4]*b->v[0];

  c = t[0] >> 26;
  t[1] += c;
  c <<= 26;
  t[0] -= c;
  c = t[1] >> 25;
  t[2] += c;
  c <<= 25;
  t[1] -= c;
  c = t[2] >> 26;
  t[3] += c;
  c <<= 26;
  t[2] -= c;
  c = t[3] >> 25;
  t[4] += c;
  c <<= 25;
  t[3] -= c;
  c = t[4] >> 25;
  t[0] += c;
  c <<= 25;
  t[4] -= c;
  c = t[0] >> 26;
  t[1] += c;
  c <<= 26;
  t[0] -= c;

  r->v[0] = t[0];
  r->v[1] = t[1];
  r->v[2] = t[2];
  r->v[3] = t[3];
  r->v[4] = t[4];
}

void gfe_mulconst(gfe *r, const gfe *a, const crypto_int32 cst)
{

  i64 t[5],c;

  t[0] = (i64)a->v[0]*(i64)cst;
  t[1] = (i64)a->v[1]*(i64)cst;
  t[2] = (i64)a->v[2]*(i64)cst;
  t[3] = (i64)a->v[3]*(i64)cst;
  t[4] = (i64)a->v[4]*(i64)cst;

  c = t[0] >> 26;
  t[1] += c;
  c <<= 26;
  t[0] -= c;
  c = t[1] >> 25;
  t[2] += c;
  c <<= 25;
  t[1] -= c;
  c = t[2] >> 26;
  t[3] += c;
  c <<= 26;
  t[2] -= c;
  c = t[3] >> 25;
  t[4] += c;
  c <<= 25;
  t[3] -= c;
  c = t[4] >> 25;
  t[0] += c;
  c <<= 25;
  t[4] -= c;
  c = t[0] >> 26;
  t[1] += c;
  c <<= 26;
  t[0] -= c;

  r->v[0] = t[0];
  r->v[1] = t[1];
  r->v[2] = t[2];
  r->v[3] = t[3];
  r->v[4] = t[4];
}

void gfe_print(const gfe *x)
{
  printf("(%d + ",x->v[0]);
  printf(" %d*2^26 + ",x->v[1]);
  printf(" %d*2^51 + ",x->v[2]);
  printf(" %d*2^77 + ",x->v[3]);
  printf(" %d*2^102 + 2^129-4)%%(2^127-1)",x->v[4]);
}


