R_API RBinJavaVerificationObj *r_bin_java_read_from_buffer_verification_info_new(ut8 *buffer, ut64 sz, ut64 buf_offset) {
	ut64 offset = 0;
	RBinJavaVerificationObj *se = R_NEW0 (RBinJavaVerificationObj);
	if (!se) {
		return NULL;
	}
	se->file_offset = buf_offset;
	se->tag = buffer[offset];
	offset += 1;
	if (se->tag == R_BIN_JAVA_STACKMAP_OBJECT) {
		se->info.obj_val_cp_idx = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
	} else if (se->tag == R_BIN_JAVA_STACKMAP_UNINIT) {
		se->info.uninit_offset = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
	}
	if (R_BIN_JAVA_STACKMAP_UNINIT < se->tag) {
		r_bin_java_verification_info_free (se);
		return NULL;
	}
	se->size = offset;
	return se;
}