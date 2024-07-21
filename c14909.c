static void SFS_AddString(ScriptParser *parser, char *str)
{
	char *new_str;
	if (!str) return;
	if (strlen(parser->string) + strlen(str) >= parser->length) {
		parser->length += PARSER_STEP_ALLOC;
		new_str = (char *)gf_malloc(sizeof(char)*parser->length);
		strcpy(new_str, parser->string);
		gf_free(parser->string);
		parser->string = new_str;
	}
	strcat(parser->string, str);
}