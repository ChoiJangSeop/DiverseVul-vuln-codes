TfLiteStatus DecodeCenterSizeBoxes(TfLiteContext* context, TfLiteNode* node,
                                   OpData* op_data) {
  // Parse input tensor boxencodings
  const TfLiteTensor* input_box_encodings =
      GetInput(context, node, kInputTensorBoxEncodings);
  TF_LITE_ENSURE_EQ(context, input_box_encodings->dims->data[0], kBatchSize);
  const int num_boxes = input_box_encodings->dims->data[1];
  TF_LITE_ENSURE(context, input_box_encodings->dims->data[2] >= kNumCoordBox);
  const TfLiteTensor* input_anchors =
      GetInput(context, node, kInputTensorAnchors);

  // Decode the boxes to get (ymin, xmin, ymax, xmax) based on the anchors
  CenterSizeEncoding box_centersize;
  CenterSizeEncoding scale_values = op_data->scale_values;
  CenterSizeEncoding anchor;
  for (int idx = 0; idx < num_boxes; ++idx) {
    switch (input_box_encodings->type) {
        // Quantized
      case kTfLiteUInt8:
        DequantizeBoxEncodings(
            input_box_encodings, idx,
            static_cast<float>(input_box_encodings->params.zero_point),
            static_cast<float>(input_box_encodings->params.scale),
            input_box_encodings->dims->data[2], &box_centersize);
        DequantizeBoxEncodings(
            input_anchors, idx,
            static_cast<float>(input_anchors->params.zero_point),
            static_cast<float>(input_anchors->params.scale), kNumCoordBox,
            &anchor);
        break;
        // Float
      case kTfLiteFloat32: {
        // Please see DequantizeBoxEncodings function for the support detail.
        const int box_encoding_idx = idx * input_box_encodings->dims->data[2];
        const float* boxes =
            &(GetTensorData<float>(input_box_encodings)[box_encoding_idx]);
        box_centersize = *reinterpret_cast<const CenterSizeEncoding*>(boxes);
        anchor =
            ReInterpretTensor<const CenterSizeEncoding*>(input_anchors)[idx];
        break;
      }
      default:
        // Unsupported type.
        return kTfLiteError;
    }

    float ycenter = box_centersize.y / scale_values.y * anchor.h + anchor.y;
    float xcenter = box_centersize.x / scale_values.x * anchor.w + anchor.x;
    float half_h =
        0.5f * static_cast<float>(std::exp(box_centersize.h / scale_values.h)) *
        anchor.h;
    float half_w =
        0.5f * static_cast<float>(std::exp(box_centersize.w / scale_values.w)) *
        anchor.w;
    TfLiteTensor* decoded_boxes =
        &context->tensors[op_data->decoded_boxes_index];
    auto& box = ReInterpretTensor<BoxCornerEncoding*>(decoded_boxes)[idx];
    box.ymin = ycenter - half_h;
    box.xmin = xcenter - half_w;
    box.ymax = ycenter + half_h;
    box.xmax = xcenter + half_w;
  }
  return kTfLiteOk;
}