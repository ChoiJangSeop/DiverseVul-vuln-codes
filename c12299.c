transientObjectPutErrorMessage(Runtime *runtime, Handle<> base, SymbolID id) {
  // Emit an error message that looks like:
  // "Cannot create property '%{id}' on ${typeof base} '${String(base)}'".
  StringView propName =
      runtime->getIdentifierTable().getStringView(runtime, id);
  Handle<StringPrimitive> baseType =
      runtime->makeHandle(vmcast<StringPrimitive>(typeOf(runtime, base)));
  StringView baseTypeAsString =
      StringPrimitive::createStringView(runtime, baseType);
  MutableHandle<StringPrimitive> valueAsString{runtime};
  if (base->isSymbol()) {
    // Special workaround for Symbol which can't be stringified.
    auto str = symbolDescriptiveString(runtime, Handle<SymbolID>::vmcast(base));
    if (str != ExecutionStatus::EXCEPTION) {
      valueAsString = *str;
    } else {
      runtime->clearThrownValue();
      valueAsString = StringPrimitive::createNoThrow(
          runtime, "<<Exception occurred getting the value>>");
    }
  } else {
    auto str = toString_RJS(runtime, base);
    assert(
        str != ExecutionStatus::EXCEPTION &&
        "Primitives should be convertible to string without exceptions");
    valueAsString = std::move(*str);
  }
  StringView valueAsStringPrintable =
      StringPrimitive::createStringView(runtime, valueAsString);

  SmallU16String<32> tmp;
  return runtime->raiseTypeError(
      TwineChar16("Cannot create property '") + propName + "' on " +
      baseTypeAsString.getUTF16Ref(tmp) + " '" +
      valueAsStringPrintable.getUTF16Ref(tmp) + "'");
}