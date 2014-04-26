#include "crypto_aead.h"
#include "api.h"
#include "stdio.h"

unsigned char RoundKey[176];
//AES S-box
int sbox[256] =
{
    //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, //0
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, //1
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, //2
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, //3
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, //4
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, //5
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, //6
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, //7
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, //8
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, //9
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, //A
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, //B
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, //C
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, //D
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, //E
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
}; 

int Isbox[256] =
{
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};


unsigned char RC[] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91};



void keyschedule(unsigned char *primekey, int roundNum)
{
    int KN, i;
    switch(roundNum)
    {
    case 10:
        KN = 16;
        break;
    case 12:
        KN = 24;
        break;
    case 14:
        KN = 32;
        break;
    }
    
    for(i = 0; i < KN; i++)
    {
        RoundKey[i] = primekey[i];
    }

    for(i = KN / 4; i < (roundNum + 1) * 4; i++)
    {

        if(i % 4 == 0)
        {
          
            RoundKey[4 * i] = sbox[RoundKey[4 * (i - 1) + 1]] ^ RC[i / 4] ^ RoundKey[4 * (i - 4)];

            RoundKey[4 * i + 1] = sbox[RoundKey[4 * (i - 1) + 2]] ^ RoundKey[4 * (i - 4) + 1];

            RoundKey[4 * i + 2] = sbox[RoundKey[4 * (i - 1) + 3]] ^ RoundKey[4 * (i - 4) + 2];

            RoundKey[(4 * i + 3)] = sbox[RoundKey[4 * (i - 1)]] ^ RoundKey[4 * (i - 4) + 3];
        }
        else
        {
            
            RoundKey[4 * i] = RoundKey[4 * (i - 1)] ^ RoundKey[4 * (i - 4)];
            RoundKey[4 * i + 1] = RoundKey[4 * (i - 1) + 1] ^ RoundKey[4 * (i - 4) + 1];
            RoundKey[4 * i + 2] = RoundKey[4 * (i - 1) + 2] ^ RoundKey[4 * (i - 4) + 2];
            RoundKey[4 * i + 3] = RoundKey[4 * (i - 1) + 3] ^ RoundKey[4 * (i - 4) + 3];
        }

    }

}


void Encryption_cpu(unsigned char *in, unsigned char *out)
{
    int i, j, round = 0;
    unsigned char temp;
    unsigned char Tmp, Tm, t;
    unsigned char state[4][4];

    
    for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
            state[j][i] = in[i * 4 + j];

    
    for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
            state[j][i] ^= RoundKey[ i * 4 + j];

    
    for(round = 1; round < 10; round++)
    {
       
        for(i = 0; i < 4; i++)
            for(j = 0; j < 4; j++)
                state[i][j] = sbox[state[i][j]];

        
        temp = state[1][0];
        state[1][0] = state[1][1];
        state[1][1] = state[1][2];
        state[1][2] = state[1][3];
        state[1][3] = temp;

       
        temp = state[2][0];
        state[2][0] = state[2][2];
        state[2][2] = temp;

        temp = state[2][1];
        state[2][1] = state[2][3];
        state[2][3] = temp;

       
        temp = state[3][0];
        state[3][0] = state[3][3];
        state[3][3] = state[3][2];
        state[3][2] = state[3][1];
        state[3][1] = temp;


       
        for(i = 0; i < 4; i++)
        {
            t = state[0][i];
            Tmp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i] ;
            Tm = state[0][i] ^ state[1][i] ;
            Tm = ((Tm << 1) ^ (((Tm >> 7) & 1) * 0x1b));
            state[0][i] ^= Tm ^ Tmp ;
            Tm = state[1][i] ^ state[2][i] ;
            Tm = ((Tm << 1) ^ (((Tm >> 7) & 1) * 0x1b));
            state[1][i] ^= Tm ^ Tmp ;
            Tm = state[2][i] ^ state[3][i] ;
            Tm = ((Tm << 1) ^ (((Tm >> 7) & 1) * 0x1b));
            state[2][i] ^= Tm ^ Tmp ;
            Tm = state[3][i] ^ t ;
            Tm = ((Tm << 1) ^ (((Tm >> 7) & 1) * 0x1b));
            state[3][i] ^= Tm ^ Tmp ;
        }


       
        for(i = 0; i < 4; i++)
            for(j = 0; j < 4; j++)
                state[j][i] ^= RoundKey[round * 16 + i * 4 + j];

    }

   
    for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
            state[i][j] = sbox[state[i][j]];

    
    temp = state[1][0];
    state[1][0] = state[1][1];
    state[1][1] = state[1][2];
    state[1][2] = state[1][3];
    state[1][3] = temp;

    
    temp = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp;

    temp = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp;

   
    temp = state[3][0];
    state[3][0] = state[3][3];
    state[3][3] = state[3][2];
    state[3][2] = state[3][1];
    state[3][1] = temp;
    
    for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
            state[j][i] ^= RoundKey[160 + i * 4 + j];

    
    for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
            out[i * 4 + j] = state[j][i];


}

