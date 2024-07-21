unsigned int FUZ_rand(unsigned int* src)
{
    U32 rand32 = *src;
    rand32 *= PRIME1;
    rand32 += PRIME2;
    rand32  = FUZ_rotl32(rand32, 13);
    *src = rand32;
    return rand32;
}