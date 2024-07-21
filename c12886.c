  Compute(OpKernelContext* context, bool sorted, int k,
          const typename TTypes<T, 2>::ConstTensor& input, const int64 num_rows,
          const int64 num_cols, typename TTypes<T, 2>::Tensor values,
          typename TTypes<int, 2>::Tensor indices) {
    const CPUDevice& d = context->eigen_device<CPUDevice>();

    // Special case for k == 1.
    if (k == 1) {
#ifdef EIGEN_HAS_INDEX_LIST
      typename Eigen::IndexList<Eigen::type2index<1>> reduce_on_cols;
      typename Eigen::IndexList<int, Eigen::type2index<1>> rows_by_one;
      rows_by_one.set(0, num_rows);
#else
      Eigen::array<int, 1> reduce_on_cols = {1};
      Eigen::array<int, 2> rows_by_one = {static_cast<int>(num_rows), 1};
#endif

      values.device(d) =
          input.maximum(/*dims=*/reduce_on_cols).eval().reshape(rows_by_one);
      // Get the indices of the maximum values.
      for (int r = 0; r < num_rows; ++r) {
        indices(r, 0) = 0;
        for (int c = 0; c < num_cols; ++c) {
          if (values(r, 0) == input(r, c)) {
            indices(r, 0) = c;
            break;
          }
        }
        values(r, 0) = input(r, indices(r, 0));
      }

      return Status::OK();
    }

    auto SortIndices = [&](int start_batch, int limit_batch) {
      for (int32 b = start_batch; b < limit_batch; ++b) {
        const T* input_data = &input(b, 0);
        const auto stable_comp = [input_data](const int32 a, const int32 b) {
          if (input_data[b] < input_data[a]) {
            return true;
          } else if (input_data[b] > input_data[a]) {
            return false;
          } else {
            return a < b;
          }
        };
        const auto comp = [input_data](const int32 a, const int32 b) {
          return input_data[b] < input_data[a];
        };
        // TODO(ebrevdo): For large k < num_cols, instead of using
        // TopN, it may be faster to create a temporary vector of
        // values 0..num_cols - 1 and then use std::partial_sort_copy
        // of this into indices. Choosing the appropriate minimum k or
        // ratio of k/num_cols will require some experimentation.
        if (k == num_cols) {
          auto* begin = &indices(b, 0);
          auto* end = &indices(b, k);
          // Set the initial array of indices 0 ... k - 1.
          std::iota(begin, end, 0);
          // We want an in-place sort, but we can cheat because we're sorting
          // indices that started out sorted.  First, do a std::sort, which
          // is notably faster than std::stable_sort.
          std::sort(begin, end, comp);
          // Then, for runs of adjacent elements that were equal, sort the
          // indices in those runs in increasing order.
          for (auto* run_begin = begin; run_begin != end;) {
            auto* run_end = run_begin + 1;
            if (run_end == end) break;
            if (input_data[*run_begin] == input_data[*run_end]) {
              while (++run_end != end) {
                if (input_data[*run_begin] != input_data[*run_end]) break;
              }
              std::sort(run_begin, run_end);
            }
            run_begin = run_end;
          }
        } else {
          // Use the TopN heap object to sort.
          gtl::TopN<int32, decltype(stable_comp)> filter(k, stable_comp);
          filter.reserve(num_cols);
          for (int32 c = 0; c < num_cols; ++c) {
            filter.push(c);
          }

          int32 i = 0;
          if (sorted) {
            std::unique_ptr<std::vector<int32>> top_k(filter.Extract());
            for (auto top_k_it = top_k->begin(); top_k_it != top_k->end();
                 ++top_k_it, ++i) {
              indices(b, i) = *top_k_it;
            }
          } else {
            for (auto top_k_it = filter.unsorted_begin();
                 top_k_it != filter.unsorted_end(); ++top_k_it, ++i) {
              indices(b, i) = *top_k_it;
            }
          }
        }
        // Now that the indices are sorted, copy the values over in
        // sorted order.
        std::transform(&indices(b, 0), &indices(b, k), &values(b, 0),
                       [b, &input](const int32 loc) { return input(b, loc); });
      }  // for (int32 b = ...
    };

    // Guesstimate of cost; 4*N*log(K) where N == num_cols.
    // If K == N, assume the cost is N*log(K + 1).
    const double cmp_cost = 3 * Eigen::TensorOpCost::AddCost<int32>() +
                            Eigen::TensorOpCost::AddCost<T>();
    const double base_cost =
        cmp_cost *
        static_cast<double>(num_cols *
                            Eigen::numext::log2(static_cast<float>(k + 1)));
    const double sort_cost = (k == num_cols) ? base_cost : 4 * base_cost;
    const double copy_cost = 2 * k * Eigen::TensorOpCost::AddCost<T>();
    const double total_cost = sort_cost + copy_cost;
    const int64 final_cost = (total_cost >= static_cast<double>(kint64max))
                                 ? kint64max
                                 : static_cast<int64>(total_cost);
    auto worker_threads = *(context->device()->tensorflow_cpu_worker_threads());
    Shard(worker_threads.num_threads, worker_threads.workers, num_rows,
          final_cost, SortIndices);

    return Status::OK();
  }