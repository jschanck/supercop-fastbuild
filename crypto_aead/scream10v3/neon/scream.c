#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include "tae.h"
#include "sbox.h"
#include "params.h"
#include "helper.h"

#define V(x) (v16qu){x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x}
#define shift_left(x)  ((x) << (v16qu)CV(4))
#define shift_right(x) ((x) >> (v16qu)CV(4))

static void LBox16P(v16qu *X);
static void LBoxInv16P(v16qu *X);

/* Data layout:

   [in[0][0] in[1][0] ...]
   [in[0][2] in[1][2] ...]
   [in[0][4] in[1][4] ...]
   ...
   [in[0][14] in[1][14] ...]
   [in[0][1] in[1][1] ...]
   [in[0][3] in[1][3] ...]
   [in[0][5] in[1][5] ...]
   ...
   [in[0][15] in[1][15] ...]

 */

#define key_idx(j) (((j)>>1)+8*((j)&1))

/* T3 is expandeded Tweakey */
static void encrypt_tweakey (v16qu *restrict x, v16qu *restrict T3) {
#if NSTEPS > 15
#error Round constant not implemented
#endif
#define CST1(i) CV((2199*i)%256)
#define CST2(i) CV(((2199*i)>>8)%256)
  static const v16qu RC1[32] = {
    CST1( 0), CST1( 1), CST1( 2), CST1( 3), CST1( 4), CST1( 5), CST1( 6), CST1( 7),
    CST1( 8), CST1( 9), CST1(10), CST1(11), CST1(12), CST1(13), CST1(14), CST1(15),
    CST1(16), CST1(17), CST1(18), CST1(19), CST1(20), CST1(21), CST1(22), CST1(23),
    CST1(24), CST1(25), CST1(26), CST1(27), CST1(28), CST1(29), CST1(30), CST1(31),
  };
  static const v16qu RC2[32] = {
    CST2( 0), CST2( 1), CST2( 2), CST2( 3), CST2( 4), CST2( 5), CST2( 6), CST2( 7),
    CST2( 8), CST2( 9), CST2(10), CST2(11), CST2(12), CST2(13), CST2(14), CST2(15),
    CST2(16), CST2(17), CST2(18), CST2(19), CST2(20), CST2(21), CST2(22), CST2(23),
    CST2(24), CST2(25), CST2(26), CST2(27), CST2(28), CST2(29), CST2(30), CST2(31),
  };
#undef CST1
#undef CST2

  int t=0;
  int i,j;

  for (i=0; i<NSTEPS; i++) {
    // Key addition
    for (j=0; j<8; j++) {
      x[j] ^= T3[t++];
    }
    SBOX(x);
    // Round constants
    x[0] ^= RC1[2*i];

    // Key addition
    for (j=8; j<16; j++) {
      x[j] ^= T3[t++];
    }
    SBOX((x+8));
    // Round constants
    x[8] ^= RC2[2*i];

    // LBox
    LBox16P(x);

    // Sbox
    SBOX(x);
    // Round constants
    x[0] ^= RC1[2*i+1];

    SBOX((x+8));
    // Round constants
    x[8] ^= RC2[2*i+1];
    
    // LBox
    LBox16P(x);
    
    if (t == 48)
      t=0;
  }

  // Final key addition
  for (j=0; j<16; j++) {
      x[j] ^= T3[t++];
  }
}

static void decrypt_tweakey (v16qu *restrict x, v16qu *restrict T3) {
#if NSTEPS > 15
#error Round constant not implemented
#endif
#define CST1(i) CV((2199*i)%256)
#define CST2(i) CV(((2199*i)>>8)%256)
  static const v16qu RC1[32] = {
    CST1( 0), CST1( 1), CST1( 2), CST1( 3), CST1( 4), CST1( 5), CST1( 6), CST1( 7),
    CST1( 8), CST1( 9), CST1(10), CST1(11), CST1(12), CST1(13), CST1(14), CST1(15),
    CST1(16), CST1(17), CST1(18), CST1(19), CST1(20), CST1(21), CST1(22), CST1(23),
    CST1(24), CST1(25), CST1(26), CST1(27), CST1(28), CST1(29), CST1(30), CST1(31),
  };
  static const v16qu RC2[32] = {
    CST2( 0), CST2( 1), CST2( 2), CST2( 3), CST2( 4), CST2( 5), CST2( 6), CST2( 7),
    CST2( 8), CST2( 9), CST2(10), CST2(11), CST2(12), CST2(13), CST2(14), CST2(15),
    CST2(16), CST2(17), CST2(18), CST2(19), CST2(20), CST2(21), CST2(22), CST2(23),
    CST2(24), CST2(25), CST2(26), CST2(27), CST2(28), CST2(29), CST2(30), CST2(31),
  };
#undef CST1
#undef CST2

  int t=16*((NSTEPS%3)+1);
  int i,j;

  // Final key addition
  for (j=15; j>=0; j--) {
      x[j] ^= T3[--t];
  }

  for (i=NSTEPS-1; i>=0; i--) {
    if (t == 0)
      t=48;

    LBoxInv16P(x);

    x[8] ^= RC2[2*i+1];
    SBOX_Inv((x+8));
    x[0] ^= RC1[2*i+1];
    SBOX_Inv(x);

    LBoxInv16P(x);

    x[8] ^= RC2[2*i];
    SBOX_Inv((x+8));
    for (j=15; j>=8; j--) {
      x[j] ^= T3[--t];
    }
    x[0] ^= RC1[2*i];
    SBOX_Inv(x);
    for (j=7; j>=0; j--) {
      x[j] ^= T3[--t];
    }
  }
}