void Times2(unsigned char times1[16], unsigned char times2[16])
{
    int i;
    for (i = 0; i < 15; i++)
    {
        times2[i] = (times1[i] << 1) ^ (times1[i + 1] >> 7);
    }

    times2[15] = times1[15] << 1;
    if ((times1[0] >> 7) != 0)
    {
        times2[15] ^= 0x87;
    }
}

int crypto_aead_encrypt(
    unsigned char *c, unsigned long long *clen,
    const unsigned char *m, unsigned long long mlen,
    const unsigned char *ad, unsigned long long adlen,
    const unsigned char *nsec,
    const unsigned char *npub,
    const unsigned char *k
)
{
    unsigned char SumB[16], LastABlock[16], MacAESTag[16], Key[16], Double[16];;
    unsigned long long a; // a is the block length of associated data A
    unsigned char PaddedPMN[16];
    unsigned char LastPlaintextBlock[16], PaddedBlock[16];
    unsigned long long l; // l is the block length of plaintext
    unsigned char StolenR[16];
    unsigned char Z[3][16], NextZ[16], U[16];
    unsigned char AESInputBlock[16], AESOutputBlock[16];
    unsigned long long i, j;

    //Precomputations, Key Scheduling
    for(j = 0; j < 16; j++) Key[j] = k[j];
    keyschedule(Key, 10);

    //Precomputations, calculating Z0, Z1, Z2 masks
    for(j = 0; j < 16; j++) AESInputBlock[j] = 0x0;
    Encryption_cpu(AESInputBlock, Z[0]);

    for(i = 0; i < 2; i++)	 Times2(Z[i], Z[i + 1]);

    Times2(Z[2], NextZ); // this is Z3

    //Authenticating associated data ad by MacAES
    if(adlen == 0)
    {
        for(j = 0; j < 16; j++) MacAESTag[j] = 0x0;
    }
    else
    {
        if (adlen % 16 == 0)  a = adlen / 16;
        else a = adlen / 16 + 1;

        for(j = 0; j < 16; j++)  SumB[j] = 0x0;

        for(i = 0; i < a - 1; i++)
        {
            for(j = 0; j < 16; j++)  AESInputBlock[j] = ad[i * 16 + j] ^ NextZ[j];

            Encryption_cpu(AESInputBlock, AESOutputBlock);

            for(j = 0; j < 16; j++) SumB[j] = SumB[j] ^ AESOutputBlock[j];

            Times2(NextZ, Double);

            for(j = 0; j < 16; j++) NextZ[j] = Double[j]; // producing the next Z
        }

        if(adlen % 16 != 0)
        {
            for(j = 0; j < adlen % 16; j++) LastABlock[j] = ad[(a - 1) * 16 + j];
            LastABlock[adlen % 16] = 0x80;
            for(j = adlen % 16 + 1; j < 16; j++)		LastABlock[j] = 0x0;

            for(j = 0; j < 16; j++) AESInputBlock[j] = SumB[j] ^ LastABlock[j] ^ Z[1][j];
        }
        else
        {
            for(j = 0; j < 16; j++) AESInputBlock[j] = SumB[j] ^ ad[(a - 1) * 16 + j] ^ Z[2][j];
        }

        Encryption_cpu(AESInputBlock, MacAESTag);

    }

    if(CRYPTO_NPUBBYTES > 16)  return -2;    // the public message number is too long
	else if (CRYPTO_NPUBBYTES == 0) return -3;  // the public message number is empty
    else if (CRYPTO_NSECBYTES > 0) return -4;   // the secret message number should be null

    *clen = mlen + 16;

    // padding PublicPlaintextNumber with 10*
    for(j = 0; j < CRYPTO_NPUBBYTES; j++) PaddedPMN[j] = npub[j];
    PaddedPMN[CRYPTO_NPUBBYTES] = 0x80;
    for(j = CRYPTO_NPUBBYTES + 1; j < 16; j++)	 PaddedPMN[j] = 0x0;

    // generating the secret value U
    Encryption_cpu(PaddedPMN, U);

    l = mlen / 16;

    if (mlen % 16 != 0)
    {
        l = l + 1;

        for(j = 0; j < mlen % 16; j++)
        {
            LastPlaintextBlock[j] = m[(l - 1) * 16 + j];
            PaddedBlock[j] = 0x0;
        }

        LastPlaintextBlock[mlen % 16] = 0x0;
        PaddedBlock[mlen % 16] = 0x80;

        for(j = mlen % 16 + 1; j < 16; j++)
        {
            LastPlaintextBlock[j] = 0x0;
            PaddedBlock[j] = 0x0;
        }
    }
    else
    {
        for(j = 0; j < 16; j++)
        {
            LastPlaintextBlock[j] = m[(l - 1) * 16 + j];
            PaddedBlock[j] = 0x0;
        }
    }

    // Encryption for Plaintext
    // Producing the first l-1 ciphertext blocks

    Times2(Z[2], NextZ); // producing Z3

    for(j = 0; j < 16; j++)  AESInputBlock[j] = NextZ[j] ^ U[j];

    Encryption_cpu(AESInputBlock, AESOutputBlock);

    for(i = 0; i < l - 1; i++)
    {
        Times2(NextZ, Double); // producing the next Z

        for(j = 0; j < 16; j++)
        {
            NextZ[j] = Double[j];
            AESInputBlock[j] = m[i * 16 + j] ^ NextZ[j] ^ U[j];
            c[i * 16 + j] = AESOutputBlock[j] ^ AESInputBlock[j];
        }
        Encryption_cpu(AESInputBlock, AESOutputBlock);
    }

    // Producing the last ciphertext block, note that its length may be smaller than 16
    // Producing the stolen ciphertext R
    if (mlen % 16 != 0)
    {
        for(j = 0; j < mlen % 16; j++)
        {
            c[(l - 1) * 16 + j] = AESOutputBlock[j] ^ LastPlaintextBlock[j];
            StolenR[j] = 0x0;
        }
        for(j = (mlen % 16); j < 16; j++) StolenR[j] = AESOutputBlock[j] ^ PaddedBlock[j];
    }
    else
    {
        for(j = 0; j < 16; j++) c[(l - 1) * 16 + j] = AESOutputBlock[j] ^ LastPlaintextBlock[j];

    }

    // Producing the final Tag
    if (mlen % 16 != 0)
    {
        for(j = 0; j < 16; j++)   AESInputBlock[j] = LastPlaintextBlock[j] ^ StolenR[j] ^ Z[1][j] ^ U[j];
        Encryption_cpu(AESInputBlock, AESOutputBlock);
        for(j = 0; j < 16; j++) c[(l - 1) * 16 + (mlen % 16) + j] = AESOutputBlock[j] ^ MacAESTag[j];
    }
    else
    {
        for(j = 0; j < 16; j++)   AESInputBlock[j] = LastPlaintextBlock[j] ^ Z[2][j] ^ U[j];
        Encryption_cpu(AESInputBlock, AESOutputBlock);
        for(j = 0; j < 16; j++) c[l * 16 + j] = AESOutputBlock[j] ^ MacAESTag[j];
    }

    return 0;
}


