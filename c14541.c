TfLiteStatus PreluPrepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  const TfLiteTensor* alpha = GetInput(context, node, 1);
  PreluOpData* data = reinterpret_cast<PreluOpData*>(node->user_data);

  TF_LITE_ENSURE_TYPES_EQ(context, input->type, alpha->type);

  output->type = input->type;

  if (output->type == kTfLiteUInt8 || output->type == kTfLiteInt8 ||
      output->type == kTfLiteInt16) {
    // prelu(x) = x if x >= 0 else x * alpha.
    // So if we translate that for quantized computation:
    //
    // input_float = (input_q - input_zp) * input_scale
    // output_float = (output_q - output_zp) * output_scale
    // alpha_float = (alpha_q - alpha_zp) * alpha_scale
    //
    // When input_q - input_zp >= 0:
    // ouput_q = (input_q - input_zp) * input_scale / output_scale + output_q
    // else:
    // output_q = (input_q - input_zp) * (alpha_q - alpha_zp) * input_scale
    //            * alpha_scale / output_scale + output_q
    //
    // So for input_q - input_zp >= 0:
    // output real multiplier 1 is input_scale / output_scale;
    // for input_q - input_zp < 0:
    // output real multiplier 2 is input_scale  * alpha_scale/ output_scale.
    double real_multiplier_1 = input->params.scale / output->params.scale;
    double real_multiplier_2 =
        input->params.scale * alpha->params.scale / output->params.scale;
    QuantizeMultiplier(real_multiplier_1, &data->output_multiplier_1,
                       &data->output_shift_1);
    QuantizeMultiplier(real_multiplier_2, &data->output_multiplier_2,
                       &data->output_shift_2);
  }

  data->requires_broadcast = !HaveSameShapes(input, alpha);
  // PRelu (parameteric Relu) shares the same alpha value on "shared axis".
  // This means it's always required to "broadcast" alpha values in PRelu.
  TfLiteIntArray* output_size = nullptr;
  TF_LITE_ENSURE_OK(
      context, CalculateShapeForBroadcast(context, input, alpha, &output_size));

  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, output, output_size));
  // After broadcasting, the output shape should always be the same as the
  // input shape.
  TF_LITE_ENSURE(context, HaveSameShapes(input, output));

  return kTfLiteOk;
}