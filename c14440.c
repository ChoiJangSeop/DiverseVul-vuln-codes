inline bool ShapeIsVector(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* shape = GetInput(context, node, kShapeTensor);
  return (shape->dims->size == 1 && shape->type == kTfLiteInt32);
}