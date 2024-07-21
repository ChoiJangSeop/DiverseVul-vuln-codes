		void Phase2() final {
			Local<Context> context_handle = Deref(context);
			Context::Scope context_scope{context_handle};
			Local<Value> key_inner = key->CopyInto();
			Local<Object> object = Local<Object>::Cast(Deref(reference));
			// Delete key before transferring in, potentially freeing up some v8 heap
			Unmaybe(object->Delete(context_handle, key_inner));
			Local<Value> val_inner = val->TransferIn();
			did_set = Unmaybe(object->Set(context_handle, key_inner, val_inner));
		}