static Status ValidateSavedTensors(const GraphDef& graph_def) {
  for (const auto& node : graph_def.node()) {
    const auto node_iterator = node.attr().find("value");
    if (node_iterator != node.attr().end()) {
      AttrValue node_value = node_iterator->second;
      if (node_value.has_tensor()) {
        const PartialTensorShape node_shape(node_value.tensor().tensor_shape());
        if (node_shape.num_elements() < 0) {
          return errors::FailedPrecondition(
              "Saved model contains node \"", node.name(), "\" (op \"",
              node.op(), "\") which initializes from a tensor with ",
              node_shape.num_elements(), " elements");
        }
      }
    } else if (node.op() == "Const") {
      return errors::FailedPrecondition(
          "Saved model contains node \"", node.name(),
          "\" which is a constant tensor but no value has been provided");
    }
  }
  return Status::OK();
}