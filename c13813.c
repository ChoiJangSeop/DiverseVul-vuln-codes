void SFS_Params(ScriptParser *parser)
{
	u32 val;
	if (parser->codec->LastError) return;
	val = gf_bs_read_int(parser->bs, 1);
	while (val) {
		SFS_Expression(parser);
		val = gf_bs_read_int(parser->bs, 1);
		if(val) SFS_AddString(parser, ",");
	}
}