int crypto_aead_decrypt(
    unsigned char *m, unsigned long long *mlen,
    unsigned char *nsec,
    const unsigned char *c, unsigned long long clen,
    const unsigned char *ad, unsigned long long adlen,
    const unsigned char *npub,
    const unsigned char *k
)
{
    unsigned char SumB[16], LastABlock[16], MacAESTag[16], Key[16], Double[16];;
    unsigned long long a; // a is the block length of associated data A
    unsigned char PaddedPMN[16];
    unsigned char LastCiphertextBlock[16], PaddedBlock[16];
    unsigned long long l; // l is the block length of plaintext
    unsigned char StolenR[16];
    unsigned char Z[3][16], NextZ[16], U[16];
    unsigned char AESInputBlock[16], AESOutputBlock[16];
    unsigned long long i, j;

    //Precomputations, Key Scheduling
    for(j = 0; j < 16; j++) Key[j] = k[j];
    keyschedule(Key, 10);

    //Precomputations, calculating Z0, Z1, Z2 masks
    for(j = 0; j < 16; j++) AESInputBlock[j] = 0x0;
    Encryption_cpu(AESInputBlock, Z[0]);

    for(i = 0; i < 2; i++)	 Times2(Z[i], Z[i + 1]);

    Times2(Z[2], NextZ); // this is Z3

    //Authenticating associated data ad by MacAES
    if(adlen == 0)
    {
        for(j = 0; j < 16; j++) MacAESTag[j] = 0x0;
    }
    else
    {
        if (adlen % 16 == 0)  a = adlen / 16;
        else a = adlen / 16 + 1;

        for(j = 0; j < 16; j++)  SumB[j] = 0x0;

        for(i = 0; i < a - 1; i++)
        {
            for(j = 0; j < 16; j++)  AESInputBlock[j] = ad[i * 16 + j] ^ NextZ[j];

            Encryption_cpu(AESInputBlock, AESOutputBlock);

            for(j = 0; j < 16; j++) SumB[j] = SumB[j] ^ AESOutputBlock[j];

            Times2(NextZ, Double);

            for(j = 0; j < 16; j++) NextZ[j] = Double[j]; // producing the next Z
        }

        if(adlen % 16 != 0)
        {
            for(j = 0; j < adlen % 16; j++) LastABlock[j] = ad[(a - 1) * 16 + j];
            LastABlock[adlen % 16] = 0x80;
            for(j = adlen % 16 + 1; j < 16; j++)		LastABlock[j] = 0x0;

            for(j = 0; j < 16; j++) AESInputBlock[j] = SumB[j] ^ LastABlock[j] ^ Z[1][j];
        }
        else
        {
            for(j = 0; j < 16; j++) AESInputBlock[j] = SumB[j] ^ ad[(a - 1) * 16 + j] ^ Z[2][j];
        }

        Encryption_cpu(AESInputBlock, MacAESTag);
    }

    if(CRYPTO_NPUBBYTES > 16)  return -2;    // the public message number is too long
    else if (CRYPTO_NSECBYTES > 0) return -3;   // the secret message number should be null

    *mlen = clen - 16;

    // padding PublicPlaintextNumber with 10*
    for(j = 0; j < CRYPTO_NPUBBYTES; j++) PaddedPMN[j] = npub[j];
    PaddedPMN[CRYPTO_NPUBBYTES] = 0x80;
    for(j = CRYPTO_NPUBBYTES + 1; j < 16; j++)	 PaddedPMN[j] = 0x0;

    // generating the secret value U
    Encryption_cpu(PaddedPMN, U);

    l = clen / 16 - 1;
    if (clen % 16 != 0)
    {
        l = l + 1;
        for(j = 0; j < clen % 16; j++)
        {
            LastCiphertextBlock[j] = c[(l - 1) * 16 + j]; // note that this is C_l, not C_{l+1} related to the final tag
            PaddedBlock[j] = 0x0;
        }

        LastCiphertextBlock[clen % 16] = 0x0;
        PaddedBlock[clen % 16] = 0x80;

        for(j = clen % 16 + 1; j < 16; j++)
        {
            LastCiphertextBlock[j] = 0x0;
            PaddedBlock[j] = 0x0;
        }
    }
    else
    {
        // l = l - 1;
        for(j = 0; j < 16; j++)
        {
            LastCiphertextBlock[j] = c[(l - 1) * 16 + j];
            PaddedBlock[j] = 0x0;
        }
    }


    // Decryption from Ciphertext
    // Producing the first l-1 plaintext blocks

    Times2(Z[2], NextZ); // producing Z3

    for(j = 0; j < 16; j++)  AESInputBlock[j] = NextZ[j] ^ U[j];

    Encryption_cpu(AESInputBlock, AESOutputBlock);

    for(i = 0; i < l - 1; i++)
    {
        Times2(NextZ, Double); // producing the next Z

        for(j = 0; j < 16; j++)
        {
            AESInputBlock[j] = AESOutputBlock[j] ^ c[i * 16 + j];
            m[i * 16 + j] = AESInputBlock[j] ^ Double[j] ^ U[j];
            NextZ[j] = Double[j];
        }
        Encryption_cpu(AESInputBlock, AESOutputBlock);
    }

    // Producing the last plaintext block
    // Producing the stolen ciphertext R
    if (clen % 16 != 0)
    {
        for(j = 0; j < clen % 16; j++)
        {
            m[(l - 1) * 16 + j] = AESOutputBlock[j] ^ c[(l - 1) * 16 + j];
            StolenR[j] = 0x0;
        }
        for(j = (clen % 16); j < 16; j++) StolenR[j] = AESOutputBlock[j] ^ PaddedBlock[j];
    }
    else
    {
        for(j = 0; j < 16; j++)
        {
            m[(l - 1) * 16 + j] = AESOutputBlock[j] ^ c[(l - 1) * 16 + j];
            StolenR[j] = 0x0;
        }
    }

    // Checking the final Tag
    if (clen % 16 != 0)
    {
        for(j = 0; j < (clen % 16); j++)   AESInputBlock[j] = m[(l - 1) * 16 + j] ^ Z[1][j] ^ U[j];
        for(j = (clen % 16); j < 16; j++) AESInputBlock[j] = StolenR[j] ^ Z[1][j] ^ U[j];
        Encryption_cpu(AESInputBlock, AESOutputBlock);

        for(j = 0; j < 16; j++)
        {
            if(c[(l - 1) * 16 + clen % 16 + j] != (AESOutputBlock[j] ^ MacAESTag[j])) return -1;
        }
    }
    else
    {
        for(j = 0; j < 16; j++)   AESInputBlock[j] = m[(l - 1) * 16 + j] ^ Z[2][j] ^ U[j];
        Encryption_cpu(AESInputBlock, AESOutputBlock);

        for(j = 0; j < 16; j++)
        {
            if(c[l * 16 + j] != (AESOutputBlock[j] ^ MacAESTag[j])) return -1;
        }
    }


    return 0;
}

