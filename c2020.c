void LZ4_init(LZ4_stream_t_internal* lz4ds, const BYTE* base)
{
    MEM_INIT(lz4ds->hashTable, 0, LZ4_STREAMSIZE);
    lz4ds->bufferStart = base;
}