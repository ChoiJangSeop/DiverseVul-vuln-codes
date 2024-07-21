auto ReferenceHandle::Get(Local<Value> key_handle, MaybeLocal<Object> maybe_options) -> Local<Value> {
	return ThreePhaseTask::Run<async, GetRunner>(*isolate, *this, key_handle, maybe_options, inherit);
}