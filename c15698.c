		DeleteRunner(ReferenceHandle& that, Local<Value> key_handle) :
				key{ExternalCopy::CopyIfPrimitive(key_handle)},
				context{that.context},
				reference{that.reference} {
			that.CheckDisposed();
			if (!key) {
				throw RuntimeTypeError("Invalid `key`");
			}
		}