void SFS_ObjectMethodCall(ScriptParser *parser)
{
	if (parser->codec->LastError) return;
	SFS_Expression(parser);
	SFS_AddString(parser, ".");
	SFS_Identifier(parser);
	SFS_AddString(parser, "(");
	SFS_Params(parser);
	SFS_AddString(parser, ")");
}