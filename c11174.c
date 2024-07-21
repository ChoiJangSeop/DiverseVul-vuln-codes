R_API RBinJavaAttrInfo *r_bin_java_local_variable_table_attr_new(RBinJavaObj *bin, ut8 *buffer, ut64 sz, ut64 buf_offset) {
	RBinJavaLocalVariableAttribute *lvattr;
	ut64 curpos = 0, offset = 6;
	RBinJavaAttrInfo *attr;
	ut32 i = 0;
	if (!buffer || sz < 1) {
		return NULL;
	}
	attr = r_bin_java_default_attr_new (bin, buffer, sz, buf_offset);
	if (!attr) {
		return NULL;
	}
	attr->type = R_BIN_JAVA_ATTR_TYPE_LOCAL_VARIABLE_TABLE_ATTR;
	attr->info.local_variable_table_attr.table_length = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	attr->info.local_variable_table_attr.local_variable_table =\
		r_list_newf (r_bin_java_local_variable_table_attr_entry_free);
	for (i = 0; i < attr->info.local_variable_table_attr.table_length; i++) {
		if (offset + 10 > sz) {
			break;
		}
		curpos = buf_offset + offset;
		lvattr = R_NEW0 (RBinJavaLocalVariableAttribute);
		if (!lvattr) {
			break;
		}
		lvattr->start_pc = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->length = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->name_idx = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->descriptor_idx = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->index = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->file_offset = curpos;
		lvattr->name = r_bin_java_get_utf8_from_bin_cp_list (R_BIN_JAVA_GLOBAL_BIN, lvattr->name_idx);
		lvattr->size = 10;
		if (!lvattr->name) {
			lvattr->name = strdup ("NULL");
			eprintf ("r_bin_java_local_variable_table_attr_new: Unable to find the name for %d index.\n", lvattr->name_idx);
		}
		lvattr->descriptor = r_bin_java_get_utf8_from_bin_cp_list (R_BIN_JAVA_GLOBAL_BIN, lvattr->descriptor_idx);
		if (!lvattr->descriptor) {
			lvattr->descriptor = strdup ("NULL");
			eprintf ("r_bin_java_local_variable_table_attr_new: Unable to find the descriptor for %d index.\n", lvattr->descriptor_idx);
		}
		r_list_append (attr->info.local_variable_table_attr.local_variable_table, lvattr);
	}
	attr->size = offset;
	// IFDBG r_bin_java_print_local_variable_table_attr_summary(attr);
	return attr;
}