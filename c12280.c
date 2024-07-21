write(Protocol_* iprot, const StructInfo& structInfo, const void* object) {
  DCHECK(object);
  size_t written = iprot->writeStructBegin(structInfo.name);
  if (UNLIKELY(structInfo.unionExt != nullptr)) {
    const FieldInfo* end = structInfo.fieldInfos + structInfo.numFields;
    const auto& unionId =
        activeUnionMemberId(object, structInfo.unionExt->unionTypeOffset);
    const FieldInfo* found = std::lower_bound(
        structInfo.fieldInfos,
        end,
        unionId,
        [](const FieldInfo& lhs, FieldID rhs) { return lhs.id < rhs; });
    if (found < end && found->id == unionId) {
      const OptionalThriftValue value = getValue(*found->typeInfo, object);
      if (value.hasValue()) {
        written += writeField(iprot, *found, value.value());
      } else if (found->typeInfo->type == protocol::TType::T_STRUCT) {
        written += iprot->writeFieldBegin(
            found->name, found->typeInfo->type, found->id);
        written += iprot->writeStructBegin(found->name);
        written += iprot->writeStructEnd();
        written += iprot->writeFieldStop();
        written += iprot->writeFieldEnd();
      }
    }
  } else {
    for (std::int16_t index = 0; index < structInfo.numFields; index++) {
      const auto& fieldInfo = structInfo.fieldInfos[index];
      if (fieldInfo.isUnqualified || fieldInfo.issetOffset == 0 ||
          fieldIsSet(object, fieldInfo.issetOffset)) {
        const OptionalThriftValue value =
            getValue(*fieldInfo.typeInfo, getMember(fieldInfo, object));
        if (value.hasValue()) {
          written += writeField(iprot, fieldInfo, value.value());
        }
      }
    }
  }

  written += iprot->writeFieldStop();
  written += iprot->writeStructEnd();
  return written;
}