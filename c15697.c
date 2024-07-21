		auto Phase3() -> Local<Value> final {
			return Boolean::New(Isolate::GetCurrent(), result);
		}