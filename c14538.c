TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  ruy::profiler::ScopeLabel label("MirrorPad");
  const TfLiteTensor* input_tensor = GetInput(context, node, 0);
  const TfLiteTensor* padding_matrix = GetInput(context, node, 1);
  auto* params =
      reinterpret_cast<TfLiteMirrorPaddingParams*>(node->builtin_data);

  if (params == nullptr) {
    return kTfLiteError;
  }
  const int input_dims = NumDimensions(input_tensor);

  TfLiteTensor* output_tensor = GetOutput(context, node, 0);
  if (IsDynamicTensor(output_tensor)) {
    auto output_size = GetPaddedOutputShape(input_tensor, padding_matrix);
    if (output_size == nullptr) {
      return kTfLiteError;
    }
    TF_LITE_ENSURE_STATUS(
        context->ResizeTensor(context, output_tensor, output_size.release()));
  }

  std::vector<int> output_dims_num_elements(input_dims, 1);
  std::vector<int> input_dims_num_elements(input_dims, 1);
  for (int i = input_dims - 2; i >= 0; i--) {
    output_dims_num_elements[i] =
        output_dims_num_elements[i + 1] * output_tensor->dims->data[i + 1];
    input_dims_num_elements[i] =
        input_dims_num_elements[i + 1] * input_tensor->dims->data[i + 1];
  }

  const int offset =
      params->mode != TfLiteMirrorPaddingMode::kTfLiteMirrorPaddingReflect ? 0
                                                                           : 1;

  CpuBackendContext* cpu_backend_context =
      CpuBackendContext::GetFromContext(context);
  const int thread_count = cpu_backend_context->max_num_threads();
  TfLiteStatus status = kTfLiteOk;
  const int output_size = NumElements(output_tensor);
#define TF_LITE_MIRROR_PAD(type)                                           \
  EvalData<type> eval_data;                                                \
  eval_data.input_data = GetTensorData<type>(input_tensor);                \
  eval_data.input_dims = input_tensor->dims;                               \
  eval_data.input_dims = input_tensor->dims;                               \
  eval_data.output_dims_num_elements = &output_dims_num_elements;          \
  eval_data.input_dims_num_elements = &input_dims_num_elements;            \
  eval_data.num_dims = input_dims;                                         \
  eval_data.offset = offset;                                               \
  eval_data.output_data = GetTensorData<type>(output_tensor);              \
  eval_data.padding_matrix = padding_matrix;                               \
  std::vector<MirrorPadWorkerTask<type>> tasks;                            \
  tasks.reserve(thread_count);                                             \
  int start = 0;                                                           \
  for (int i = 0; i < thread_count; ++i) {                                 \
    int end = start + (output_size - start) / (thread_count - i);          \
    tasks.emplace_back(MirrorPadWorkerTask<type>(&eval_data, start, end)); \
    start = end;                                                           \
  }                                                                        \
  cpu_backend_threadpool::Execute(tasks.size(), tasks.data(),              \
                                  cpu_backend_context);

  switch (output_tensor->type) {
    case kTfLiteFloat32: {
      TF_LITE_MIRROR_PAD(float);
      break;
    }
    case kTfLiteInt32: {
      TF_LITE_MIRROR_PAD(int32_t);
      break;
    }
    case kTfLiteUInt8: {
      TF_LITE_MIRROR_PAD(uint8_t);
      break;
    }
    case kTfLiteInt8: {
      TF_LITE_MIRROR_PAD(int8_t);
      break;
    }
    case kTfLiteInt64: {
      TF_LITE_MIRROR_PAD(int64_t);
      break;
    }
    default:
      status = kTfLiteError;
      break;
  }
#undef TF_LITE_MIRROR_PAD
  return status;
}