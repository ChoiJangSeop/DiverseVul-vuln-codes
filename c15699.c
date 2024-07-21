		auto Phase3() -> Local<Value> final {
			return Boolean::New(Isolate::GetCurrent(), did_set);
		}