/* Computes 16 LBoxes (16bit each) in parallel */

static void LBox16P(v16qu *X) {
  int j;
  static const v16qu tables[8] =
    {
      {0x00, 0x6d, 0xe0, 0x8d, 0x24, 0x49, 0xc4, 0xa9, 0xa5, 0xc8, 0x45, 0x28, 0x81, 0xec, 0x61, 0x0c, },
      {0x00, 0xd7, 0x3a, 0xed, 0xe5, 0x32, 0xdf, 0x08, 0xfe, 0x29, 0xc4, 0x13, 0x1b, 0xcc, 0x21, 0xf6, },
      {0x00, 0x5c, 0xa9, 0xf5, 0xb3, 0xef, 0x1a, 0x46, 0xc1, 0x9d, 0x68, 0x34, 0x72, 0x2e, 0xdb, 0x87, },
      {0x00, 0x38, 0x52, 0x6a, 0x7b, 0x43, 0x29, 0x11, 0x96, 0xae, 0xc4, 0xfc, 0xed, 0xd5, 0xbf, 0x87, },
      {0x00, 0x12, 0x6f, 0x7d, 0x1b, 0x09, 0x74, 0x66, 0xb1, 0xa3, 0xde, 0xcc, 0xaa, 0xb8, 0xc5, 0xd7, },
      {0x00, 0x67, 0x7a, 0x1d, 0x70, 0x17, 0x0a, 0x6d, 0x8a, 0xed, 0xf0, 0x97, 0xfa, 0x9d, 0x80, 0xe7, },
      {0x00, 0x4b, 0xaf, 0xe4, 0x33, 0x78, 0x9c, 0xd7, 0x74, 0x3f, 0xdb, 0x90, 0x47, 0x0c, 0xe8, 0xa3, },
      {0x00, 0x46, 0xf1, 0xb7, 0xa1, 0xe7, 0x50, 0x16, 0x7f, 0x39, 0x8e, 0xc8, 0xde, 0x98, 0x2f, 0x69, },
    };

  for (j=0; j<8; j+=2) {

    // Evaluate 2 series LBoxes in parallel, with 16-bit input
    // (X[4*j], X[4*j+2]) and (X[4*j+1], X[4*j+3]) ie (A,B) and (C,D)

#define A (X[j])
#define B (X[j+8])
#define C (X[j+1])
#define D (X[j+9])

#define shuffle(t,x1,x2,y1,y2)	do {			\
      y1 ^= vcombine_u8(vtbl2_u8(t,vget_low_u8(x1)),	\
			vtbl2_u8(t,vget_high_u8(x1)));	\
      y2 ^= vcombine_u8(vtbl2_u8(t,vget_low_u8(x2)),	\
			vtbl2_u8(t,vget_high_u8(x2)));	\
    } while(0);

    v16qu in[4] = {A, B, C, D};
    v16qu t0, t1;

#define READ(t,x) x = t;

    v16qu table;

    READ(tables[0], table);

    t0 = shift_right(in[0]);
    t1 = shift_right(in[2]);

    A  = __builtin_shuffle(table, t0);
    C  = __builtin_shuffle(table, t1);

    READ(tables[1], table);

    B  = __builtin_shuffle(table, t0);
    D  = __builtin_shuffle(table, t1);

    READ(tables[2], table);

    A ^= __builtin_shuffle(table, in[0]);
    C ^= __builtin_shuffle(table, in[2]);

    READ(tables[3], table);

    B ^= __builtin_shuffle(table, in[0]);
    D ^= __builtin_shuffle(table, in[2]);

    READ(tables[4], table);

    t0 = shift_right(in[1]);
    t1 = shift_right(in[3]);

    A ^= __builtin_shuffle(table, t0);
    C ^= __builtin_shuffle(table, t1);

    READ(tables[5], table);

    B ^= __builtin_shuffle(table, t0);
    D ^= __builtin_shuffle(table, t1);

    READ(tables[6], table);

    A ^= __builtin_shuffle(table, in[1]);
    C ^= __builtin_shuffle(table, in[3]);

    READ(tables[7], table);

    B ^= __builtin_shuffle(table, in[1]);
    D ^= __builtin_shuffle(table, in[3]);



    /* t0 = shift_right(in[0]) & 0xf; */
    /* t1 = shift_right(in[2]) & 0xf; */

    /* in[0] &= 0xf; */
    /* in[2] &= 0xf; */

    /* A = B = C = D = (v16qu) CV(0); */

    /* /\* A  = shuffle(tables[0], t0); *\/ */
    /* /\* C  = shuffle(tables[0], t1); *\/ */
    /* shuffle(tables[0], t0, t1, A, C); */

    /* /\* B  = shuffle(tables[1], t0); *\/ */
    /* /\* D  = shuffle(tables[1], t1); *\/ */
    /* shuffle(tables[1], t0, t1, B, D); */

    /* /\* A ^= shuffle(tables[2], in[0]); *\/ */
    /* /\* C ^= shuffle(tables[2], in[2]); *\/ */
    /* shuffle(tables[2], in[0], in[2], A, C); */

    /* /\* B ^= shuffle(tables[3], in[0]); *\/ */
    /* /\* D ^= shuffle(tables[3], in[2]); *\/ */
    /* shuffle(tables[3], in[0], in[2], B, D); */

    /* t0 = shift_right(in[1]) & 0xf; */
    /* t1 = shift_right(in[3]) & 0xf; */

    /* in[1] &= 0xf; */
    /* in[3] &= 0xf; */

    /* /\* A ^= shuffle(tables[4], t0); *\/ */
    /* /\* C ^= shuffle(tables[4], t1); *\/ */
    /* shuffle(tables[4], t0, t1, A, C); */

    /* /\* B ^= shuffle(tables[5], t0); *\/ */
    /* /\* D ^= shuffle(tables[5], t1); *\/ */
    /* shuffle(tables[5], t0, t1, B, D); */

    /* /\* A ^= shuffle(tables[6], in[1]); *\/ */
    /* /\* C ^= shuffle(tables[6], in[3]); *\/ */
    /* shuffle(tables[6], in[1], in[3], A, C); */

    /* /\* B ^= shuffle(tables[7], in[1]); *\/ */
    /* /\* D ^= shuffle(tables[7], in[3]); *\/ */
    /* shuffle(tables[7], in[1], in[3], B, D); */

#undef A
#undef B
#undef C
#undef D
  }
}

