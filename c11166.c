R_API RBinJavaAttrInfo *r_bin_java_synthetic_attr_new(RBinJavaObj *bin, ut8 *buffer, ut64 sz, ut64 buf_offset) {
	ut64 offset = 0;
	RBinJavaAttrInfo *attr = r_bin_java_default_attr_new (bin, buffer, sz, buf_offset);
	if (!attr) {
		return NULL;
	}
	offset += 6;
	attr->type = R_BIN_JAVA_ATTR_TYPE_SYNTHETIC_ATTR;
	attr->size = offset;
	return attr;
}