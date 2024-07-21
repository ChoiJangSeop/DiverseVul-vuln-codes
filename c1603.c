static void
Bfree(Bigint *v)
{
    if (v) {
        ACQUIRE_DTOA_LOCK(0);
        v->next = freelist[v->k];
        freelist[v->k] = v;
        FREE_DTOA_LOCK(0);
    }