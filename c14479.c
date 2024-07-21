TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLitePackParams* data =
      reinterpret_cast<TfLitePackParams*>(node->builtin_data);

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  switch (output->type) {
    case kTfLiteFloat32: {
      return PackImpl<float>(context, node, output, data->values_count,
                             data->axis);
    }
    case kTfLiteUInt8: {
      return PackImpl<uint8_t>(context, node, output, data->values_count,
                               data->axis);
    }
    case kTfLiteInt8: {
      return PackImpl<int8_t>(context, node, output, data->values_count,
                              data->axis);
    }
    case kTfLiteInt16: {
      return PackImpl<int16_t>(context, node, output, data->values_count,
                               data->axis);
    }
    case kTfLiteInt32: {
      return PackImpl<int32_t>(context, node, output, data->values_count,
                               data->axis);
    }
    case kTfLiteInt64: {
      return PackImpl<int64_t>(context, node, output, data->values_count,
                               data->axis);
    }
    default: {
      context->ReportError(context, "Type '%s' is not supported by pack.",
                           TfLiteTypeGetName(output->type));
      return kTfLiteError;
    }
  }

  return kTfLiteOk;
}