		GetRunner(
			const ReferenceHandle& that,
			Local<Value> key_handle,
			MaybeLocal<Object> maybe_options,
			bool inherit
		) :
				context{that.context},
				reference{that.reference},
				options{maybe_options, inherit ?
					TransferOptions::Type::DeepReference : TransferOptions::Type::Reference},
				inherit{inherit} {
			that.CheckDisposed();
			key = ExternalCopy::CopyIfPrimitive(key_handle);
			if (!key) {
				throw RuntimeTypeError("Invalid `key`");
			}
		}