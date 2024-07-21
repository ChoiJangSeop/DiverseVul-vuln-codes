int fbytes(unsigned char *buf, int num)
{
    int ret;
    BIGNUM *tmp = NULL;

    if (fbytes_counter >= 8)
        return 0;
    tmp = BN_new();
    if (!tmp)
        return 0;
    if (!BN_dec2bn(&tmp, numbers[fbytes_counter])) {
        BN_free(tmp);
        return 0;
    }
    fbytes_counter++;
    if (num != BN_num_bytes(tmp) || !BN_bn2bin(tmp, buf))
        ret = 0;
    else
        ret = 1;
    if (tmp)
        BN_free(tmp);
    return ret;
}