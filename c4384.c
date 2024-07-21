void AES::SetKey(const byte* userKey, word32 keylen, CipherDir /*dummy*/)
{
    if (keylen <= 16)
        keylen = 16;
    else if (keylen >= 32)
        keylen = 32;
    else if (keylen != 24)
        keylen = 24;
    
    rounds_ = keylen/4 + 6;

    word32 temp, *rk = key_;
    unsigned int i=0;

    GetUserKey(BigEndianOrder, rk, keylen/4, userKey, keylen);

    switch(keylen)
    {
    case 16:
        while (true)
        {
            temp  = rk[3];
            rk[4] = rk[0] ^
                (Te4[GETBYTE(temp, 2)] & 0xff000000) ^
                (Te4[GETBYTE(temp, 1)] & 0x00ff0000) ^
                (Te4[GETBYTE(temp, 0)] & 0x0000ff00) ^
                (Te4[GETBYTE(temp, 3)] & 0x000000ff) ^
                rcon_[i];
            rk[5] = rk[1] ^ rk[4];
            rk[6] = rk[2] ^ rk[5];
            rk[7] = rk[3] ^ rk[6];
            if (++i == 10)
                break;
            rk += 4;
        }
        break;

    case 24:
        while (true)    // for (;;) here triggers a bug in VC60 SP4 w/ Pro Pack
        {
            temp = rk[ 5];
            rk[ 6] = rk[ 0] ^
                (Te4[GETBYTE(temp, 2)] & 0xff000000) ^
                (Te4[GETBYTE(temp, 1)] & 0x00ff0000) ^
                (Te4[GETBYTE(temp, 0)] & 0x0000ff00) ^
                (Te4[GETBYTE(temp, 3)] & 0x000000ff) ^
                rcon_[i];
            rk[ 7] = rk[ 1] ^ rk[ 6];
            rk[ 8] = rk[ 2] ^ rk[ 7];
            rk[ 9] = rk[ 3] ^ rk[ 8];
            if (++i == 8)
                break;
            rk[10] = rk[ 4] ^ rk[ 9];
            rk[11] = rk[ 5] ^ rk[10];
            rk += 6;
        }
        break;

    case 32:
        while (true)
        {
            temp = rk[ 7];
            rk[ 8] = rk[ 0] ^
                (Te4[GETBYTE(temp, 2)] & 0xff000000) ^
                (Te4[GETBYTE(temp, 1)] & 0x00ff0000) ^
                (Te4[GETBYTE(temp, 0)] & 0x0000ff00) ^
                (Te4[GETBYTE(temp, 3)] & 0x000000ff) ^
                rcon_[i];
            rk[ 9] = rk[ 1] ^ rk[ 8];
            rk[10] = rk[ 2] ^ rk[ 9];
            rk[11] = rk[ 3] ^ rk[10];
            if (++i == 7)
                break;
            temp = rk[11];
            rk[12] = rk[ 4] ^
                (Te4[GETBYTE(temp, 3)] & 0xff000000) ^
                (Te4[GETBYTE(temp, 2)] & 0x00ff0000) ^
                (Te4[GETBYTE(temp, 1)] & 0x0000ff00) ^
                (Te4[GETBYTE(temp, 0)] & 0x000000ff);
            rk[13] = rk[ 5] ^ rk[12];
            rk[14] = rk[ 6] ^ rk[13];
            rk[15] = rk[ 7] ^ rk[14];

            rk += 8;
        }
        break;
    }

    if (dir_ == DECRYPTION)
    {
        unsigned int i, j;
        rk = key_;

        /* invert the order of the round keys: */
        for (i = 0, j = 4*rounds_; i < j; i += 4, j -= 4) {
            temp = rk[i    ]; rk[i    ] = rk[j    ]; rk[j    ] = temp;
            temp = rk[i + 1]; rk[i + 1] = rk[j + 1]; rk[j + 1] = temp;
            temp = rk[i + 2]; rk[i + 2] = rk[j + 2]; rk[j + 2] = temp;
            temp = rk[i + 3]; rk[i + 3] = rk[j + 3]; rk[j + 3] = temp;
        }
        // apply the inverse MixColumn transform to all round keys but the
        // first and the last:
        for (i = 1; i < rounds_; i++) {
            rk += 4;
            rk[0] =
                Td0[Te4[GETBYTE(rk[0], 3)] & 0xff] ^
                Td1[Te4[GETBYTE(rk[0], 2)] & 0xff] ^
                Td2[Te4[GETBYTE(rk[0], 1)] & 0xff] ^
                Td3[Te4[GETBYTE(rk[0], 0)] & 0xff];
            rk[1] =
                Td0[Te4[GETBYTE(rk[1], 3)] & 0xff] ^
                Td1[Te4[GETBYTE(rk[1], 2)] & 0xff] ^
                Td2[Te4[GETBYTE(rk[1], 1)] & 0xff] ^
                Td3[Te4[GETBYTE(rk[1], 0)] & 0xff];
            rk[2] =
                Td0[Te4[GETBYTE(rk[2], 3)] & 0xff] ^
                Td1[Te4[GETBYTE(rk[2], 2)] & 0xff] ^
                Td2[Te4[GETBYTE(rk[2], 1)] & 0xff] ^
                Td3[Te4[GETBYTE(rk[2], 0)] & 0xff];
            rk[3] =
                Td0[Te4[GETBYTE(rk[3], 3)] & 0xff] ^
                Td1[Te4[GETBYTE(rk[3], 2)] & 0xff] ^
                Td2[Te4[GETBYTE(rk[3], 1)] & 0xff] ^
                Td3[Te4[GETBYTE(rk[3], 0)] & 0xff];
        }
    }
}