static void LBoxInv16P(v16qu *X) {
  int j;
  static const v16qu tables[8] =
    {
      {0x00, 0x05, 0xf6, 0xf3, 0x39, 0x3c, 0xcf, 0xca, 0xae, 0xab, 0x58, 0x5d, 0x97, 0x92, 0x61, 0x64, },
      {0x00, 0xdc, 0xae, 0x72, 0x29, 0xf5, 0x87, 0x5b, 0x82, 0x5e, 0x2c, 0xf0, 0xab, 0x77, 0x05, 0xd9, },
      {0x00, 0x9e, 0x04, 0x9a, 0xd1, 0x4f, 0xd5, 0x4b, 0x13, 0x8d, 0x17, 0x89, 0xc2, 0x5c, 0xc6, 0x58, },
      {0x00, 0xe7, 0x77, 0x90, 0x2a, 0xcd, 0x5d, 0xba, 0x63, 0x84, 0x14, 0xf3, 0x49, 0xae, 0x3e, 0xd9, },
      {0x00, 0xbf, 0xe5, 0x5a, 0xd6, 0x69, 0x33, 0x8c, 0xd8, 0x67, 0x3d, 0x82, 0x0e, 0xb1, 0xeb, 0x54, },
      {0x00, 0x6a, 0x7e, 0x14, 0x7b, 0x11, 0x05, 0x6f, 0x86, 0xec, 0xf8, 0x92, 0xfd, 0x97, 0x83, 0xe9, },
      {0x00, 0x54, 0xbe, 0xea, 0xd8, 0x8c, 0x66, 0x32, 0xa5, 0xf1, 0x1b, 0x4f, 0x7d, 0x29, 0xc3, 0x97, },
      {0x00, 0x1e, 0xb9, 0xa7, 0x19, 0x07, 0xa0, 0xbe, 0xa8, 0xb6, 0x11, 0x0f, 0xb1, 0xaf, 0x08, 0x16, },
    };

  for (j=0; j<8; j+=2) {

    // Evaluate 2 series LBoxes in parallel, with 16-bit input
    // (X[4*j], X[4*j+2]) and (X[4*j+1], X[4*j+3]) ie (A,B) and (C,D)

#define A (X[j])
#define B (X[j+8])
#define C (X[j+1])
#define D (X[j+9])

    v16qu in[4] = {A, B, C, D};
    v16qu t0, t1;

#define READ(t,x) x = t;

    v16qu table;

    READ(tables[0], table);

    t0 = shift_right(in[0]);
    t1 = shift_right(in[2]);

    A  = __builtin_shuffle(table, t0);
    C  = __builtin_shuffle(table, t1);

    READ(tables[1], table);

    B  = __builtin_shuffle(table, t0);
    D  = __builtin_shuffle(table, t1);

    READ(tables[2], table);

    A ^= __builtin_shuffle(table, in[0]);
    C ^= __builtin_shuffle(table, in[2]);

    READ(tables[3], table);

    B ^= __builtin_shuffle(table, in[0]);
    D ^= __builtin_shuffle(table, in[2]);

    READ(tables[4], table);

    t0 = shift_right(in[1]);
    t1 = shift_right(in[3]);

    A ^= __builtin_shuffle(table, t0);
    C ^= __builtin_shuffle(table, t1);

    READ(tables[5], table);

    B ^= __builtin_shuffle(table, t0);
    D ^= __builtin_shuffle(table, t1);

    READ(tables[6], table);

    A ^= __builtin_shuffle(table, in[1]);
    C ^= __builtin_shuffle(table, in[3]);

    READ(tables[7], table);

    B ^= __builtin_shuffle(table, in[1]);
    D ^= __builtin_shuffle(table, in[3]);

#undef READ
#undef A
#undef B
#undef C
#undef D
  }
}


