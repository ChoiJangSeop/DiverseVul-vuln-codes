static void BrotliDefaultFreeFunc(void* opaque, void* address) {
  BROTLI_UNUSED(opaque);
  free(address);
}