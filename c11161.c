R_API RBinJavaElementValue *r_bin_java_element_value_new(ut8 *buffer, ut64 sz, ut64 buf_offset) {
	ut32 i = 0;
	ut64 offset = 0;
	RBinJavaElementValue *element_value = R_NEW0 (RBinJavaElementValue);
	if (!element_value) {
		return NULL;
	}
	RBinJavaElementValuePair *evps = NULL;
	element_value->metas = R_NEW0 (RBinJavaMetaInfo);
	if (!element_value->metas) {
		R_FREE (element_value);
		return NULL;
	}
	element_value->file_offset = buf_offset;
	element_value->tag = buffer[offset];
	element_value->size += 1;
	offset += 1;
	element_value->metas->type_info = (void *) r_bin_java_get_ev_meta_from_tag (element_value->tag);
	switch (element_value->tag) {
	case R_BIN_JAVA_EV_TAG_BYTE:
	case R_BIN_JAVA_EV_TAG_CHAR:
	case R_BIN_JAVA_EV_TAG_DOUBLE:
	case R_BIN_JAVA_EV_TAG_FLOAT:
	case R_BIN_JAVA_EV_TAG_INT:
	case R_BIN_JAVA_EV_TAG_LONG:
	case R_BIN_JAVA_EV_TAG_SHORT:
	case R_BIN_JAVA_EV_TAG_BOOLEAN:
	case R_BIN_JAVA_EV_TAG_STRING:
		// look up value in bin->cp_list
		// (ut16) read and set const_value.const_value_idx
		element_value->value.const_value.const_value_idx = R_BIN_JAVA_USHORT (buffer, offset);
		element_value->size += 2;
		// look-up, deep copy, and set const_value.const_value_cp_obj
		element_value->value.const_value.const_value_cp_obj = r_bin_java_clone_cp_idx (R_BIN_JAVA_GLOBAL_BIN, element_value->value.const_value.const_value_idx);
		break;
	case R_BIN_JAVA_EV_TAG_ENUM:
		// (ut16) read and set enum_const_value.type_name_idx
		element_value->value.enum_const_value.type_name_idx = R_BIN_JAVA_USHORT (buffer, offset);
		element_value->size += 2;
		offset += 2;
		// (ut16) read and set enum_const_value.const_name_idx
		element_value->value.enum_const_value.const_name_idx = R_BIN_JAVA_USHORT (buffer, offset);
		element_value->size += 2;
		offset += 2;
		// look up type_name_index in bin->cp_list
		// look-up, deep copy, and set enum_const_value.const_name_cp_obj
		element_value->value.enum_const_value.const_name_cp_obj = r_bin_java_clone_cp_idx (R_BIN_JAVA_GLOBAL_BIN, element_value->value.enum_const_value.const_name_idx);
		// look-up, deep copy, and set enum_const_value.type_name_cp_obj
		element_value->value.enum_const_value.type_name_cp_obj = r_bin_java_clone_cp_idx (R_BIN_JAVA_GLOBAL_BIN, element_value->value.enum_const_value.type_name_idx);
		break;
	case R_BIN_JAVA_EV_TAG_CLASS:
		// (ut16) read and set class_value.class_info_idx
		element_value->value.class_value.class_info_idx = R_BIN_JAVA_USHORT (buffer, offset);
		element_value->size += 2;
		offset += 2;
		// look up type_name_index in bin->cp_list
		// look-up, deep copy, and set class_value.class_info_cp_obj
		element_value->value.class_value.class_info_cp_obj = r_bin_java_clone_cp_idx (R_BIN_JAVA_GLOBAL_BIN, element_value->value.class_value.class_info_idx);
		break;
	case R_BIN_JAVA_EV_TAG_ARRAY:
		// (ut16) read and set array_value.num_values
		element_value->value.array_value.num_values = R_BIN_JAVA_USHORT (buffer, offset);
		element_value->size += 2;
		offset += 2;
		element_value->value.array_value.values = r_list_new ();
		for (i = 0; i < element_value->value.array_value.num_values; i++) {
			if (offset >= sz) {
				break;
			}
			RBinJavaElementValue *ev_element = r_bin_java_element_value_new (buffer + offset, sz - offset, buf_offset + offset);
			if (ev_element) {
				element_value->size += ev_element->size;
				offset += ev_element->size;
				// read array_value.num_values, and append to array_value.values
				r_list_append (element_value->value.array_value.values, (void *) ev_element);
			}
		}
		break;
	case R_BIN_JAVA_EV_TAG_ANNOTATION:
		// annotation new is not used here.
		// (ut16) read and set annotation_value.type_idx;
		if (offset + 8 < sz) {
			element_value->value.annotation_value.type_idx = R_BIN_JAVA_USHORT (buffer, offset);
			element_value->size += 2;
			offset += 2;
			// (ut16) read and set annotation_value.num_element_value_pairs;
			element_value->value.annotation_value.num_element_value_pairs = R_BIN_JAVA_USHORT (buffer, offset);
			element_value->size += 2;
			offset += 2;
		}
		element_value->value.annotation_value.element_value_pairs = r_list_newf (r_bin_java_element_pair_free);
		// read annotation_value.num_element_value_pairs, and append to annotation_value.element_value_pairs
		for (i = 0; i < element_value->value.annotation_value.num_element_value_pairs; i++) {
			if (offset > sz) {
				break;
			}
			evps = r_bin_java_element_pair_new (buffer + offset, sz - offset, buf_offset + offset);
			if (evps) {
				element_value->size += evps->size;
				offset += evps->size;
			}
			if (evps == NULL) {
				// TODO: eprintf error when reading element pair
			}
			r_list_append (element_value->value.annotation_value.element_value_pairs, (void *) evps);
		}
		break;
	default:
		// eprintf unable to handle tag
		break;
	}
	return element_value;
}