/* Tweakey scheduling */

void tweak_expand(v16qu T[16], v16qu K[16], v16qu T3[48]) {
  int i;

  for (i = 0; i<16; i++)
    T3[i] = T[i];

  for (i = 0; i<8; i++) {
    T3[16+key_idx(i)]   = T[key_idx(i)] ^ T[key_idx(i+8)];
    T3[16+key_idx(i+8)] = T[key_idx(i)];
    T3[32+key_idx(i)]   = T[key_idx(i+8)];
    T3[32+key_idx(i+8)] = T[key_idx(i)] ^ T[key_idx(i+8)];
  }

  for (i = 0; i<48; i++)
    T3[i] ^= K[i%16];
}

void tweakey_schedule(const uint8_t *restrict k,
		      const uint8_t *restrict n,
		      uint8_t ctrl,
		      v16qu *restrict TK) {
  v16qu *K = &TK[3*16];
  v16qu *T = &TK[4*16];
  v16qu C= {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
  int j;

  for (j=0; j<16; j++) {
    K[key_idx(j)] = (v16qu) CV(k[j]);
  }
  for (j=0; j<11; j++) {
    T[key_idx(j)] = (v16qu) CV(n[j]);
  }

  T[key_idx(15)] = (v16qu) CV(ctrl);
  T[key_idx(14)] = C;
  T[key_idx(13)] = (v16qu) CV(0);
  T[key_idx(12)] = (v16qu) CV(0);
  T[key_idx(11)] = (v16qu) CV(0);

  tweak_expand(T, K, TK);
}

// Asumes i > 7
#define UPDATE(i) do {							\
    TK[key_idx(i)   ] = T[key_idx(i)]^K[key_idx(i)];			\
    TK[key_idx(i-8)+16] = T[key_idx(i)]^T[key_idx(i-8)]^K[key_idx(i-8)];			\
    TK[key_idx(i-8)+32] = T[key_idx(i)]^K[key_idx(i-8)];			\
    TK[key_idx(i)+32] = T[key_idx(i)]^T[key_idx(i-8)]^K[key_idx(i)];	\
  } while(0)

void tweakey_increment(v16qu *TK, size_t idx) {
  v16qu *K = &TK[3*16];
  v16qu *T = &TK[4*16];

  T[key_idx(14)] += V(16);
  UPDATE(14);
  if (((idx+256) & 0xfff) == 0) {
    T[key_idx(13)] += V(1);
    UPDATE(13);
    if (((idx+256) & 0xfffff) == 0) {
      T[key_idx(12)] += V(1);
      UPDATE(12);
      if (((idx+256) & 0xfffffff) == 0) {
	T[key_idx(11)] += V(1);
	UPDATE(11);
      }
    }
  }
}

void tweakey_set(v16qu *TK, int i, int j, uint8_t x) {
  v16qu *K = &TK[3*16];
  v16qu *T = &TK[4*16];

  T[key_idx(j)][i] = x;
  UPDATE(j);
}

#undef UPDATE
