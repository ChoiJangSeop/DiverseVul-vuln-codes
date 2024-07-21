static void* BrotliDefaultAllocFunc(void* opaque, size_t size) {
  BROTLI_UNUSED(opaque);
  return malloc(size);
}