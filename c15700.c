		void Phase2() final {
			Local<Context> context_handle = Deref(context);
			Context::Scope context_scope{context_handle};
			Local<Object> object = Local<Object>::Cast(Deref(reference));
			result = Unmaybe(object->Delete(context_handle, key->CopyInto()));
		}