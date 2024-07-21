R_API RBinJavaAttrInfo *r_bin_java_annotation_default_attr_new(RBinJavaObj *bin, ut8 *buffer, ut64 sz, ut64 buf_offset) {
	ut64 offset = 0;
	RBinJavaAttrInfo *attr = NULL;
	attr = r_bin_java_default_attr_new (bin, buffer, sz, buf_offset);
	offset += 6;
	if (attr && sz >= offset) {
		attr->type = R_BIN_JAVA_ATTR_TYPE_ANNOTATION_DEFAULT_ATTR;
		attr->info.annotation_default_attr.default_value = r_bin_java_element_value_new (buffer + offset, sz - offset, buf_offset + offset);
		if (attr->info.annotation_default_attr.default_value) {
			offset += attr->info.annotation_default_attr.default_value->size;
		}
	}
	r_bin_java_print_annotation_default_attr_summary (attr);
	return attr;
}