void AES::encrypt(const byte* inBlock, const byte* xorBlock,
                  byte* outBlock) const
{
    word32 s0, s1, s2, s3;
    word32 t0, t1, t2, t3;

    const word32 *rk = key_;
    /*
     * map byte array block to cipher state
     * and add initial round key:
     */
    gpBlock::Get(inBlock)(s0)(s1)(s2)(s3);
    s0 ^= rk[0];
    s1 ^= rk[1];
    s2 ^= rk[2];
    s3 ^= rk[3];
   
    /*
     * Nr - 1 full rounds:
     */

    unsigned int r = rounds_ >> 1;
    for (;;) {
        t0 =
            Te0[GETBYTE(s0, 3)] ^
            Te1[GETBYTE(s1, 2)]  ^
            Te2[GETBYTE(s2, 1)]  ^
            Te3[GETBYTE(s3, 0)]  ^
            rk[4];
        t1 =
            Te0[GETBYTE(s1, 3)] ^
            Te1[GETBYTE(s2, 2)]  ^
            Te2[GETBYTE(s3, 1)]  ^
            Te3[GETBYTE(s0, 0)]  ^
            rk[5];
        t2 =
            Te0[GETBYTE(s2, 3)] ^
            Te1[GETBYTE(s3, 2)]  ^
            Te2[GETBYTE(s0, 1)]  ^
            Te3[GETBYTE(s1, 0)]  ^
            rk[6];
        t3 =
            Te0[GETBYTE(s3, 3)] ^
            Te1[GETBYTE(s0, 2)]  ^
            Te2[GETBYTE(s1, 1)]  ^
            Te3[GETBYTE(s2, 0)]  ^
            rk[7];

        rk += 8;
        if (--r == 0) {
            break;
        }
        
        s0 =
            Te0[GETBYTE(t0, 3)] ^
            Te1[GETBYTE(t1, 2)] ^
            Te2[GETBYTE(t2, 1)] ^
            Te3[GETBYTE(t3, 0)] ^
            rk[0];
        s1 =
            Te0[GETBYTE(t1, 3)] ^
            Te1[GETBYTE(t2, 2)] ^
            Te2[GETBYTE(t3, 1)] ^
            Te3[GETBYTE(t0, 0)] ^
            rk[1];
        s2 =
            Te0[GETBYTE(t2, 3)] ^
            Te1[GETBYTE(t3, 2)] ^
            Te2[GETBYTE(t0, 1)] ^
            Te3[GETBYTE(t1, 0)] ^
            rk[2];
        s3 =
            Te0[GETBYTE(t3, 3)] ^
            Te1[GETBYTE(t0, 2)] ^
            Te2[GETBYTE(t1, 1)] ^
            Te3[GETBYTE(t2, 0)] ^
            rk[3];
    }

    /*
     * apply last round and
     * map cipher state to byte array block:
     */

    s0 =
        (Te4[GETBYTE(t0, 3)] & 0xff000000) ^
        (Te4[GETBYTE(t1, 2)] & 0x00ff0000) ^
        (Te4[GETBYTE(t2, 1)] & 0x0000ff00) ^
        (Te4[GETBYTE(t3, 0)] & 0x000000ff) ^
        rk[0];
    s1 =
        (Te4[GETBYTE(t1, 3)] & 0xff000000) ^
        (Te4[GETBYTE(t2, 2)] & 0x00ff0000) ^
        (Te4[GETBYTE(t3, 1)] & 0x0000ff00) ^
        (Te4[GETBYTE(t0, 0)] & 0x000000ff) ^
        rk[1];
    s2 =
        (Te4[GETBYTE(t2, 3)] & 0xff000000) ^
        (Te4[GETBYTE(t3, 2)] & 0x00ff0000) ^
        (Te4[GETBYTE(t0, 1)] & 0x0000ff00) ^
        (Te4[GETBYTE(t1, 0)] & 0x000000ff) ^
        rk[2];
    s3 =
        (Te4[GETBYTE(t3, 3)] & 0xff000000) ^
        (Te4[GETBYTE(t0, 2)] & 0x00ff0000) ^
        (Te4[GETBYTE(t1, 1)] & 0x0000ff00) ^
        (Te4[GETBYTE(t2, 0)] & 0x000000ff) ^
        rk[3];


    gpBlock::Put(xorBlock, outBlock)(s0)(s1)(s2)(s3);
}