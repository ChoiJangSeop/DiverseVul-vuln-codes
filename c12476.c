void CPINLINE swoole_mini_memcpy(void *dst, const void *src, size_t len)
{
    register unsigned char *dd = (unsigned char*) dst + len;
    register const unsigned char *ss = (const unsigned char*) src + len;
    switch (len)
    {
        case 68: *((int*) (dd - 68)) = *((int*) (ss - 68));
        /* no break */
        case 64: *((int*) (dd - 64)) = *((int*) (ss - 64));
        /* no break */
        case 60: *((int*) (dd - 60)) = *((int*) (ss - 60));
        /* no break */
        case 56: *((int*) (dd - 56)) = *((int*) (ss - 56));
        /* no break */
        case 52: *((int*) (dd - 52)) = *((int*) (ss - 52));
        /* no break */
        case 48: *((int*) (dd - 48)) = *((int*) (ss - 48));
        /* no break */
        case 44: *((int*) (dd - 44)) = *((int*) (ss - 44));
        /* no break */
        case 40: *((int*) (dd - 40)) = *((int*) (ss - 40));
        /* no break */
        case 36: *((int*) (dd - 36)) = *((int*) (ss - 36));
        /* no break */
        case 32: *((int*) (dd - 32)) = *((int*) (ss - 32));
        /* no break */
        case 28: *((int*) (dd - 28)) = *((int*) (ss - 28));
        /* no break */
        case 24: *((int*) (dd - 24)) = *((int*) (ss - 24));
        /* no break */
        case 20: *((int*) (dd - 20)) = *((int*) (ss - 20));
        /* no break */
        case 16: *((int*) (dd - 16)) = *((int*) (ss - 16));
        /* no break */
        case 12: *((int*) (dd - 12)) = *((int*) (ss - 12));
        /* no break */
        case 8: *((int*) (dd - 8)) = *((int*) (ss - 8));
        /* no break */
        case 4: *((int*) (dd - 4)) = *((int*) (ss - 4));
            break;
        case 67: *((int*) (dd - 67)) = *((int*) (ss - 67));
        /* no break */
        case 63: *((int*) (dd - 63)) = *((int*) (ss - 63));
        /* no break */
        case 59: *((int*) (dd - 59)) = *((int*) (ss - 59));
        /* no break */
        case 55: *((int*) (dd - 55)) = *((int*) (ss - 55));
        /* no break */
        case 51: *((int*) (dd - 51)) = *((int*) (ss - 51));
        /* no break */
        case 47: *((int*) (dd - 47)) = *((int*) (ss - 47));
        /* no break */
        case 43: *((int*) (dd - 43)) = *((int*) (ss - 43));
        /* no break */
        case 39: *((int*) (dd - 39)) = *((int*) (ss - 39));
        /* no break */
        case 35: *((int*) (dd - 35)) = *((int*) (ss - 35));
        /* no break */
        case 31: *((int*) (dd - 31)) = *((int*) (ss - 31));
        /* no break */
        case 27: *((int*) (dd - 27)) = *((int*) (ss - 27));
        /* no break */
        case 23: *((int*) (dd - 23)) = *((int*) (ss - 23));
        /* no break */
        case 19: *((int*) (dd - 19)) = *((int*) (ss - 19));
        /* no break */
        case 15: *((int*) (dd - 15)) = *((int*) (ss - 15));
        /* no break */
        case 11: *((int*) (dd - 11)) = *((int*) (ss - 11));
        /* no break */
        case 7: *((int*) (dd - 7)) = *((int*) (ss - 7));
            *((int*) (dd - 4)) = *((int*) (ss - 4));
            break;
        case 3: *((short*) (dd - 3)) = *((short*) (ss - 3));
            dd[-1] = ss[-1];
            break;
        case 66: *((int*) (dd - 66)) = *((int*) (ss - 66));
        /* no break */
        case 62: *((int*) (dd - 62)) = *((int*) (ss - 62));
        /* no break */
        case 58: *((int*) (dd - 58)) = *((int*) (ss - 58));
        /* no break */
        case 54: *((int*) (dd - 54)) = *((int*) (ss - 54));
        /* no break */
        case 50: *((int*) (dd - 50)) = *((int*) (ss - 50));
        /* no break */
        case 46: *((int*) (dd - 46)) = *((int*) (ss - 46));
        /* no break */
        case 42: *((int*) (dd - 42)) = *((int*) (ss - 42));
        /* no break */
        case 38: *((int*) (dd - 38)) = *((int*) (ss - 38));
        /* no break */
        case 34: *((int*) (dd - 34)) = *((int*) (ss - 34));
        /* no break */
        case 30: *((int*) (dd - 30)) = *((int*) (ss - 30));
        /* no break */
        case 26: *((int*) (dd - 26)) = *((int*) (ss - 26));
        /* no break */
        case 22: *((int*) (dd - 22)) = *((int*) (ss - 22));
        /* no break */
        case 18: *((int*) (dd - 18)) = *((int*) (ss - 18));
        /* no break */
        case 14: *((int*) (dd - 14)) = *((int*) (ss - 14));
        /* no break */
        case 10: *((int*) (dd - 10)) = *((int*) (ss - 10));
        /* no break */
        case 6: *((int*) (dd - 6)) = *((int*) (ss - 6));
        /* no break */
        case 2: *((short*) (dd - 2)) = *((short*) (ss - 2));
            break;
        case 65: *((int*) (dd - 65)) = *((int*) (ss - 65));
        /* no break */
        case 61: *((int*) (dd - 61)) = *((int*) (ss - 61));
        /* no break */
        case 57: *((int*) (dd - 57)) = *((int*) (ss - 57));
        /* no break */
        case 53: *((int*) (dd - 53)) = *((int*) (ss - 53));
        /* no break */
        case 49: *((int*) (dd - 49)) = *((int*) (ss - 49));
        /* no break */
        case 45: *((int*) (dd - 45)) = *((int*) (ss - 45));
        /* no break */
        case 41: *((int*) (dd - 41)) = *((int*) (ss - 41));
        /* no break */
        case 37: *((int*) (dd - 37)) = *((int*) (ss - 37));
        /* no break */
        case 33: *((int*) (dd - 33)) = *((int*) (ss - 33));
        /* no break */
        case 29: *((int*) (dd - 29)) = *((int*) (ss - 29));
        /* no break */
        case 25: *((int*) (dd - 25)) = *((int*) (ss - 25));
        /* no break */
        case 21: *((int*) (dd - 21)) = *((int*) (ss - 21));
        /* no break */
        case 17: *((int*) (dd - 17)) = *((int*) (ss - 17));
        /* no break */
        case 13: *((int*) (dd - 13)) = *((int*) (ss - 13));
        /* no break */
        case 9: *((int*) (dd - 9)) = *((int*) (ss - 9));
        /* no break */
        case 5: *((int*) (dd - 5)) = *((int*) (ss - 5));
        /* no break */
        case 1: dd[-1] = ss[-1];
            break;
        case 0:
        default: break;
    }
}