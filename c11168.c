R_API RBinJavaAnnotation *r_bin_java_annotation_new(ut8 *buffer, ut64 sz, ut64 buf_offset) {
	ut32 i = 0;
	RBinJavaAnnotation *annotation = NULL;
	RBinJavaElementValuePair *evps = NULL;
	ut64 offset = 0;
	annotation = R_NEW0 (RBinJavaAnnotation);
	if (!annotation) {
		return NULL;
	}
	// (ut16) read and set annotation_value.type_idx;
	annotation->type_idx = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	// (ut16) read and set annotation_value.num_element_value_pairs;
	annotation->num_element_value_pairs = R_BIN_JAVA_USHORT (buffer, offset);
	offset += 2;
	annotation->element_value_pairs = r_list_newf (r_bin_java_element_pair_free);
	// read annotation_value.num_element_value_pairs, and append to annotation_value.element_value_pairs
	for (i = 0; i < annotation->num_element_value_pairs; i++) {
		if (offset > sz) {
			break;
		}
		evps = r_bin_java_element_pair_new (buffer + offset, sz - offset, buf_offset + offset);
		if (evps) {
			offset += evps->size;
			r_list_append (annotation->element_value_pairs, (void *) evps);
		}
	}
	annotation->size = offset;
	return annotation;
}