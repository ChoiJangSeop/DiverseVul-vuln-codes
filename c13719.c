void SpatialAvgPool(OpKernelContext* context, Tensor* output,
                    const Tensor& input, const PoolParameters& params,
                    const Padding& padding) {
  typedef Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>>
      ConstEigenMatrixMap;
  typedef Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>>
      EigenMatrixMap;

  auto in_flat = input.flat<T>();
  auto out_flat = output->flat<T>();

  auto shard = [&params, &in_flat, &out_flat](int64_t start, int64_t limit) {
    // Calculate indices for this shards chunk of work.
    const int64_t input_image_size =
        params.tensor_in_rows * params.tensor_in_cols * params.depth;
    const int64_t output_image_size =
        params.out_width * params.out_height * params.depth;
    const int64_t shard_batch_size = limit - start;

    ConstEigenMatrixMap in_mat(
        in_flat.data() + start * input_image_size, params.depth,
        params.tensor_in_cols * params.tensor_in_rows * shard_batch_size);
    EigenMatrixMap out_mat(
        out_flat.data() + start * output_image_size, params.depth,
        params.out_width * params.out_height * shard_batch_size);
    Eigen::Matrix<T, Eigen::Dynamic, 1> out_count(out_mat.cols());
    out_count.setZero();

    // Initializes output to zero.
    out_mat.setZero();

    // The following code basically does the following:
    // 1. Flattens the input and output tensors into two dimensional arrays.
    //    tensor_in_as_matrix:
    //      depth by (tensor_in_cols * tensor_in_rows * tensor_in_batch)
    //    output_as_matrix:
    //      depth by (out_width * out_height * tensor_in_batch)
    //
    // 2. Walks through the set of columns in the flattened
    // tensor_in_as_matrix,
    //    and updates the corresponding column(s) in output_as_matrix with the
    //    average value.
    for (int b = 0; b < shard_batch_size; ++b) {
      for (int h = 0; h < params.tensor_in_rows; ++h) {
        for (int w = 0; w < params.tensor_in_cols; ++w) {
          // (h_start, h_end) * (w_start, w_end) is the range that the input
          // vector projects to.
          const int hpad = h + params.pad_top;
          const int wpad = w + params.pad_left;
          const int h_start =
              (hpad < params.window_rows)
                  ? 0
                  : (hpad - params.window_rows) / params.row_stride + 1;
          const int h_end =
              std::min<int>(hpad / params.row_stride + 1, params.out_height);
          const int w_start =
              (wpad < params.window_cols)
                  ? 0
                  : (wpad - params.window_cols) / params.col_stride + 1;
          const int w_end =
              std::min<int>(wpad / params.col_stride + 1, params.out_width);
          const int in_offset =
              (b * params.tensor_in_rows + h) * params.tensor_in_cols + w;
          Eigen::DSizes<Eigen::DenseIndex, 2> in_indices(0, in_offset);
          for (int ph = h_start; ph < h_end; ++ph) {
            for (int pw = w_start; pw < w_end; ++pw) {
              const int out_offset =
                  (b * params.out_height + ph) * params.out_width + pw;
              out_mat.col(out_offset) += in_mat.col(in_offset);
              out_count(out_offset) += T(1);
            }
          }
        }
      }
    }

    DCHECK_GT(out_count.minCoeff(), T(0));
    out_mat.array().rowwise() /= out_count.transpose().array();
  };

  const int64_t work_unit_size =
      params.tensor_in_rows * params.tensor_in_cols * params.depth;
  // NOTE: Constants in calculation below were estimated based on benchmarking.
  // Nanoseconds/work_unit for benchmarks ranged from 0.01 to 0.001, and
  // so the factor 0.01 (i.e. 1/100) with a max of 10000, was chosen to limit
  // the work unit cost to an operating range in which it empirically performed
  // best.
  const int64_t work_unit_cost = std::max(int64_t{10000}, work_unit_size / 100);
  const DeviceBase::CpuWorkerThreads& worker_threads =
      *(context->device()->tensorflow_cpu_worker_threads());
  Shard(worker_threads.num_threads, worker_threads.workers,
        params.tensor_in_batch, work_unit_cost, shard);
}