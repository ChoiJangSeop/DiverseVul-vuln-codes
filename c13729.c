TfLiteTensor* GetOutput(TfLiteContext* context, const TfLiteNode* node,
                        int index) {
  if (context->tensors != nullptr) {
    return &context->tensors[node->outputs->data[index]];
  } else {
    return context->GetTensor(context, node->outputs->data[index]);
  }
}