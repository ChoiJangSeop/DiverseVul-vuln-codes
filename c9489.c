BROTLI_BOOL BrotliDecoderHuffmanTreeGroupInit(BrotliDecoderState* s,
    HuffmanTreeGroup* group, uint32_t alphabet_size_max,
    uint32_t alphabet_size_limit, uint32_t ntrees) {
  /* Pack two allocations into one */
  const size_t max_table_size =
      kMaxHuffmanTableSize[(alphabet_size_limit + 31) >> 5];
  const size_t code_size = sizeof(HuffmanCode) * ntrees * max_table_size;
  const size_t htree_size = sizeof(HuffmanCode*) * ntrees;
  /* Pointer alignment is, hopefully, wider than sizeof(HuffmanCode). */
  HuffmanCode** p = (HuffmanCode**)BROTLI_DECODER_ALLOC(s,
      code_size + htree_size);
  group->alphabet_size_max = (uint16_t)alphabet_size_max;
  group->alphabet_size_limit = (uint16_t)alphabet_size_limit;
  group->num_htrees = (uint16_t)ntrees;
  group->htrees = p;
  group->codes = (HuffmanCode*)(&p[ntrees]);
  return !!p;
}