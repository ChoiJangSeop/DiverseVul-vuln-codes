LuacBinInfo *luac_build_info(LuaProto *proto) {
	if (!proto) {
		RZ_LOG_ERROR("Invalid luac file\n");
		return NULL;
	}

	LuacBinInfo *ret = RZ_NEW0(LuacBinInfo);
	if (!ret) {
		return NULL;
	}

	ret->entry_list = rz_list_newf((RzListFree)free_rz_addr);
	ret->symbol_list = rz_list_newf((RzListFree)rz_bin_symbol_free);
	ret->section_list = rz_list_newf((RzListFree)free_rz_section);
	ret->string_list = rz_list_newf((RzListFree)free_rz_string);

	if (!(ret->entry_list && ret->symbol_list && ret->section_list && ret->string_list)) {
		try_free_empty_list(ret->entry_list);
		try_free_empty_list(ret->symbol_list);
		try_free_empty_list(ret->section_list);
		try_free_empty_list(ret->string_list);
	}

	_luac_build_info(proto, ret);

	// add entry of main
	ut64 main_entry_offset;
	main_entry_offset = proto->code_offset + proto->code_skipped;
	luac_add_entry(ret->entry_list, main_entry_offset, RZ_BIN_ENTRY_TYPE_PROGRAM);

	return ret;
}