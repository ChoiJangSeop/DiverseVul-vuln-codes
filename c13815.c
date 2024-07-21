void SFS_CompoundExpression(ScriptParser *parser)
{
	if (parser->codec->LastError) return;
	SFS_Expression(parser);
	if (! gf_bs_read_int(parser->bs, 1)) return;
	SFS_AddString(parser, ",");
	SFS_CompoundExpression(parser);
}