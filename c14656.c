void fx_DataView(txMachine* the)
{
	txSlot* slot;
	txBoolean flag = 0;
	txInteger offset, size;
	txSlot* info;
	txSlot* instance;
	txSlot* view;
	txSlot* buffer;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: DataView");
	if ((mxArgc > 0) && (mxArgv(0)->kind == XS_REFERENCE_KIND)) {
		slot = mxArgv(0)->value.reference->next;
		if (slot && ((slot->kind == XS_ARRAY_BUFFER_KIND) || (slot->kind == XS_HOST_KIND))) {
			flag = 1;
		}
	}
	if (!flag)
		mxTypeError("buffer is no ArrayBuffer instance");
		
	offset = fxArgToByteLength(the, 1, 0);
	info = fxGetBufferInfo(the, mxArgv(0));
	if (info->value.bufferInfo.length < offset)
		mxRangeError("out of range byteOffset %ld", offset);
	size = fxArgToByteLength(the, 2, -1);
	if (size >= 0) {
		if (info->value.bufferInfo.length < (offset + size))
			mxRangeError("out of range byteLength %ld", size);
	}
	else {
		if (info->value.bufferInfo.maxLength < 0)
			size = info->value.bufferInfo.length - offset;
	}
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxDataViewPrototype);
	instance = fxNewDataViewInstance(the);
	mxPullSlot(mxResult);
	view = instance->next;
	buffer = view->next;
	buffer->kind = XS_REFERENCE_KIND;
	buffer->value.reference = mxArgv(0)->value.reference;
	info = fxGetBufferInfo(the, buffer);
	if (info->value.bufferInfo.maxLength >= 0) {
		if (info->value.bufferInfo.length < offset)
			mxRangeError("out of range byteOffset %ld", offset);
		else if (size >= 0) {
			if (info->value.bufferInfo.length < (offset + size))
				mxRangeError("out of range byteLength %ld", size);
		}
	}
	view->value.dataView.offset = offset;
	view->value.dataView.size = size;
}