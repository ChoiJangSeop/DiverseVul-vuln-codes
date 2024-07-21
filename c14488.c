TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);
  auto* params = reinterpret_cast<TfLiteSubParams*>(node->builtin_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);
  const TfLiteTensor* input2 = GetInput(context, node, kInputTensor2);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input2->type);
  output->type = input2->type;

  data->requires_broadcast = !HaveSameShapes(input1, input2);

  TfLiteIntArray* output_size = nullptr;
  if (data->requires_broadcast) {
    TF_LITE_ENSURE_OK(context, CalculateShapeForBroadcast(
                                   context, input1, input2, &output_size));
  } else {
    output_size = TfLiteIntArrayCopy(input1->dims);
  }

  // 8bit -> 8bit general quantized path, with general rescalings
  // as well as, 16bit -> 16bit with general rescalings
  bool pot_scale_int16 = true;

  bool input1_scale_is_pot = false;
  bool input2_scale_is_pot = false;
  bool output_scale_is_pot = false;

  int input1_scale_log2_rounded{0};
  int input2_scale_log2_rounded{0};
  int output_scale_log2_rounded{0};

  if (input1->type == kTfLiteInt16 && input2->type == kTfLiteInt16 &&
      output->type == kTfLiteInt16) {
    // In case of 16-bit, there are two implementation:
    // the scale parameter is a general number
    // the scale parameter is POT and
    // zero_point is zero for inputs/output.
    pot_scale_int16 = (input1->params.zero_point == 0) &&
                      (input2->params.zero_point == 0) &&
                      (output->params.zero_point == 0);

    input1_scale_is_pot =
        CheckedLog2(input1->params.scale, &input1_scale_log2_rounded);

    input2_scale_is_pot =
        CheckedLog2(input2->params.scale, &input2_scale_log2_rounded);

    output_scale_is_pot =
        CheckedLog2(output->params.scale, &output_scale_log2_rounded);

    pot_scale_int16 &=
        input1_scale_is_pot && input2_scale_is_pot && output_scale_is_pot;
  }

  data->pot_scale_int16 = pot_scale_int16;

  if (output->type == kTfLiteUInt8 || output->type == kTfLiteInt8 ||
      !pot_scale_int16) {
    TF_LITE_ENSURE_OK(context, PrepareGeneralSubOp(context, input1, input2,
                                                   output, params, data, -1));
  } else if (output->type == kTfLiteInt16) {
    // LSTM-special case with scale parameter of POT
    TF_LITE_ENSURE_OK(context, PrepareInt16SubOpPOT(context, input1, input2,
                                                    output, params, data));
  }

  return context->ResizeTensor(context, output, output_size);
}