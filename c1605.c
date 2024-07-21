static Bigint *
Balloc(int k)
{
    int x;
    Bigint *rv;
#ifndef Omit_Private_Memory
    size_t len;
#endif

    ACQUIRE_DTOA_LOCK(0);
    if ((rv = freelist[k]) != 0) {
        freelist[k] = rv->next;
    }
    else {
        x = 1 << k;
#ifdef Omit_Private_Memory
        rv = (Bigint *)MALLOC(sizeof(Bigint) + (x-1)*sizeof(ULong));
#else
        len = (sizeof(Bigint) + (x-1)*sizeof(ULong) + sizeof(double) - 1)
                /sizeof(double);
        if (pmem_next - private_mem + len <= PRIVATE_mem) {
            rv = (Bigint*)pmem_next;
            pmem_next += len;
        }
        else
            rv = (Bigint*)MALLOC(len*sizeof(double));
#endif
        rv->k = k;
        rv->maxwds = x;
    }
    FREE_DTOA_LOCK(0);
    rv->sign = rv->wds = 0;
    return rv;