R_API RBinJavaAttrInfo *r_bin_java_local_variable_type_table_attr_new(ut8 *buffer, ut64 sz, ut64 buf_offset) {
	RBinJavaLocalVariableTypeAttribute *lvattr;
	ut64 offset = 6;
	ut32 i = 0;
	RBinJavaAttrInfo *attr = r_bin_java_default_attr_new (buffer, sz, 0);
	if (!attr) {
		return NULL;
	}
	attr->type = R_BIN_JAVA_ATTR_TYPE_LOCAL_VARIABLE_TYPE_TABLE_ATTR;
	attr->info.local_variable_type_table_attr.table_length = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	attr->info.local_variable_type_table_attr.local_variable_table = r_list_newf (r_bin_java_local_variable_type_table_attr_entry_free);
	for (i = 0; i < attr->info.local_variable_type_table_attr.table_length; i++) {
		ut64 curpos = buf_offset + offset;
		lvattr = R_NEW0 (RBinJavaLocalVariableTypeAttribute);
		if (!lvattr) {
			perror ("calloc");
			break;
		}
		lvattr->start_pc = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->length = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->name_idx = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->signature_idx = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->index = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		lvattr->file_offset = curpos;
		lvattr->name = r_bin_java_get_utf8_from_bin_cp_list (R_BIN_JAVA_GLOBAL_BIN, lvattr->name_idx);
		lvattr->size = 10;
		if (!lvattr->name) {
			lvattr->name = strdup ("NULL");
			eprintf ("r_bin_java_local_variable_type_table_attr_new: Unable to find the name for %d index.\n", lvattr->name_idx);
		}
		lvattr->signature = r_bin_java_get_utf8_from_bin_cp_list (R_BIN_JAVA_GLOBAL_BIN, lvattr->signature_idx);
		if (!lvattr->signature) {
			lvattr->signature = strdup ("NULL");
			eprintf ("r_bin_java_local_variable_type_table_attr_new: Unable to find the descriptor for %d index.\n", lvattr->signature_idx);
		}
		r_list_append (attr->info.local_variable_type_table_attr.local_variable_table, lvattr);
	}
	// IFDBG r_bin_java_print_local_variable_type_table_attr_summary(attr);
	attr->size = offset;
	return attr;
}