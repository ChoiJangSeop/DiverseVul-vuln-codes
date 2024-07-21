inline void GatherNd(const RuntimeShape& params_shape,
                     const ParamsT* params_data,
                     const RuntimeShape& indices_shape,
                     const IndicesT* indices_data,
                     const RuntimeShape& output_shape, ParamsT* output_data) {
  ruy::profiler::ScopeLabel label("GatherNd");

  const GatherNdHelperResult res = GatherNdHelper(params_shape, indices_shape);
  for (int i = 0; i < res.n_slices; ++i) {
    int from_pos = 0;
    for (int j = 0; j < res.indices_nd; ++j) {
      from_pos += indices_data[i * res.indices_nd + j] * res.dims_to_count[j];
    }
    std::memcpy(output_data + i * res.slice_size, params_data + from_pos,
                sizeof(ParamsT) * res.slice_size);
  }
}