void CarbonProtocolReader::skip(const FieldType ft) {
  switch (ft) {
    case FieldType::True:
    case FieldType::False: {
      break;
    }
    case FieldType::Int8: {
      readRaw<int8_t>();
      break;
    }
    case FieldType::Int16: {
      readRaw<int16_t>();
      break;
    }
    case FieldType::Int32: {
      readRaw<int32_t>();
      break;
    }
    case FieldType::Int64: {
      readRaw<int64_t>();
      break;
    }
    case FieldType::Double: {
      readRaw<double>();
      break;
    }
    case FieldType::Float: {
      readRaw<float>();
      break;
    }
    case FieldType::Binary: {
      readRaw<std::string>();
      break;
    }
    case FieldType::List: {
      skipLinearContainer();
      break;
    }
    case FieldType::Struct: {
      readStructBegin();
      while (true) {
        const auto fieldType = readFieldHeader().first;
        if (fieldType == FieldType::Stop) {
          break;
        }
        skip(fieldType);
      }
      readStructEnd();
      break;
    }
    case FieldType::Set: {
      skipLinearContainer();
      break;
    }
    case FieldType::Map: {
      skipKVContainer();
      break;
    }
    default: { break; }
  }
}