TfLiteStatus CheckLstmTensorDimensionsAndTypes(
    TfLiteContext* context, TfLiteNode* node, int n_input, int n_output,
    int n_cell, int input_to_input_weights_tensor,
    int input_to_forget_weights_tensor, int input_to_cell_weights_tensor,
    int input_to_output_weights_tensor, int recurrent_to_input_weights_tensor,
    int recurrent_to_forget_weights_tensor,
    int recurrent_to_cell_weights_tensor,
    int recurrent_to_output_weights_tensor, int cell_to_input_weights_tensor,
    int cell_to_forget_weights_tensor, int cell_to_output_weights_tensor,
    int input_gate_bias_tensor, int forget_gate_bias_tensor,
    int cell_gate_bias_tensor, int output_gate_bias_tensor,
    int projection_weights_tensor, int projection_bias_tensor) {
  const auto* params = reinterpret_cast<TfLiteBidirectionalSequenceLSTMParams*>(
      node->builtin_data);

  // Making sure clipping parameters have valid values.
  // == 0 means no clipping
  //  > 0 means clipping
  TF_LITE_ENSURE(context, params->cell_clip >= 0);
  TF_LITE_ENSURE(context, params->proj_clip >= 0);

  const TfLiteTensor* input_to_forget_weights =
      GetInput(context, node, input_to_forget_weights_tensor);
  TF_LITE_ENSURE_EQ(context, input_to_forget_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, input_to_forget_weights->dims->data[0], n_cell);
  TF_LITE_ENSURE_EQ(context, input_to_forget_weights->dims->data[1], n_input);
  TF_LITE_ENSURE(context, (input_to_forget_weights->type == kTfLiteFloat32) ||
                              (input_to_forget_weights->type == kTfLiteInt8) ||
                              (input_to_forget_weights->type == kTfLiteUInt8));

  const TfLiteTensor* input_to_input_weights =
      GetOptionalInputTensor(context, node, input_to_input_weights_tensor);
  if (input_to_input_weights != nullptr) {
    TF_LITE_ENSURE_EQ(context, input_to_input_weights->dims->size, 2);
    TF_LITE_ENSURE_EQ(context, input_to_input_weights->dims->data[0], n_cell);
    TF_LITE_ENSURE_EQ(context, input_to_input_weights->dims->data[1], n_input);
    TF_LITE_ENSURE_TYPES_EQ(context, input_to_input_weights->type,
                            input_to_forget_weights->type);
  }

  const TfLiteTensor* input_to_cell_weights =
      GetInput(context, node, input_to_cell_weights_tensor);
  TF_LITE_ENSURE_EQ(context, input_to_cell_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, input_to_cell_weights->dims->data[0], n_cell);
  TF_LITE_ENSURE_EQ(context, input_to_cell_weights->dims->data[1], n_input);
  TF_LITE_ENSURE_TYPES_EQ(context, input_to_cell_weights->type,
                          input_to_forget_weights->type);

  const TfLiteTensor* input_to_output_weights =
      GetInput(context, node, input_to_output_weights_tensor);
  TF_LITE_ENSURE_EQ(context, input_to_output_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, input_to_output_weights->dims->data[0], n_cell);
  TF_LITE_ENSURE_EQ(context, input_to_output_weights->dims->data[1], n_input);
  TF_LITE_ENSURE_TYPES_EQ(context, input_to_output_weights->type,
                          input_to_forget_weights->type);

  const TfLiteTensor* recurrent_to_input_weights =
      GetOptionalInputTensor(context, node, recurrent_to_input_weights_tensor);
  if (recurrent_to_input_weights != nullptr) {
    TF_LITE_ENSURE_EQ(context, recurrent_to_input_weights->dims->size, 2);
    TF_LITE_ENSURE_EQ(context, recurrent_to_input_weights->dims->data[0],
                      n_cell);
    TF_LITE_ENSURE_EQ(context, recurrent_to_input_weights->dims->data[1],
                      n_output);
    TF_LITE_ENSURE_TYPES_EQ(context, recurrent_to_input_weights->type,
                            input_to_forget_weights->type);
  }

  const TfLiteTensor* recurrent_to_forget_weights =
      GetInput(context, node, recurrent_to_forget_weights_tensor);
  TF_LITE_ENSURE_EQ(context, recurrent_to_forget_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, recurrent_to_forget_weights->dims->data[0],
                    n_cell);
  TF_LITE_ENSURE_EQ(context, recurrent_to_forget_weights->dims->data[1],
                    n_output);
  TF_LITE_ENSURE_TYPES_EQ(context, recurrent_to_forget_weights->type,
                          input_to_forget_weights->type);

  const TfLiteTensor* recurrent_to_cell_weights =
      GetInput(context, node, recurrent_to_cell_weights_tensor);
  TF_LITE_ENSURE_EQ(context, recurrent_to_cell_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, recurrent_to_cell_weights->dims->data[0], n_cell);
  TF_LITE_ENSURE_EQ(context, recurrent_to_cell_weights->dims->data[1],
                    n_output);
  TF_LITE_ENSURE_TYPES_EQ(context, recurrent_to_cell_weights->type,
                          input_to_forget_weights->type);

  // We make sure the input-gate's parameters are either both present (regular
  // LSTM) or not at all (CIFG-LSTM).
  const bool cifg_weights_all_or_none =
      ((input_to_input_weights != nullptr) &&
       (recurrent_to_input_weights != nullptr)) ||
      ((input_to_input_weights == nullptr) &&
       (recurrent_to_input_weights == nullptr));
  TF_LITE_ENSURE(context, cifg_weights_all_or_none == true);

  const TfLiteTensor* cell_to_input_weights =
      GetOptionalInputTensor(context, node, cell_to_input_weights_tensor);
  if (cell_to_input_weights != nullptr) {
    TF_LITE_ENSURE_EQ(context, cell_to_input_weights->dims->size, 1);
    TF_LITE_ENSURE_EQ(context, cell_to_input_weights->dims->data[0], n_cell);
    TF_LITE_ENSURE_TYPES_EQ(context, cell_to_input_weights->type,
                            input_to_forget_weights->type);
  }

  const TfLiteTensor* cell_to_forget_weights =
      GetOptionalInputTensor(context, node, cell_to_forget_weights_tensor);
  if (cell_to_forget_weights != nullptr) {
    TF_LITE_ENSURE_EQ(context, cell_to_forget_weights->dims->size, 1);
    TF_LITE_ENSURE_EQ(context, cell_to_forget_weights->dims->data[0], n_cell);
    TF_LITE_ENSURE_TYPES_EQ(context, cell_to_forget_weights->type,
                            input_to_forget_weights->type);
  }

  const TfLiteTensor* cell_to_output_weights =
      GetOptionalInputTensor(context, node, cell_to_output_weights_tensor);
  if (cell_to_output_weights != nullptr) {
    TF_LITE_ENSURE_EQ(context, cell_to_output_weights->dims->size, 1);
    TF_LITE_ENSURE_EQ(context, cell_to_output_weights->dims->data[0], n_cell);
    TF_LITE_ENSURE_TYPES_EQ(context, cell_to_output_weights->type,
                            input_to_forget_weights->type);
  }

  // Making sure the peephole weights are there all or none.
  const bool use_cifg = (input_to_input_weights == nullptr);
  const bool peephole_weights_all_or_none =
      ((cell_to_input_weights != nullptr || use_cifg) &&
       (cell_to_forget_weights != nullptr) &&
       (cell_to_output_weights != nullptr)) ||
      ((cell_to_input_weights == nullptr) &&
       (cell_to_forget_weights == nullptr) &&
       (cell_to_output_weights == nullptr));
  TF_LITE_ENSURE(context, peephole_weights_all_or_none == true);

  // Make sure the input gate bias is present only when not a CIFG-LSTM.
  const TfLiteTensor* input_gate_bias =
      GetOptionalInputTensor(context, node, input_gate_bias_tensor);
  if (use_cifg) {
    TF_LITE_ENSURE_EQ(context, input_gate_bias, nullptr);
  } else {
    TF_LITE_ENSURE_EQ(context, input_gate_bias->dims->size, 1);
    TF_LITE_ENSURE_EQ(context, input_gate_bias->dims->data[0], n_cell);
    TF_LITE_ENSURE_TYPES_EQ(context, input_gate_bias->type, kTfLiteFloat32);
  }

  const TfLiteTensor* forget_gate_bias =
      GetInput(context, node, forget_gate_bias_tensor);
  TF_LITE_ENSURE_EQ(context, forget_gate_bias->dims->size, 1);
  TF_LITE_ENSURE_EQ(context, forget_gate_bias->dims->data[0], n_cell);
  TF_LITE_ENSURE_TYPES_EQ(context, forget_gate_bias->type, kTfLiteFloat32);

  const TfLiteTensor* cell_gate_bias =
      GetInput(context, node, cell_gate_bias_tensor);
  TF_LITE_ENSURE_EQ(context, cell_gate_bias->dims->size, 1);
  TF_LITE_ENSURE_EQ(context, cell_gate_bias->dims->data[0], n_cell);
  TF_LITE_ENSURE_EQ(context, cell_gate_bias->type, kTfLiteFloat32);

  const TfLiteTensor* output_gate_bias =
      GetInput(context, node, output_gate_bias_tensor);
  TF_LITE_ENSURE_EQ(context, output_gate_bias->dims->size, 1);
  TF_LITE_ENSURE_EQ(context, output_gate_bias->dims->data[0], n_cell);
  TF_LITE_ENSURE_TYPES_EQ(context, output_gate_bias->type, kTfLiteFloat32);

  const TfLiteTensor* projection_weights =
      GetOptionalInputTensor(context, node, projection_weights_tensor);
  if (projection_weights != nullptr) {
    TF_LITE_ENSURE_EQ(context, projection_weights->dims->size, 2);
    TF_LITE_ENSURE_EQ(context, projection_weights->dims->data[0], n_output);
    TF_LITE_ENSURE_EQ(context, projection_weights->dims->data[1], n_cell);
    TF_LITE_ENSURE_TYPES_EQ(context, projection_weights->type,
                            input_to_forget_weights->type);
  }

  const TfLiteTensor* projection_bias =
      GetOptionalInputTensor(context, node, projection_bias_tensor);
  if (projection_bias != nullptr) {
    TF_LITE_ENSURE_EQ(context, projection_bias->dims->size, 1);
    TF_LITE_ENSURE_EQ(context, projection_bias->dims->data[0], n_output);
    TF_LITE_ENSURE_TYPES_EQ(context, projection_bias->type, kTfLiteFloat32);
  }

  // Making sure the projection tensors are consistent:
  // 1) If projection weight is not present, then projection bias should not be
  // present.
  // 2) If projection weight is present, then projection bias is optional.
  // TODO(ghodrat): make sure this is correct.
  const bool projecton_tensors_consistent =
      ((projection_weights != nullptr) || (projection_bias == nullptr));
  TF_LITE_ENSURE(context, projecton_tensors_consistent == true);

  return kTfLiteOk;
}