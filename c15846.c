void DoNonMaxSuppressionOp(OpKernelContext* context, const Tensor& scores,
                           int num_boxes, const Tensor& max_output_size,
                           const T similarity_threshold,
                           const T score_threshold, const T soft_nms_sigma,
                           const std::function<float(int, int)>& similarity_fn,
                           bool return_scores_tensor = false,
                           bool pad_to_max_output_size = false,
                           int* ptr_num_valid_outputs = nullptr) {
  const int output_size = max_output_size.scalar<int>()();

  std::vector<T> scores_data(num_boxes);
  std::copy_n(scores.flat<T>().data(), num_boxes, scores_data.begin());

  // Data structure for a selection candidate in NMS.
  struct Candidate {
    int box_index;
    T score;
    int suppress_begin_index;
  };

  auto cmp = [](const Candidate bs_i, const Candidate bs_j) {
    return ((bs_i.score == bs_j.score) && (bs_i.box_index > bs_j.box_index)) ||
           bs_i.score < bs_j.score;
  };
  std::priority_queue<Candidate, std::deque<Candidate>, decltype(cmp)>
      candidate_priority_queue(cmp);
  for (int i = 0; i < scores_data.size(); ++i) {
    if (scores_data[i] > score_threshold) {
      candidate_priority_queue.emplace(Candidate({i, scores_data[i], 0}));
    }
  }

  T scale = static_cast<T>(0.0);
  bool is_soft_nms = soft_nms_sigma > static_cast<T>(0.0);
  if (is_soft_nms) {
    scale = static_cast<T>(-0.5) / soft_nms_sigma;
  }

  auto suppress_weight = [similarity_threshold, scale,
                          is_soft_nms](const T sim) {
    const T weight = Eigen::numext::exp<T>(scale * sim * sim);
    return is_soft_nms || sim <= similarity_threshold ? weight
                                                      : static_cast<T>(0.0);
  };

  std::vector<int> selected;
  std::vector<T> selected_scores;
  float similarity;
  T original_score;
  Candidate next_candidate;

  while (selected.size() < output_size && !candidate_priority_queue.empty()) {
    next_candidate = candidate_priority_queue.top();
    original_score = next_candidate.score;
    candidate_priority_queue.pop();

    // Overlapping boxes are likely to have similar scores, therefore we
    // iterate through the previously selected boxes backwards in order to
    // see if `next_candidate` should be suppressed. We also enforce a property
    // that a candidate can be suppressed by another candidate no more than
    // once via `suppress_begin_index` which tracks which previously selected
    // boxes have already been compared against next_candidate prior to a given
    // iteration.  These previous selected boxes are then skipped over in the
    // following loop.
    bool should_hard_suppress = false;
    for (int j = static_cast<int>(selected.size()) - 1;
         j >= next_candidate.suppress_begin_index; --j) {
      similarity = similarity_fn(next_candidate.box_index, selected[j]);

      next_candidate.score *= suppress_weight(static_cast<T>(similarity));

      // First decide whether to perform hard suppression
      if (!is_soft_nms && static_cast<T>(similarity) > similarity_threshold) {
        should_hard_suppress = true;
        break;
      }

      // If next_candidate survives hard suppression, apply soft suppression
      if (next_candidate.score <= score_threshold) break;
    }
    // If `next_candidate.score` has not dropped below `score_threshold`
    // by this point, then we know that we went through all of the previous
    // selections and can safely update `suppress_begin_index` to
    // `selected.size()`. If on the other hand `next_candidate.score`
    // *has* dropped below the score threshold, then since `suppress_weight`
    // always returns values in [0, 1], further suppression by items that were
    // not covered in the above for loop would not have caused the algorithm
    // to select this item. We thus do the same update to
    // `suppress_begin_index`, but really, this element will not be added back
    // into the priority queue in the following.
    next_candidate.suppress_begin_index = selected.size();

    if (!should_hard_suppress) {
      if (next_candidate.score == original_score) {
        // Suppression has not occurred, so select next_candidate
        selected.push_back(next_candidate.box_index);
        selected_scores.push_back(next_candidate.score);
        continue;
      }
      if (next_candidate.score > score_threshold) {
        // Soft suppression has occurred and current score is still greater than
        // score_threshold; add next_candidate back onto priority queue.
        candidate_priority_queue.push(next_candidate);
      }
    }
  }

  int num_valid_outputs = selected.size();
  if (pad_to_max_output_size) {
    selected.resize(output_size, 0);
    selected_scores.resize(output_size, static_cast<T>(0));
  }
  if (ptr_num_valid_outputs) {
    *ptr_num_valid_outputs = num_valid_outputs;
  }

  // Allocate output tensors
  Tensor* output_indices = nullptr;
  TensorShape output_shape({static_cast<int>(selected.size())});
  OP_REQUIRES_OK(context,
                 context->allocate_output(0, output_shape, &output_indices));
  TTypes<int, 1>::Tensor output_indices_data = output_indices->tensor<int, 1>();
  std::copy_n(selected.begin(), selected.size(), output_indices_data.data());

  if (return_scores_tensor) {
    Tensor* output_scores = nullptr;
    OP_REQUIRES_OK(context,
                   context->allocate_output(1, output_shape, &output_scores));
    typename TTypes<T, 1>::Tensor output_scores_data =
        output_scores->tensor<T, 1>();
    std::copy_n(selected_scores.begin(), selected_scores.size(),
                output_scores_data.data());
  }
}