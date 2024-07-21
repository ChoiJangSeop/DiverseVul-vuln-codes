local int fixed(struct state *s)
{
    static int virgin = 1;
    static boost::int16_t lencnt[MAXBITS+1], lensym[FIXLCODES];
    static boost::int16_t distcnt[MAXBITS+1], distsym[MAXDCODES];
    static struct huffman lencode = {lencnt, lensym};
    static struct huffman distcode = {distcnt, distsym};

    /* build fixed huffman tables if first call (may not be thread safe) */
    if (virgin) {
        int symbol;
        boost::int16_t lengths[FIXLCODES];

        /* literal/length table */
        for (symbol = 0; symbol < 144; symbol++)
            lengths[symbol] = 8;
        for (; symbol < 256; symbol++)
            lengths[symbol] = 9;
        for (; symbol < 280; symbol++)
            lengths[symbol] = 7;
        for (; symbol < FIXLCODES; symbol++)
            lengths[symbol] = 8;
        construct(&lencode, lengths, FIXLCODES);

        /* distance table */
        for (symbol = 0; symbol < MAXDCODES; symbol++)
            lengths[symbol] = 5;
        construct(&distcode, lengths, MAXDCODES);

        /* do this just once */
        virgin = 0;
    }

    /* decode data until end-of-block code */
    return codes(s, &lencode, &distcode);
}