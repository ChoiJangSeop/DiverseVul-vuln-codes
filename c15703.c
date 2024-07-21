auto ReferenceHandle::New(Local<Value> value, MaybeLocal<Object> options) -> unique_ptr<ReferenceHandle> {
	auto inherit = ReadOption<bool>(options, StringTable::Get().inheritUnsafe, false);
	return std::make_unique<ReferenceHandle>(value, inherit);
}