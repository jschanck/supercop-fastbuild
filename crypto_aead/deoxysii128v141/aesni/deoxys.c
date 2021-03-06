/*
 * Deoxys-II-128 Optimized AES-NI C Implementation
 * 
 * Copyright 2016:
 *     Ivica Nikolic <inikolic@ntu.edu.sg>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <xmmintrin.h>              /* SSE instructions and _mm_malloc */
#include <emmintrin.h>              /* SSE2 instructions               */
#include <wmmintrin.h>              /* SSSE3 instructions              */
#include <tmmintrin.h>              /* SSSE3 instructions              */
#include "tweakable-cipher.macros"
#include "ae.macros"


/*
** Constant-time memcmp function
*/
static int memcmp_const(const void * a, const void *b, const size_t size)  {

  size_t i;
  unsigned char result = 0;
  const unsigned char *_a = (const unsigned char *) a;
  const unsigned char *_b = (const unsigned char *) b;

  for (i = 0; i < size; i++) {
    result |= _a[i] ^ _b[i];
  }

  /* returns 0 if equal, nonzero otherwise */
  return result; 
}    

int deoxys_aead_encrypt(const unsigned char *ass_data, unsigned long long int ass_data_len,
                                  const unsigned char *message, unsigned long long int m_len,
                                  const unsigned char *key, const unsigned char *nonce,
                                  unsigned char *ciphertext, unsigned long long int *c_len)
{

    unsigned long long i,j;
    unsigned char last_block[16];    
    const __m128i EIGHT			= constant8( 0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,8);
    const __m128i SEIGHT		= constant8( 0,0,0,0,  0,0,0,0,  8,0,0,0,  0,0,0,0);
    const __m128i STAGE_MASK   		= constant8( 0x0f    ,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff );
    const __m128i BLOCK_MASK   		= constant8( 0xff    ,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xf0,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );      
    const __m128i MSB_AD  	 	= constant8( (0x2<<4),0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );    
    const __m128i MSB_AD_LAST	 	= constant8( (0x6<<4),0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );    
    const __m128i MSB_M  	 	= constant8( (0x0<<4),0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );    
    const __m128i MSB_M_LAST_NONZERO 	= constant8( (0x4<<4),0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );    
    const __m128i PERM_MASK		= constant8( 0,1,2,3,4,5,6,7,  15,14,13,12,11,10,9,8 ); 
    const __m128i H_PERMUTATION  	= constant8( 7,0,13,10, 11,4,1,14, 15,8,5,2, 3,12,9,6 );   	
    __m128i ADD_ONE			= constant8( 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1 );   	
    __m128i RCONS[17];
    __m128i tmp;
    __m128i subkeys[17], tsubkeys[17];
    __m128i States[8];
    __m128i State;
    __m128i Tweak 			 = constant32( 0, 0, 0, 0);
    __m128i tTweak;
    __m128i Auth  	 		= constant32( 0 , 0 , 0 , 0 );
    static const unsigned char rcon[17] = {0x2f,0x5e,0xbc,0x63,0xc6,0x97,0x35,0x6a,0xd4,0xb3,0x7d,0xfa,0xef,0xc5,0x91,0x39,0x72};
  __m128i tw_c[17][8];
  __m128i mask_top_7_bits        = constant8_same(0xfe);
  __m128i mask_bottom_1_bit      = constant8_same(0x01);
  __m128i nonce_plaintext;
        

  // Set up tweak constants
  tmp = ADD_ONE;
  for(i=0; i<15; i++){
    tw_c[i][0] = constant8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
    tw_c[i][1] = add8(tw_c[i][0], tmp );
    tw_c[i][2] = add8(tw_c[i][1], tmp );
    tw_c[i][3] = add8(tw_c[i][2], tmp );
    tw_c[i][4] = add8(tw_c[i][3], tmp );
    tw_c[i][5] = add8(tw_c[i][4], tmp );
    tw_c[i][6] = add8(tw_c[i][5], tmp );
    tw_c[i][7] = add8(tw_c[i][6], tmp );
    tmp = permute( tmp, H_PERMUTATION );
  }


  // Set up the round constants and keys for AES
  for(i=0; i<15;i++){
    RCONS[i] = constant8( 1,2,4,8, rcon[i], rcon[i], rcon[i], rcon[i],  0,0,0,0,  0,0,0,0);
  }
  TWEAKEY_SCHEDULE2( tsubkeys,subkeys, key);


    
    /*
     * New :
     * RT - Round Tweaks
     * OT - Value of 1 according to the permutation
     */
    __m128i RT[8], OT[8];
    tmp = EIGHT;
    for( i=0; i<8; i++){
        OT[i] = tmp;
        tmp = permute( tmp, H_PERMUTATION );
    }




    /*
     *
     *			 Associated data
     *
     *
     */

    if(ass_data_len) {
            SET_STAGE_IN_TWEAK(Tweak, MSB_AD);
            i = 0;
            SET_BLOCK_NO_IN_TWEAK( Tweak, i);
            RT[0] = Tweak;
            RT[1] = permute( RT[0], H_PERMUTATION );
            RT[2] = permute( RT[1], H_PERMUTATION );
            RT[3] = permute( RT[2], H_PERMUTATION );
            RT[4] = permute( RT[3], H_PERMUTATION );
            RT[5] = permute( RT[4], H_PERMUTATION );
            RT[6] = permute( RT[5], H_PERMUTATION );
            RT[7] = permute( RT[6], H_PERMUTATION );

            // Absorb 8 blocks in parallel
            while (16*(i+8) <= ass_data_len) {
              PROCESS_8_ASS_DATA_BLOCKS( States, ass_data + 16 * i , Auth );
              if(248 == ( i % 256) ){
                  ADD_TO_TWEAK( RT[ 0], SEIGHT );
                  RT[1] = permute( RT[0], H_PERMUTATION );
                  RT[2] = permute( RT[1], H_PERMUTATION );
                  RT[3] = permute( RT[2], H_PERMUTATION );
                  RT[4] = permute( RT[3], H_PERMUTATION );
                  RT[5] = permute( RT[4], H_PERMUTATION );
                  RT[6] = permute( RT[5], H_PERMUTATION );
                  RT[7] = permute( RT[6], H_PERMUTATION );
              }
              else{

                  RT[ 0] = add8( RT[ 0], OT[ 0] );
                  RT[ 1] = add8( RT[ 1], OT[ 1] );
                  RT[ 2] = add8( RT[ 2], OT[ 2] );
                  RT[ 3] = add8( RT[ 3], OT[ 3] );
                  RT[ 4] = add8( RT[ 4], OT[ 4] );
                  RT[ 5] = add8( RT[ 5], OT[ 5] );
                  RT[ 6] = add8( RT[ 6], OT[ 6] );
                  RT[ 7] = add8( RT[ 7], OT[ 7] );
              }

              i += 8;
            }

            // Absorb the remaining full 16-byte blocks
            while (16*(i+1) <= ass_data_len) {
               State = load( (__m128i *) ( ass_data + 16 * i ) );
               SET_BLOCK_NO_IN_TWEAK( Tweak, i);
               AES( State, subkeys, Tweak, tTweak );
               Auth = xor( Auth , State );

               i++;
            }

            /* Last block if incomplete */
            if ( ass_data_len > 16*i )  {

                /* Prepare the last padded block */
                memset(last_block, 0, 16);
                memcpy(last_block, ass_data+16*i, ass_data_len-16*i);
                last_block[ass_data_len-16*i]=0x80;

                /* Encrypt the last block */
                SET_STAGE_IN_TWEAK(Tweak, MSB_AD_LAST);
                SET_BLOCK_NO_IN_TWEAK( Tweak, i);
                State = load( (__m128i *) ( last_block ) );
                AES( State, subkeys, Tweak, tTweak );

                /* Update the Auth value */
                Auth = xor ( Auth, State );
            }
    }



    /*
     *
     *
     * 			Message : First Pass
     *
     *
     */
    
    Tweak = xor( Tweak , Tweak ); // Tweak  =  0
    
    SET_STAGE_IN_TWEAK(Tweak, MSB_M);

    i=0;
    SET_BLOCK_NO_IN_TWEAK( Tweak, i);
    RT[0] = Tweak;
    RT[1] = permute( RT[0], H_PERMUTATION );
    RT[2] = permute( RT[1], H_PERMUTATION );
    RT[3] = permute( RT[2], H_PERMUTATION );
    RT[4] = permute( RT[3], H_PERMUTATION );
    RT[5] = permute( RT[4], H_PERMUTATION );
    RT[6] = permute( RT[5], H_PERMUTATION );
    RT[7] = permute( RT[6], H_PERMUTATION );

    // Absorb all 128-byte blocks (8 in parallel)
    while (16*(i+8) <= m_len) {

          PROCESS_8_MESSAGE_BLOCKS_FIRST_PASS( States, message + 16 * i  , Auth );

          if(248 == ( i % 256) ){
              ADD_TO_TWEAK( RT[ 0], SEIGHT );
              RT[1] = permute( RT[0], H_PERMUTATION );
              RT[2] = permute( RT[1], H_PERMUTATION );
              RT[3] = permute( RT[2], H_PERMUTATION );
              RT[4] = permute( RT[3], H_PERMUTATION );
              RT[5] = permute( RT[4], H_PERMUTATION );
              RT[6] = permute( RT[5], H_PERMUTATION );
              RT[7] = permute( RT[6], H_PERMUTATION );
          }
          else{

              RT[ 0] = add8( RT[ 0], OT[ 0] );
              RT[ 1] = add8( RT[ 1], OT[ 1] );
              RT[ 2] = add8( RT[ 2], OT[ 2] );
              RT[ 3] = add8( RT[ 3], OT[ 3] );
              RT[ 4] = add8( RT[ 4], OT[ 4] );
              RT[ 5] = add8( RT[ 5], OT[ 5] );
              RT[ 6] = add8( RT[ 6], OT[ 6] );
              RT[ 7] = add8( RT[ 7], OT[ 7] );
          }
          i += 8;
    }
    
    // Absorb the remaining full 16-byte blocks
    while (16*(i+1) <= m_len) {
	  State = load( (__m128i *) ( message + 16 * i ) );
	  SET_BLOCK_NO_IN_TWEAK( Tweak, i);	
	  AES( State, subkeys, Tweak, tTweak );
          Auth = xor( Auth, State );
	  i++;
    }
        
    // if last message block is incomplete
    if( m_len != 16 * i ){

        SET_STAGE_IN_TWEAK(Tweak, MSB_M_LAST_NONZERO);
        SET_BLOCK_NO_IN_TWEAK( Tweak, i);
        memset(last_block, 0, 16);
        memcpy(last_block, message +16*i, m_len-16*i);
        last_block[m_len-16*i]=0x80;
        tmp = load( (__m128i *) ( last_block ) );
        AES( tmp , subkeys, Tweak, tTweak );
        Auth = xor( tmp, Auth);
	    
    }


    last_block[0] = 1<<4;
    memcpy(last_block+1,nonce,15);
    Tweak = load( (__m128i *) ( last_block ) );
    AES( Auth, subkeys, Tweak, tTweak );

    
    store( last_block , Auth );
    memcpy( ciphertext+m_len , last_block , 16 );   // it overwrites here
        
    *c_len=m_len+16;




    /*
     *
     *
     * 			Message : Second Pass
     *
     *
     */

    // Compute subtweaks for 1||tag and add them to the subkeys
    Tweak = xor ( and (Auth, constant32(0xffffff7f,0xffffffff,0xffffffff,0xffffffff) ), constant32(0x00000080,0,0,0) );

    for( i=0; i< 15; i++){
        subkeys[i] = xor( subkeys[i], Tweak);
        Tweak = permute( Tweak , H_PERMUTATION);
    }



    // Assume that the remaining tweaks are 0,1,...

    last_block[0] = 0;
    for( i=0; i<15; i++)
        last_block[i+1] = nonce[i];
    nonce_plaintext = load( (__m128i *) ( last_block ) );


    i = 0;
    Tweak = xor( Tweak,Tweak);
    RT[0] = Tweak;
    RT[1] = permute( RT[0], H_PERMUTATION );
    RT[2] = permute( RT[1], H_PERMUTATION );
    RT[3] = permute( RT[2], H_PERMUTATION );
    RT[4] = permute( RT[3], H_PERMUTATION );
    RT[5] = permute( RT[4], H_PERMUTATION );
    RT[6] = permute( RT[5], H_PERMUTATION );
    RT[7] = permute( RT[6], H_PERMUTATION );


    // Absorb 8 blocks in parallel
    while (16*(i+8) <= m_len) {
        PROCESS_8_MESSAGE_BLOCKS_SECOND_PASS( States, nonce_plaintext , message + 16*i, ciphertext + 16*i);

      if(248 == ( i % 256) ){
          ADD_TO_TWEAK( RT[ 0], SEIGHT );
          RT[1] = permute( RT[0], H_PERMUTATION );
          RT[2] = permute( RT[1], H_PERMUTATION );
          RT[3] = permute( RT[2], H_PERMUTATION );
          RT[4] = permute( RT[3], H_PERMUTATION );
          RT[5] = permute( RT[4], H_PERMUTATION );
          RT[6] = permute( RT[5], H_PERMUTATION );
          RT[7] = permute( RT[6], H_PERMUTATION );
      }
      else{

          RT[ 0] = add8( RT[ 0], OT[ 0] );
          RT[ 1] = add8( RT[ 1], OT[ 1] );
          RT[ 2] = add8( RT[ 2], OT[ 2] );
          RT[ 3] = add8( RT[ 3], OT[ 3] );
          RT[ 4] = add8( RT[ 4], OT[ 4] );
          RT[ 5] = add8( RT[ 5], OT[ 5] );
          RT[ 6] = add8( RT[ 6], OT[ 6] );
          RT[ 7] = add8( RT[ 7], OT[ 7] );
      }

      i += 8;
    }


    // Absorb the remaining full 16-byte blocks
    while (16*(i+1) <= m_len) {
       State = nonce_plaintext;
       SET_BLOCK_NO_IN_TWEAK( Tweak, i);

       AES( State, subkeys, Tweak, tTweak );

       State = xor( State, load( (__m128i *) (  message + 16 * i   ) ) );
       store( ciphertext + 16*i , State );
       i++;


    }

    // if last message block is incomplete
    if( m_len != 16 * i ){
        State = nonce_plaintext;
        SET_BLOCK_NO_IN_TWEAK( Tweak, i);
        AES( State, subkeys, Tweak, tTweak );
        store( last_block, State);
        for( j=0; j<m_len-16*i; j++)
            ciphertext[ 16*i + j] = message[ 16*i + j ] ^ last_block[j] ;

    }




    
    return 0;

}


