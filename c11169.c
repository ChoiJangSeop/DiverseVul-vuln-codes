R_API RBinJavaVerificationObj *r_bin_java_verification_info_from_type(RBinJavaObj *bin, R_BIN_JAVA_STACKMAP_TYPE type, ut32 value) {
	RBinJavaVerificationObj *se = R_NEW0 (RBinJavaVerificationObj);
	if (!se) {
		return NULL;
	}
	se->tag = type;
	if (se->tag == R_BIN_JAVA_STACKMAP_OBJECT) {
		se->info.obj_val_cp_idx = (ut16) value;
	} else if (se->tag == R_BIN_JAVA_STACKMAP_UNINIT) {
		/*if (bin->offset_sz == 4) {
		se->info.uninit_offset = value;
		} else {
		se->info.uninit_offset = (ut16) value;
		}*/
		se->info.uninit_offset = (ut16) value;
	}
	return se;
}