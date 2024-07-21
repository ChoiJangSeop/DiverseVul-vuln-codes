		SetRunner(
			ReferenceHandle& that,
			Local<Value> key_handle,
			Local<Value> val_handle,
			MaybeLocal<Object> maybe_options
		) :
				key{ExternalCopy::CopyIfPrimitive(key_handle)},
				val{TransferOut(val_handle, TransferOptions{maybe_options})},
				context{that.context},
				reference{that.reference} {
			that.CheckDisposed();
			if (!key) {
				throw RuntimeTypeError("Invalid `key`");
			}
		}