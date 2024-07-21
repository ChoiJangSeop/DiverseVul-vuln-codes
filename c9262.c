int blobAddData(blob *b, const unsigned char *data, size_t len)
{
#if HAVE_CLI_GETPAGESIZE
    static int pagesize;
    int growth;
#endif

    assert(b != NULL);
#ifdef CL_DEBUG
    assert(b->magic == BLOBCLASS);
#endif
    assert(data != NULL);

    if (len == 0)
        return 0;

    if (b->isClosed) {
        /*
		 * Should be cli_dbgmsg, but I want to see them for now,
		 * and cli_dbgmsg doesn't support debug levels
		 */
        cli_warnmsg("Reopening closed blob\n");
        b->isClosed = 0;
    }
    /*
	 * The payoff here is between reducing the number of calls to
	 * malloc/realloc and not overallocating memory. A lot of machines
	 * are more tight with memory than one may imagine which is why
	 * we don't just allocate a *huge* amount and be done with it. Closing
	 * the blob helps because that reclaims memory. If you know the maximum
	 * size of a blob before you start adding data, use blobGrow() that's
	 * the most optimum
	 */
#if HAVE_CLI_GETPAGESIZE
    if (pagesize == 0) {
        pagesize = cli_getpagesize();
        if (pagesize == 0)
            pagesize = 4096;
    }
    growth = pagesize;
    if (len >= (size_t)pagesize)
        growth = ((len / pagesize) + 1) * pagesize;

    /*cli_dbgmsg("blobGrow: b->size %lu, b->len %lu, len %lu, growth = %u\n",
		b->size, b->len, len, growth);*/

    if (b->data == NULL) {
        assert(b->len == 0);
        assert(b->size == 0);

        b->size = growth;
        b->data = cli_malloc(growth);
    } else if (b->size < b->len + (off_t)len) {
        unsigned char *p = cli_realloc(b->data, b->size + growth);

        if (p == NULL)
            return -1;

        b->size += growth;
        b->data = p;
    }
#else
    if (b->data == NULL) {
        assert(b->len == 0);
        assert(b->size == 0);

        b->size = (off_t)len * 4;
        b->data = cli_malloc(b->size);
    } else if (b->size < b->len + (off_t)len) {
        unsigned char *p = cli_realloc(b->data, b->size + (len * 4));

        if (p == NULL)
            return -1;

        b->size += (off_t)len * 4;
        b->data = p;
    }
#endif

    if (b->data) {
        memcpy(&b->data[b->len], data, len);
        b->len += (off_t)len;
    }
    return 0;
}