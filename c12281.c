void read(Protocol_* iprot, const StructInfo& structInfo, void* object) {
  DCHECK(object);
  ProtocolReaderStructReadState<Protocol_> readState;
  readState.readStructBegin(iprot);

  if (UNLIKELY(structInfo.unionExt != nullptr)) {
    readState.fieldId = 0;
    readState.readFieldBegin(iprot);
    if (readState.atStop()) {
      structInfo.unionExt->clear(object);
      readState.readStructEnd(iprot);
      return;
    }
    const auto* fieldInfo = findFieldInfo(iprot, readState, structInfo);
    // Found it.
    if (fieldInfo) {
      void* unionVal = getMember(*fieldInfo, object);
      // Default construct and placement new into the member union.
      structInfo.unionExt->initMember[fieldInfo - structInfo.fieldInfos](
          unionVal);
      read(iprot, *fieldInfo->typeInfo, readState, unionVal);
      const_cast<FieldID&>(activeUnionMemberId(
          object, structInfo.unionExt->unionTypeOffset)) = fieldInfo->id;
    } else {
      skip(iprot, readState);
    }
    readState.readFieldEnd(iprot);
    readState.readFieldBegin(iprot);
    if (UNLIKELY(!readState.atStop())) {
      TProtocolException::throwUnionMissingStop();
    }
    readState.readStructEnd(iprot);
    return;
  }

  // Define out of loop to call advanceToNextField after the loop ends.
  FieldID prevFieldId = 0;

  // The index of the expected field in the struct layout.
  std::int16_t index = 0;

  // Every time advanceToNextField reports a field mismatch, either because the
  // field is missing or if the serialized fields are not sorted (protocols
  // don't guarantee a specific field order), we search for the field info
  // matching the read bytes. Then we resume from the one past the found field
  // to reduce the number of scans we have to do if the fields are sorted which
  // is a common case. When we increment index past the number of fields we
  // utilize the same search logic with a field info of type TType::T_STOP.
  for (;; ++index) {
    auto* fieldInfo = index < structInfo.numFields
        ? &structInfo.fieldInfos[index]
        : &kStopMarker;
    // Try to match the next field in order against the current bytes.
    if (UNLIKELY(!readState.advanceToNextField(
            iprot, prevFieldId, fieldInfo->id, fieldInfo->typeInfo->type))) {
      // Loop to skip until we find a match for both field id/name and type.
      for (;;) {
        readState.afterAdvanceFailure(iprot);
        if (readState.atStop()) {
          // Already at stop, return immediately.
          readState.readStructEnd(iprot);
          return;
        }
        fieldInfo = findFieldInfo(iprot, readState, structInfo);
        // Found it.
        if (fieldInfo) {
          // Set the index to the field next in order to the found field.
          index = fieldInfo - structInfo.fieldInfos;
          break;
        }
        skip(iprot, readState);
      }
    } else if (UNLIKELY(index >= structInfo.numFields)) {
      // We are at stop and have tried all of the fields, so return.
      readState.readStructEnd(iprot);
      return;
    }
    // Id and type are what we expect, try read.
    prevFieldId = fieldInfo->id;
    read(iprot, *fieldInfo->typeInfo, readState, getMember(*fieldInfo, object));
    if (fieldInfo->issetOffset > 0) {
      const_cast<bool&>(fieldIsSet(object, fieldInfo->issetOffset)) = true;
    }
  }
}