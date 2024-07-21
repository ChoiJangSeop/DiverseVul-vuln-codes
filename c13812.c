GF_Err SFScript_Parse(GF_BifsDecoder *codec, SFScript *script_field, GF_BitStream *bs, GF_Node *n)
{
	GF_Err e;
	u32 i, count, nbBits;
	char *ptr;
	ScriptParser parser;
	e = GF_OK;
	if (gf_node_get_tag(n) != TAG_MPEG4_Script) return GF_NON_COMPLIANT_BITSTREAM;

	parser.codec = codec;
	parser.script = n;
	parser.bs = bs;
	parser.length = 500;
	parser.string = (char *) gf_malloc(sizeof(char)* parser.length);
	parser.string[0] = 0;
	parser.identifiers = gf_list_new();
	parser.new_line = (char *) (codec->dec_memory_mode ? "\n" : NULL);
	parser.indent = 0;

	//first parse fields

	if (gf_bs_read_int(bs, 1)) {
		//endFlag
		while (!gf_bs_read_int(bs, 1)) {
			e = ParseScriptField(&parser);
			if (e) goto exit;
		}
	} else {
		nbBits = gf_bs_read_int(bs, 4);
		count = gf_bs_read_int(bs, nbBits);
		for (i=0; i<count; i++) {
			e = ParseScriptField(&parser);
			if (e) goto exit;
		}
	}
	//reserevd
	gf_bs_read_int(bs, 1);
	//then parse
	SFS_AddString(&parser, "javascript:");
	SFS_AddString(&parser, parser.new_line);

	//hasFunction
	while (gf_bs_read_int(bs, 1)) {
		SFS_AddString(&parser, "function ");
		SFS_Identifier(&parser);
		SFS_Arguments(&parser, GF_FALSE);
		SFS_Space(&parser);
		SFS_StatementBlock(&parser, GF_TRUE);
		SFS_Line(&parser);
	}

	SFS_Line(&parser);

	if (script_field->script_text) gf_free(script_field->script_text);
	script_field->script_text = (char *) gf_strdup(parser.string);

exit:
	//clean up
	while (gf_list_count(parser.identifiers)) {
		ptr = (char *)gf_list_get(parser.identifiers, 0);
		gf_free(ptr);
		gf_list_rem(parser.identifiers, 0);
	}
	gf_list_del(parser.identifiers);
	if (parser.string) gf_free(parser.string);
	return e;
}