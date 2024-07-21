Status QuantizeV2Shape(InferenceContext* c) {
  int axis = -1;
  Status s = c->GetAttr("axis", &axis);
  if (!s.ok() && s.code() != error::NOT_FOUND) {
    return s;
  }
  const int minmax_rank = (axis == -1) ? 0 : 1;
  TF_RETURN_IF_ERROR(shape_inference::UnchangedShape(c));
  ShapeHandle minmax;
  TF_RETURN_IF_ERROR(c->WithRank(c->input(1), minmax_rank, &minmax));
  TF_RETURN_IF_ERROR(c->WithRank(c->input(2), minmax_rank, &minmax));
  if (axis != -1) {
    ShapeHandle input;
    TF_RETURN_IF_ERROR(c->WithRankAtLeast(c->input(0), axis + 1, &input));
    DimensionHandle depth;
    TF_RETURN_IF_ERROR(
        c->Merge(c->Dim(minmax, 0), c->Dim(input, axis), &depth));
  }
  c->set_output(1, minmax);
  c->set_output(2, minmax);
  return Status::OK();
}