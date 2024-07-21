__global__ void UnsortedSegmentCustomKernel(const Index input_outer_dim_size,
                                            const Index inner_dim_size,
                                            const Index output_outer_dim_size,
                                            const Index* segment_ids,
                                            const T* input, T* output) {
  const Index input_total_size = input_outer_dim_size * inner_dim_size;
  const Index output_total_size = output_outer_dim_size * inner_dim_size;
  for (int input_index : GpuGridRangeX(input_total_size)) {
    const Index input_segment_index = input_index / inner_dim_size;
    const Index segment_offset = input_index % inner_dim_size;
    const Index output_segment_index = segment_ids[input_segment_index];
    if (output_segment_index < 0 || output_segment_index >= output_total_size) {
      continue;
    }
    const Index output_index =
        output_segment_index * inner_dim_size + segment_offset;
    KernelReductionFunctor()(output + output_index, ldg(input + input_index));
  }
}