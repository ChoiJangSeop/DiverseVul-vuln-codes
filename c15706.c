		void Phase2() final {
			Local<Context> context_handle = Deref(context);
			Context::Scope context_scope{context_handle};
			Local<Value> key_inner = key->CopyInto();
			Local<Object> object = Local<Object>::Cast(Deref(reference));
			bool allow = [&]() {
				if (!inherit) {
					if (key_inner->IsName()) {
						return Unmaybe(object->HasRealNamedProperty(context_handle, key_inner.As<Name>()));
					} else if (key_inner->IsNumber()) {
						return Unmaybe(object->HasRealIndexedProperty(context_handle, HandleCast<uint32_t>(key_inner)));
					} else {
						return false;
					}
				}
				return true;
			}();
			Local<Value> value = allow ?
				Unmaybe(object->Get(context_handle, key_inner)) :
				Undefined(Isolate::GetCurrent()).As<Value>();
			ret = TransferOut(value, options);
		}