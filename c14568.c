static TfLiteStatus InitTemporaryTensors(TfLiteContext* context,
                                         TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);
  // The prepare function may be executed multiple times. But temporary tensors
  // only need to be initiated once.
  if (data->fft_integer_working_area_id != kTensorNotAllocated &&
      data->fft_double_working_area_id != kTensorNotAllocated) {
    return kTfLiteOk;
  }

  TfLiteIntArrayFree(node->temporaries);
  // Create two temporary tensors.
  node->temporaries = TfLiteIntArrayCreate(2);
  int first_new_index;
  TF_LITE_ENSURE_STATUS(context->AddTensors(context, 2, &first_new_index));
  node->temporaries->data[kFftIntegerWorkingAreaTensor] = first_new_index;
  data->fft_integer_working_area_id = first_new_index;
  node->temporaries->data[kFftDoubleWorkingAreaTensor] = first_new_index + 1;
  data->fft_double_working_area_id = first_new_index + 1;

  // Set up FFT integer working area buffer.
  TfLiteTensor* fft_integer_working_area =
      GetTemporary(context, node, kFftIntegerWorkingAreaTensor);
  fft_integer_working_area->type = kTfLiteInt32;
  // If fft_length is not a constant tensor, fft_integer_working_area will be
  // set to dynamic later in Prepare.
  fft_integer_working_area->allocation_type = kTfLiteArenaRw;

  // Set up FFT double working area buffer.
  TfLiteTensor* fft_double_working_area =
      GetTemporary(context, node, kFftDoubleWorkingAreaTensor);
  // fft_double_working_area is a double tensor. Ideally, double should be
  // added into tflite data types. However, since fft_double_working_area is a
  // temporary tensor, and there are no ops having double input/output tensors
  // in tflite at this point, adding double as a tflite data type may confuse
  // users that double is supported. As a results, kTfLiteInt64 is used here
  // for memory allocation. And it will be cast into double in Eval when being
  // used.
  fft_double_working_area->type = kTfLiteInt64;
  // If fft_length is not a constant tensor, fft_double_working_area will be
  // set to dynamic later in Prepare.
  fft_double_working_area->allocation_type = kTfLiteArenaRw;

  return kTfLiteOk;
}