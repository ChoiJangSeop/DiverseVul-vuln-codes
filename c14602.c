TfLiteStatus SelectPrepare(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 3);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input_condition =
      GetInput(context, node, kInputTensorCondition);
  const TfLiteTensor* input_x = GetInput(context, node, kInputTensorX);
  const TfLiteTensor* input_y = GetInput(context, node, kInputTensorY);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  // Input must be bool.
  TF_LITE_ENSURE_TYPES_EQ(context, input_condition->type, kTfLiteBool);
  TF_LITE_ENSURE_TYPES_EQ(context, input_x->type, input_y->type);
  output->type = input_x->type;

  bool same_shape = HaveSameShapes(input_condition, input_x) &&
                    HaveSameShapes(input_x, input_y);
  TfLiteIntArray* output_size;
  if (!same_shape) {
    switch (kernel_type) {
      case kVersionOne: {
        bool is_input_condition_scalar = NumDimensions(input_condition) == 0;
        bool has_rank_one_input_condition =
            NumDimensions(input_condition) == 1 &&
            SizeOfDimension(input_condition, 0) == SizeOfDimension(input_x, 0);
        data->has_low_rank_input_condition =
            is_input_condition_scalar || has_rank_one_input_condition;
        TF_LITE_ENSURE(context, data->has_low_rank_input_condition);

        output_size = TfLiteIntArrayCopy(input_x->dims);

        // Input tensors must have the same type and size
        TF_LITE_ENSURE(context, HaveSameShapes(input_x, input_y));
        break;
      }
      case kVersionTwo: {
        TF_LITE_ENSURE_OK(context, CalculateShapeForBroadcast(
                                       context, input_condition, input_x,
                                       input_y, &output_size));
        data->requires_broadcast = true;
        break;
      }
      default:
        return kTfLiteError;
    }
  } else {
    output_size = TfLiteIntArrayCopy(input_x->dims);
  }

  return context->ResizeTensor(context, output, output_size);
}