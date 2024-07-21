inline TfLiteIntArray* GetOutputShapeFromTensor(TfLiteContext* context,
                                                TfLiteNode* node) {
  const TfLiteTensor* shape = GetInput(context, node, kShapeTensor);

  TfLiteIntArray* output_shape = TfLiteIntArrayCreate(shape->dims->data[0]);
  for (int i = 0; i < output_shape->size; ++i) {
    output_shape->data[i] = shape->data.i32[i];
  }

  return output_shape;
}