int deoxys_aead_decrypt(const unsigned char *ass_data, unsigned long long ass_data_len,
                        unsigned char *message, unsigned long long int *m_len,
                        const unsigned char *key, const unsigned char *nonce,
                        const unsigned char *c, unsigned long long c_len
                        )
{

    unsigned long long int i,j;
    unsigned char last_block[16];
    const __m128i EIGHT			= constant8( 0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,8);
    const __m128i SEIGHT		= constant8( 0,0,0,0,  0,0,0,0,  8,0,0,0,  0,0,0,0);
    const __m128i STAGE_MASK   		= constant8( 0x0f    ,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff );
    const __m128i BLOCK_MASK   		= constant8( 0xff    ,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xf0,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );
    const __m128i MSB_AD  	 	= constant8( (0x2<<4),0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );
    const __m128i MSB_AD_LAST	 	= constant8( (0x6<<4),0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );
    const __m128i MSB_M  	 	= constant8( (0x0<<4),0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );
    const __m128i MSB_M_LAST_NONZERO 	= constant8( (0x4<<4),0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 );
    const __m128i PERM_MASK		= constant8( 0,1,2,3,4,5,6,7,  15,14,13,12,11,10,9,8 );
    const __m128i H_PERMUTATION  	= constant8( 7,0,13,10, 11,4,1,14, 15,8,5,2, 3,12,9,6 );
   __m128i ADD_ONE			= constant8( 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1 );
    __m128i RCONS[15];
    __m128i tmp;
    __m128i subkeys[15], tsubkeys[15];
    __m128i States[8];
    __m128i State;
    __m128i Tweak 			 = constant32( 0, 0, 0, 0);
    __m128i tTweak;
    __m128i Auth  	 		= constant32( 0 , 0 , 0 , 0 );
    static const unsigned char rcon[17] = {0x2f,0x5e,0xbc,0x63,0xc6,0x97,0x35,0x6a,0xd4,0xb3,0x7d,0xfa,0xef,0xc5,0x91,0x39,0x72};
    __m128i mask_top_7_bits        = constant8_same(0xfe);
    __m128i mask_bottom_1_bit      = constant8_same(0x01);
    __m128i nonce_plaintext, Tag;
    __m128i tw_c[15][8];


    // Set up tweak constants
    tmp = ADD_ONE;
    for(i=0; i<15; i++){
      tw_c[i][0] = constant8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
      tw_c[i][1] = add8(tw_c[i][0], tmp );
      tw_c[i][2] = add8(tw_c[i][1], tmp );
      tw_c[i][3] = add8(tw_c[i][2], tmp );
      tw_c[i][4] = add8(tw_c[i][3], tmp );
      tw_c[i][5] = add8(tw_c[i][4], tmp );
      tw_c[i][6] = add8(tw_c[i][5], tmp );
      tw_c[i][7] = add8(tw_c[i][6], tmp );
      tmp = permute( tmp, H_PERMUTATION );
    }

    // Set up the round keys for AES
    for(i=0; i<15;i++){
      RCONS[i] = constant8( 1,2,4,8, rcon[i], rcon[i], rcon[i], rcon[i],  0,0,0,0,  0,0,0,0);
    }
    TWEAKEY_SCHEDULE2( tsubkeys,subkeys, key);

    /*
     * New :
     * RT - Round Tweaks
     * OT - Value of 1 according to the permutation
     */
    __m128i RT[8],OT[8];
    tmp = EIGHT;
    for( i=0; i<8; i++){
        OT[i] = tmp;
        tmp = permute( tmp, H_PERMUTATION );
    }


    /*
     *
     *			 Associated data
     *
     *
     */

    if(ass_data_len) {
            SET_STAGE_IN_TWEAK(Tweak, MSB_AD);
            i = 0;
            SET_BLOCK_NO_IN_TWEAK( Tweak, i);
            RT[0] = Tweak;
            RT[1] = permute( RT[0], H_PERMUTATION );
            RT[2] = permute( RT[1], H_PERMUTATION );
            RT[3] = permute( RT[2], H_PERMUTATION );
            RT[4] = permute( RT[3], H_PERMUTATION );
            RT[5] = permute( RT[4], H_PERMUTATION );
            RT[6] = permute( RT[5], H_PERMUTATION );
            RT[7] = permute( RT[6], H_PERMUTATION );

            // Absorb 8 blocks in parallel
            while (16*(i+8) <= ass_data_len) {
              PROCESS_8_ASS_DATA_BLOCKS( States, ass_data + 16 * i , Auth );
              if(248 == ( i % 256) ){
                  ADD_TO_TWEAK( RT[ 0], SEIGHT );
                  RT[1] = permute( RT[0], H_PERMUTATION );
                  RT[2] = permute( RT[1], H_PERMUTATION );
                  RT[3] = permute( RT[2], H_PERMUTATION );
                  RT[4] = permute( RT[3], H_PERMUTATION );
                  RT[5] = permute( RT[4], H_PERMUTATION );
                  RT[6] = permute( RT[5], H_PERMUTATION );
                  RT[7] = permute( RT[6], H_PERMUTATION );
              }
              else{

                  RT[ 0] = add8( RT[ 0], OT[ 0] );
                  RT[ 1] = add8( RT[ 1], OT[ 1] );
                  RT[ 2] = add8( RT[ 2], OT[ 2] );
                  RT[ 3] = add8( RT[ 3], OT[ 3] );
                  RT[ 4] = add8( RT[ 4], OT[ 4] );
                  RT[ 5] = add8( RT[ 5], OT[ 5] );
                  RT[ 6] = add8( RT[ 6], OT[ 6] );
                  RT[ 7] = add8( RT[ 7], OT[ 7] );
              }

              i += 8;
            }

            // Absorb the remaining full 16-byte blocks
            while (16*(i+1) <= ass_data_len) {
               State = load( (__m128i *) ( ass_data + 16 * i ) );
               SET_BLOCK_NO_IN_TWEAK( Tweak, i);

               AES( State, subkeys, Tweak, tTweak );
               Auth = xor( Auth , State );

               i++;
            }

            /* Last block if incomplete */
            if ( ass_data_len > 16*i )  {

                /* Prepare the last padded block */
                memset(last_block, 0, 16);
                memcpy(last_block, ass_data+16*i, ass_data_len-16*i);
                last_block[ass_data_len-16*i]=0x80;

                /* Encrypt the last block */
                    SET_STAGE_IN_TWEAK(Tweak, MSB_AD_LAST);
                    SET_BLOCK_NO_IN_TWEAK( Tweak, i);
                    State = load( (__m128i *) ( last_block ) );
                    AES( State, subkeys, Tweak, tTweak );

                /* Update the Auth value */
                Auth = xor ( Auth, State );
            }
    }



    /*
     *
     *
     * 			Message : Second Pass
     *
     *
     */

    c_len -= 16;
    *m_len = c_len;

    memcpy( last_block, c + c_len, 16);
    Tag = load( (__m128i *) ( last_block ) );
    Tweak = xor ( and (Tag, constant32(0xffffff7f,0xffffffff,0xffffffff,0xffffffff) ), constant32(0x00000080,0,0,0) );

    for( i=0; i< 15; i++){
        subkeys[i] = xor( subkeys[i], Tweak);
        Tweak = permute( Tweak , H_PERMUTATION);
    }



    // Assume that the remaining tweaks are 0,1,...

    last_block[0] = 0;
    for( i=0; i<15; i++)
        last_block[i+1] = nonce[i];
    nonce_plaintext = load( (__m128i *) ( last_block ) );


    i = 0;
    Tweak = xor( Tweak,Tweak);
    RT[0] = Tweak;
    RT[1] = permute( RT[0], H_PERMUTATION );
    RT[2] = permute( RT[1], H_PERMUTATION );
    RT[3] = permute( RT[2], H_PERMUTATION );
    RT[4] = permute( RT[3], H_PERMUTATION );
    RT[5] = permute( RT[4], H_PERMUTATION );
    RT[6] = permute( RT[5], H_PERMUTATION );
    RT[7] = permute( RT[6], H_PERMUTATION );



    // Absorb 8 blocks in parallel
    while (16*(i+8) <= c_len) {

      PROCESS_8_MESSAGE_BLOCKS_SECOND_PASS( States, nonce_plaintext , c + 16*i , message + 16*i);

      if(248 == ( i % 256) ){
          ADD_TO_TWEAK( RT[ 0], SEIGHT );
          RT[1] = permute( RT[0], H_PERMUTATION );
          RT[2] = permute( RT[1], H_PERMUTATION );
          RT[3] = permute( RT[2], H_PERMUTATION );
          RT[4] = permute( RT[3], H_PERMUTATION );
          RT[5] = permute( RT[4], H_PERMUTATION );
          RT[6] = permute( RT[5], H_PERMUTATION );
          RT[7] = permute( RT[6], H_PERMUTATION );
      }
      else{

          RT[ 0] = add8( RT[ 0], OT[ 0] );
          RT[ 1] = add8( RT[ 1], OT[ 1] );
          RT[ 2] = add8( RT[ 2], OT[ 2] );
          RT[ 3] = add8( RT[ 3], OT[ 3] );
          RT[ 4] = add8( RT[ 4], OT[ 4] );
          RT[ 5] = add8( RT[ 5], OT[ 5] );
          RT[ 6] = add8( RT[ 6], OT[ 6] );
          RT[ 7] = add8( RT[ 7], OT[ 7] );
      }

      i += 8;
    }




    // Absorb the remaining full 16-byte blocks
    while (16*(i+1) <= c_len) {
       State = nonce_plaintext;

       SET_BLOCK_NO_IN_TWEAK( Tweak, i);
       AES( State, subkeys, Tweak, tTweak );
       State = xor( State, load( (__m128i *) (  c + 16 * i   ) ) );
       store( message + 16*i , State );
       i++;


    }


    // if last message block is incomplete
    if( c_len != 16 * i ){
        State = nonce_plaintext;
        SET_BLOCK_NO_IN_TWEAK( Tweak, i);
        AES( State, subkeys, Tweak, tTweak );
        store( last_block, State);
        for( j=0; j<c_len-16*i ; j++)
            message[ 16*i + j] = c[ 16*i + j ] ^ last_block[j] ;
    }


    *m_len = c_len;


    /*
     *
     *
     * 			Message : First Pass
     *
     *
     */
    TWEAKEY_SCHEDULE2( tsubkeys,subkeys, key);


    Tweak = xor( Tweak , Tweak ); // Tweak  =  0

    SET_STAGE_IN_TWEAK(Tweak, MSB_M);

    i=0;
    SET_BLOCK_NO_IN_TWEAK( Tweak, i);
    RT[0] = Tweak;
    RT[1] = permute( RT[0], H_PERMUTATION );
    RT[2] = permute( RT[1], H_PERMUTATION );
    RT[3] = permute( RT[2], H_PERMUTATION );
    RT[4] = permute( RT[3], H_PERMUTATION );
    RT[5] = permute( RT[4], H_PERMUTATION );
    RT[6] = permute( RT[5], H_PERMUTATION );
    RT[7] = permute( RT[6], H_PERMUTATION );

    // Absorb all 128-byte blocks (8 in parallel)
    while (16*(i+8) <= *m_len) {

          PROCESS_8_MESSAGE_BLOCKS_FIRST_PASS( States, message + 16 * i , Auth );

          if(248 == ( i % 256) ){
              ADD_TO_TWEAK( RT[ 0], SEIGHT );
              RT[1] = permute( RT[0], H_PERMUTATION );
              RT[2] = permute( RT[1], H_PERMUTATION );
              RT[3] = permute( RT[2], H_PERMUTATION );
              RT[4] = permute( RT[3], H_PERMUTATION );
              RT[5] = permute( RT[4], H_PERMUTATION );
              RT[6] = permute( RT[5], H_PERMUTATION );
              RT[7] = permute( RT[6], H_PERMUTATION );
          }
          else{

              RT[ 0] = add8( RT[ 0], OT[ 0] );
              RT[ 1] = add8( RT[ 1], OT[ 1] );
              RT[ 2] = add8( RT[ 2], OT[ 2] );
              RT[ 3] = add8( RT[ 3], OT[ 3] );
              RT[ 4] = add8( RT[ 4], OT[ 4] );
              RT[ 5] = add8( RT[ 5], OT[ 5] );
              RT[ 6] = add8( RT[ 6], OT[ 6] );
              RT[ 7] = add8( RT[ 7], OT[ 7] );
          }
          i += 8;
    }

    // Absorb the remaining full 16-byte blocks
    while (16*(i+1) <= *m_len) {
          State = load( (__m128i *) ( message + 16 * i ) );
          SET_BLOCK_NO_IN_TWEAK( Tweak, i);
          AES( State, subkeys, Tweak, tTweak );
          Auth = xor( Auth, State );
          i++;
    }


    // if last message block is incomplete
    if( *m_len != 16 * i ){


        SET_STAGE_IN_TWEAK(Tweak, MSB_M_LAST_NONZERO);
        SET_BLOCK_NO_IN_TWEAK( Tweak, i);
        memset(last_block, 0, 16);
        memcpy(last_block, message +16*i, *m_len-16*i);
        last_block[*m_len-16*i]=0x80;
        tmp = load( (__m128i *) ( last_block ) );
        AES( tmp , subkeys, Tweak, tTweak );
        Auth = xor( tmp, Auth);


    }



    last_block[0] = 1<<4;
    memcpy(last_block+1,nonce,15);
    Tweak = load( (__m128i *) ( last_block ) );
    AES( Auth, subkeys, Tweak, tTweak );

    store( last_block , Auth );



    /* If the tags does not match, return error -1 */
    if( 0 != memcmp_const(last_block, c+c_len, 16) ) {
        return -1;
    }




    return 0;
}
