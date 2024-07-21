TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteSkipGramParams*>(node->builtin_data);

  // Split sentence to words.
  std::vector<StringRef> words;
  tflite::StringRef strref = tflite::GetString(GetInput(context, node, 0), 0);
  int prev_idx = 0;
  for (int i = 1; i < strref.len; i++) {
    if (isspace(*(strref.str + i))) {
      if (i > prev_idx && !isspace(*(strref.str + prev_idx))) {
        words.push_back({strref.str + prev_idx, i - prev_idx});
      }
      prev_idx = i + 1;
    }
  }
  if (strref.len > prev_idx) {
    words.push_back({strref.str + prev_idx, strref.len - prev_idx});
  }

  // Generate n-grams recursively.
  tflite::DynamicBuffer buf;
  if (words.size() < params->ngram_size) {
    buf.WriteToTensorAsVector(GetOutput(context, node, 0));
    return kTfLiteOk;
  }

  // Stack stores the index of word used to generate ngram.
  // The size of stack is the size of ngram.
  std::vector<int> stack(params->ngram_size, 0);
  // Stack index that indicates which depth the recursion is operating at.
  int stack_idx = 1;
  int num_words = words.size();

  while (stack_idx >= 0) {
    if (ShouldStepInRecursion(params, stack, stack_idx, num_words)) {
      // When current depth can fill with a new word
      // and the new word is within the max range to skip,
      // fill this word to stack, recurse into next depth.
      stack[stack_idx]++;
      stack_idx++;
      if (stack_idx < params->ngram_size) {
        stack[stack_idx] = stack[stack_idx - 1];
      }
    } else {
      if (ShouldIncludeCurrentNgram(params, stack_idx)) {
        // Add n-gram to tensor buffer when the stack has filled with enough
        // words to generate the ngram.
        std::vector<StringRef> gram(stack_idx);
        for (int i = 0; i < stack_idx; i++) {
          gram[i] = words[stack[i]];
        }
        buf.AddJoinedString(gram, ' ');
      }
      // When current depth cannot fill with a valid new word,
      // and not in last depth to generate ngram,
      // step back to previous depth to iterate to next possible word.
      stack_idx--;
    }
  }

  buf.WriteToTensorAsVector(GetOutput(context, node, 0));
  return kTfLiteOk;
}