QPDFWriter::calculateXrefStreamPadding(int xref_bytes)
{
    // This routine is called right after a linearization first pass
    // xref stream has been written without compression.  Calculate
    // the amount of padding that would be required in the worst case,
    // assuming the number of uncompressed bytes remains the same.
    // The worst case for zlib is that the output is larger than the
    // input by 6 bytes plus 5 bytes per 16K, and then we'll add 10
    // extra bytes for number length increases.

    return 16 + (5 * ((xref_bytes + 16383) / 16384));
}