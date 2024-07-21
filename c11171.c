R_API RBinJavaAttrInfo *r_bin_java_enclosing_methods_attr_new(RBinJavaObj *bin, ut8 *buffer, ut64 sz, ut64 buf_offset) {
	ut64 offset = 6;
	RBinJavaAttrInfo *attr = r_bin_java_default_attr_new (bin, buffer, sz, buf_offset);
	if (!attr || sz < 10) {
		free (attr);
		return NULL;
	}
	attr->type = R_BIN_JAVA_ATTR_TYPE_ENCLOSING_METHOD_ATTR;
	attr->info.enclosing_method_attr.class_idx = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	attr->info.enclosing_method_attr.method_idx = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	attr->info.enclosing_method_attr.class_name = r_bin_java_get_name_from_bin_cp_list (R_BIN_JAVA_GLOBAL_BIN, attr->info.enclosing_method_attr.class_idx);
	if (attr->info.enclosing_method_attr.class_name == NULL) {
		eprintf ("Could not resolve enclosing class name for the enclosed method.\n");
	}
	attr->info.enclosing_method_attr.method_name = r_bin_java_get_name_from_bin_cp_list (R_BIN_JAVA_GLOBAL_BIN, attr->info.enclosing_method_attr.method_idx);
	if (attr->info.enclosing_method_attr.class_name == NULL) {
		eprintf ("Could not resolve method descriptor for the enclosed method.\n");
	}
	attr->info.enclosing_method_attr.method_descriptor = r_bin_java_get_desc_from_bin_cp_list (R_BIN_JAVA_GLOBAL_BIN, attr->info.enclosing_method_attr.method_idx);
	if (attr->info.enclosing_method_attr.method_name == NULL) {
		eprintf ("Could not resolve method name for the enclosed method.\n");
	}
	attr->size = offset;
	return attr;
}