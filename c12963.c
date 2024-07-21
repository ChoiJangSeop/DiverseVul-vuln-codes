Status PySeqToTensor(PyObject* obj, DataType dtype, Tensor* ret) {
  ConverterState state;
  TF_RETURN_IF_ERROR(InferShapeAndType(obj, &state));
  DataType requested_dtype = DT_INVALID;
  if (dtype != DT_INVALID) {
    requested_dtype = dtype;
  }
  // NOTE(josh11b): If don't successfully convert to the requested type,
  // we just try instead to create a tensor of the inferred type and
  // let the caller convert it to the requested type using a cast
  // operation.
  switch (requested_dtype) {
    case DT_FLOAT:
      if (FloatConverter::Convert(obj, &state, ret) == nullptr)
        return Status::OK();
      break;

    case DT_DOUBLE:
      if (DoubleConverter::Convert(obj, &state, ret) == nullptr)
        return Status::OK();
      break;

    case DT_HALF:
      RETURN_STRING_AS_STATUS(NumpyHalfConverter::Convert(obj, &state, ret));

    case DT_INT64:
      if (Int64Converter::Convert(obj, &state, ret) == nullptr)
        return Status::OK();
      break;

    case DT_INT32:
      if (Int32Converter::Convert(obj, &state, ret) == nullptr)
        return Status::OK();
      break;

    case DT_UINT64:
      if (UInt64Converter::Convert(obj, &state, ret) == nullptr)
        return Status::OK();
      break;

    case DT_COMPLEX128:
      if (Complex128Converter::Convert(obj, &state, ret) == nullptr)
        return Status::OK();
      break;

    case DT_STRING:
      if (StringConverter::Convert(obj, &state, ret) == nullptr)
        return Status::OK();
      break;

    case DT_BOOL:
      if (BoolConverter::Convert(obj, &state, ret) == nullptr)
        return Status::OK();
      break;

    default:
      break;
  }
  switch (state.inferred_dtype) {
    case DT_FLOAT:
      // TODO(josh11b): Handle mixed floats and complex numbers?
      if (requested_dtype == DT_INVALID) {
        // TensorFlow uses float32s to represent floating point numbers
        // by default (for space and speed over using doubles).
        RETURN_STRING_AS_STATUS(FloatConverter::Convert(obj, &state, ret));
      } else {
        // We are going to do a cast to the user's requested dtype
        // after this.  We use doubles for this intermediate result so
        // we don't lose precision that might be representable in the
        // final type.
        RETURN_STRING_AS_STATUS(DoubleConverter::Convert(obj, &state, ret));
      }

    case DT_DOUBLE:
      RETURN_STRING_AS_STATUS(DoubleConverter::Convert(obj, &state, ret));

    case DT_HALF:
      RETURN_STRING_AS_STATUS(NumpyHalfConverter::Convert(obj, &state, ret));

    case DT_INT64:
      if (requested_dtype == DT_INVALID) {
        const char* error = Int32Converter::Convert(obj, &state, ret);
        if (error == ErrorFoundInt64) {
          error = Int64Converter::Convert(obj, &state, ret);
        }
        if (error == ErrorFoundFloat) {
          error = FloatConverter::Convert(obj, &state, ret);
        }
        // TODO(josh11b): May also want to fall back to using doubles if
        // error == ErrorOutOfRange?
        RETURN_STRING_AS_STATUS(error);
      } else {
        const char* error = Int64Converter::Convert(obj, &state, ret);
        if (error == ErrorFoundFloat) {
          error = DoubleConverter::Convert(obj, &state, ret);
        }
        RETURN_STRING_AS_STATUS(error);
      }

    case DT_STRING:
      RETURN_STRING_AS_STATUS(StringConverter::Convert(obj, &state, ret));

    case DT_COMPLEX128:
      RETURN_STRING_AS_STATUS(Complex128Converter::Convert(obj, &state, ret));

    case DT_BOOL:
      RETURN_STRING_AS_STATUS(BoolConverter::Convert(obj, &state, ret));

    case DT_INVALID:  // Only occurs for empty tensors.
      *ret = Tensor(requested_dtype == DT_INVALID ? DT_FLOAT : requested_dtype,
                    state.inferred_shape);
      return Status::OK();

    default:
      return errors::Unimplemented("Missing Python -> Tensor conversion for ",
                                   DataTypeString(state.inferred_dtype));
  }

  return Status::OK();
}