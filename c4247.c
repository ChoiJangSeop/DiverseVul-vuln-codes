local int decode(struct state *s, struct huffman *h)
{
    int len;            /* current number of bits in code */
    int code;           /* len bits being decoded */
    int first;          /* first code of length len */
    int count;          /* number of codes of length len */
    int index;          /* index of first code of length len in symbol table */

    code = first = index = 0;
    for (len = 1; len <= MAXBITS; len++) {
        code |= bits(s, 1);             /* get next bit */
        count = h->count[len];
        if (code < first + count)       /* if length len, return symbol */
            return h->symbol[index + (code - first)];
        index += count;                 /* else update for next length */
        first += count;
        first <<= 1;
        code <<= 1;
    }
    return -9;                          /* ran out of codes */
}