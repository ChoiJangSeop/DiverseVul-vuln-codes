static void HeaderMapImplGetByteSize(benchmark::State& state) {
  HeaderMapImpl headers;
  addDummyHeaders(headers, state.range(0));
  uint64_t size = 0;
  for (auto _ : state) {
    size += headers.byteSize();
  }
  benchmark::DoNotOptimize(size);
}