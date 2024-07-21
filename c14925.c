R_API RBinJavaAttrInfo *r_bin_java_code_attr_new(ut8 *buffer, ut64 sz, ut64 buf_offset) {
	RBinJavaAttrInfo *attr = NULL, *_attr = NULL;
	ut32 k = 0, curpos;
	ut64 offset = 0;
	attr = r_bin_java_default_attr_new (buffer, sz, buf_offset);
	if (!attr) {
		return NULL;
	}
	if (sz < 16 || sz > buf_offset) {// sz > buf_offset) {
		free (attr);
		return NULL;
	}
	offset += 6;
	attr->type = R_BIN_JAVA_ATTR_TYPE_CODE_ATTR;
	attr->info.code_attr.max_stack = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	attr->info.code_attr.max_locals = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	attr->info.code_attr.code_length = R_BIN_JAVA_UINT (buffer, offset);
	offset += 4;
	// BUG: possible unsigned integer overflow here
	attr->info.code_attr.code_offset = buf_offset + offset;
	attr->info.code_attr.code = (ut8 *) malloc (attr->info.code_attr.code_length);
	if (attr->info.code_attr.code == NULL) {
		eprintf ("Handling Code Attributes: Unable to allocate memory "
			"(%u bytes) for a code.\n", attr->info.code_attr.code_length);
		return attr;
	}
	R_BIN_JAVA_GLOBAL_BIN->current_code_attr = attr;
	{
		int len = attr->info.code_attr.code_length;
		memset (attr->info.code_attr.code, 0, len);
		if (offset + len >= sz) {
			len = sz;
			return attr;
		}
		memcpy (attr->info.code_attr.code, buffer + offset, len);
		offset += len;
	}
	attr->info.code_attr.exception_table_length = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	attr->info.code_attr.exception_table = r_list_newf (free);
	for (k = 0; k < attr->info.code_attr.exception_table_length; k++) {
		curpos = buf_offset + offset;
		if (curpos + 8 > sz) {
			return attr;
		}
		RBinJavaExceptionEntry *e = R_NEW0 (RBinJavaExceptionEntry);
		if (!e) {
			free (attr);
			return NULL;
		}
		e->file_offset = curpos;
		e->start_pc = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		e->end_pc = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		e->handler_pc = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		e->catch_type = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		r_list_append (attr->info.code_attr.exception_table, e);
		e->size = 8;
	}
	attr->info.code_attr.attributes_count = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	// IFDBG eprintf ("	code Attributes_count: %d\n", attr->info.code_attr.attributes_count);
	// XXX - attr->info.code_attr.attributes is not freed because one of the code attributes is improperly parsed.
	attr->info.code_attr.attributes = r_list_newf (r_bin_java_attribute_free);
	if (attr->info.code_attr.attributes_count > 0) {
		for (k = 0; k < attr->info.code_attr.attributes_count; k++) {
			int size = (offset < sz) ? sz - offset : 0;
			if (size > sz || size <= 0) {
				break;
			}
			_attr = r_bin_java_read_next_attr_from_buffer (buffer + offset, size, buf_offset + offset);
			if (!_attr) {
				eprintf ("[X] r_bin_java_code_attr_new: Error unable to parse remainder of classfile after Method's Code Attribute: %d.\n", k);
				break;
			}
			IFDBG eprintf("Parsing @ 0x%"PFMT64x " (%s) = 0x%"PFMT64x " bytes, %p\n", _attr->file_offset, _attr->name, _attr->size, _attr);
			offset += _attr->size;
			r_list_append (attr->info.code_attr.attributes, _attr);
			if (_attr->type == R_BIN_JAVA_ATTR_TYPE_LOCAL_VARIABLE_TABLE_ATTR) {
				IFDBG eprintf("Parsed the LocalVariableTable, preparing the implicit mthod frame.\n");
				// r_bin_java_print_attr_summary(_attr);
				attr->info.code_attr.implicit_frame = r_bin_java_build_stack_frame_from_local_variable_table (R_BIN_JAVA_GLOBAL_BIN, _attr);
				attr->info.code_attr.implicit_frame->file_offset = buf_offset;
				IFDBG r_bin_java_print_stack_map_frame_summary(attr->info.code_attr.implicit_frame);
				// r_list_append (attr->info.code_attr.attributes, attr->info.code_attr.implicit_frame);
			}
			// if (offset > sz) {
			// eprintf ("[X] r_bin_java: Error unable to parse remainder of classfile after Attribute: %d.\n", k);
			// break;
			// }

		}
	}
	if (attr->info.code_attr.implicit_frame == NULL) {
		// build a default implicit_frame
		attr->info.code_attr.implicit_frame = r_bin_java_default_stack_frame ();
		// r_list_append (attr->info.code_attr.attributes, attr->info.code_attr.implicit_frame);
	}
	attr->size = offset;
	return attr;
}