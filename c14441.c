TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteUnpackParams* data =
      reinterpret_cast<TfLiteUnpackParams*>(node->builtin_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  switch (input->type) {
    case kTfLiteFloat32: {
      UnpackImpl<float>(context, node, input, data->num, data->axis);
      break;
    }
    case kTfLiteInt32: {
      UnpackImpl<int32_t>(context, node, input, data->num, data->axis);
      break;
    }
    case kTfLiteUInt8: {
      UnpackImpl<uint8_t>(context, node, input, data->num, data->axis);
      break;
    }
    case kTfLiteInt8: {
      UnpackImpl<int8_t>(context, node, input, data->num, data->axis);
      break;
    }
    case kTfLiteBool: {
      UnpackImpl<bool>(context, node, input, data->num, data->axis);
      break;
    }
    case kTfLiteInt16: {
      UnpackImpl<int16_t>(context, node, input, data->num, data->axis);
      break;
    }
    default: {
      context->ReportError(context, "Type '%s' is not supported by unpack.",
                           TfLiteTypeGetName(input->type));
      return kTfLiteError;
    }
  }

  return kTfLiteOk;
}