/*

int main()
{
    //HINSTANCE hDLL;
    unsigned char Key1[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    unsigned char Key2[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10}; //key of text vector in submission

    unsigned char Message1[2] = "1";
    unsigned char Message2[4] = "abc";
    unsigned char Message3[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    unsigned char Message4[27] = "abcdefghijklmnopqrstuvwxyz"; // associated data of text vector in submission
    unsigned char Message5[37] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; // plaintext of text vector in submission
    unsigned char Message6[81] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
    unsigned char Message7[48] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
                                  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                  0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
                                 };

    unsigned long long Message1ByteLength = 0;
    unsigned long long Message2ByteLength = 3;
    unsigned long long Message3ByteLength = 16;
    unsigned long long Message4ByteLength = 26;
    unsigned long long Message5ByteLength = 36;
    unsigned long long Message6ByteLength = 80;
    unsigned long long Message7ByteLength = 48;

    unsigned char PublicMessageNumber1[13] = "TCA@ISCAS@CN";  //12-byte PMN
    unsigned char PublicMessageNumber2[14] = "iFeed AE Mode"; //13-byte PMN, text vector in submission

    unsigned long long PublicMessageNumber1ByteLength = 12;
    unsigned long long PublicMessageNumber2ByteLength = 13;
    unsigned long long j;

    unsigned char plaintext[100], ciphertext[100];
    unsigned long long plaintextbytelength, ciphertextbytelength;
    unsigned char *SMN = NULL;

    unsigned char PMN[CRYPTO_NPUBBYTES], AD[100], PT[100], Key[16];
    unsigned long long ADbytelength, PTbytelength;

    //use Key 2 for test
    for(j = 0; j < 16; j++) Key[j] = Key2[j];

    //use public messge number 2 for test
    for(j = 0; j < CRYPTO_NPUBBYTES; j++) PMN[j] = PublicMessageNumber2[j];

    //use Message4 as Associated Data for test
    ADbytelength = Message4ByteLength;
    for(j = 0; j < ADbytelength; j++) AD[j] = Message4[j];

    //use Message5 as PlainText for test
    PTbytelength = Message5ByteLength;
    for(j = 0; j < PTbytelength; j++) PT[j] = Message5[j];




    crypto_aead_encrypt(ciphertext, &ciphertextbytelength, PT, PTbytelength, AD, ADbytelength, SMN, PMN, Key);

    printf("\nIn authenticated encryption, \n");
    printf("\n\nthe Key is: \n");
    for(j = 0; j < 16; j++) printf("%02x", Key[j]);

    printf("\n\nthe public message number is: \n");
    for(j = 0; j < CRYPTO_NPUBBYTES; j++) printf("%c", PMN[j]);
    printf("\n\nthe public message number is: \n");
    for(j = 0; j < CRYPTO_NPUBBYTES; j++) printf("%02x", PMN[j]);

    printf("\n\nthe AD is: \n");
    for(j = 0; j < ADbytelength; j++) printf("%c", AD[j]);
    printf("\n\nthe AD is: \n");
    for(j = 0; j < ADbytelength; j++) printf("%02x", AD[j]);

    printf("\n\nthe plaintext is: \n");
    for(j = 0; j < PTbytelength; j++) printf("%c", PT[j]);

    printf("\n\nthe plaintext is: \n");
    for(j = 0; j < PTbytelength; j++) printf("%02x", PT[j]);

    printf("\n\nthe ciphertext is: \n");
    for(j = 0; j < ciphertextbytelength - 16; j++) printf("%02x", ciphertext[j]);

    printf("\n\nthe tag is: \n");
    for(j = ciphertextbytelength - 16; j < ciphertextbytelength; j++) printf("%02x", ciphertext[j]);

    printf("\n\n\n");

    //crypto_aead_decrypt(plaintext, &plaintextbytelength, SMN, ciphertext,  ciphertextbytelength, AD, ADbytelength, PMN, Key);

    if(crypto_aead_decrypt(plaintext, &plaintextbytelength, SMN, ciphertext,  ciphertextbytelength, AD, ADbytelength, PMN, Key) == -1) printf("\n\nDecryption failed, the ciphertext is invalid");
    else if (crypto_aead_decrypt(plaintext, &plaintextbytelength, SMN, ciphertext,  ciphertextbytelength, AD, ADbytelength, PMN, Key) == -2) printf("\n\nDecryption failed, the public message number is too long");
    else if (crypto_aead_decrypt(plaintext, &plaintextbytelength, SMN, ciphertext,  ciphertextbytelength, AD, ADbytelength, PMN, Key) == -3) printf("\n\nDecryption failed, the secret message number should be null");
    else
    {
        printf("\nIn authenticated decryption, \n");
        printf("\n\nthe Key is: \n");
        for(j = 0; j < 16; j++) printf("%02x", Key[j]);

        printf("\n\nthe public message number is: \n");
        for(j = 0; j < CRYPTO_NPUBBYTES; j++) printf("%c", PMN[j]);
        printf("\n\nthe public message number is: \n");
        for(j = 0; j < CRYPTO_NPUBBYTES; j++) printf("%02x", PMN[j]);

        printf("\n\nthe AD is: \n");
        for(j = 0; j < ADbytelength; j++) printf("%c", AD[j]);
        printf("\n\nthe AD is: \n");
        for(j = 0; j < ADbytelength; j++) printf("%02x", AD[j]);

        printf("\n\nthe ciphertext is: \n");
        for(j = 0; j < ciphertextbytelength - 16; j++) printf("%02x", ciphertext[j]);

        printf("\n\nthe plaintext is: \n");
        for(j = 0; j < plaintextbytelength; j++) printf("%c", plaintext[j]);

        printf("\n\nthe plaintext is: \n");
        for(j = 0; j < plaintextbytelength; j++) printf("%02x", plaintext[j]);

        printf("\n\nDecrypting Sucessfully\n\n");

    }

    return 0;
}
*/
