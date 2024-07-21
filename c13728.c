inline TfLiteTensor* GetMutableInput(const TfLiteContext* context,
                                     const TfLiteNode* node, int index) {
  if (context->tensors != nullptr) {
    return &context->tensors[node->inputs->data[index]];
  } else {
    return context->GetTensor(context, node->inputs->data[index]);
  }
}