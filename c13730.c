inline TfLiteTensor* GetTemporary(TfLiteContext* context,
                                  const TfLiteNode* node, int index) {
  return &context->tensors[node->temporaries->data[index]];
}