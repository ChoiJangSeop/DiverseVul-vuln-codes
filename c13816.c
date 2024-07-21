void SFS_Expression(ScriptParser *parser)
{
	u32 val = gf_bs_read_int(parser->bs, NUMBITS_EXPR_TYPE);
	if (parser->codec->LastError) return;

	switch(val) {
	case ET_CURVED_EXPR:
		SFS_AddString(parser, "(");
		SFS_CompoundExpression(parser);
		SFS_AddString(parser, ")");
		break;
	case ET_NEGATIVE:
		SFS_AddString(parser, "-");
		SFS_Expression(parser);
		break;
	case ET_NOT:
		SFS_AddString(parser, "!");
		SFS_Expression(parser);
		break;
	case ET_ONESCOMP:
		SFS_AddString(parser, "~");
		SFS_Expression(parser);
		break;
	case ET_INCREMENT:
		SFS_AddString(parser, "++");
		SFS_Expression(parser);
		break;
	case ET_DECREMENT:
		SFS_AddString(parser, "--");
		SFS_Expression(parser);
		break;
	case ET_POST_INCREMENT:
		SFS_Expression(parser);
		SFS_AddString(parser, "++");
		break;
	case ET_POST_DECREMENT:
		SFS_Expression(parser);
		SFS_AddString(parser, "--");
		break;
	case ET_CONDTEST:
		SFS_Expression(parser);
		SFS_AddString(parser, " ? ");
		SFS_Expression(parser);
		SFS_AddString(parser, " : ");
		SFS_Expression(parser);
		break;
	case ET_STRING:
		SFS_AddString(parser, "'");
		SFS_GetString(parser);
		SFS_AddString(parser, "'");
		break;
	case ET_NUMBER:
		SFS_GetNumber(parser);
		break;
	case ET_IDENTIFIER:
		SFS_Identifier(parser);
		break;
	case ET_FUNCTION_CALL:
		SFS_FunctionCall(parser);
		break;
	case ET_NEW:
		SFS_NewObject(parser);
		break;
	case ET_OBJECT_MEMBER_ACCESS:
		SFS_ObjectMemberAccess(parser);
		break;
	case ET_OBJECT_METHOD_CALL:
		SFS_ObjectMethodCall(parser);
		break;
	case ET_ARRAY_DEREFERENCE:
		SFS_ArrayDeref(parser);
		break;

	case ET_MULTIPLY:
		SFS_Expression(parser);
		SFS_AddString(parser, "*");
		SFS_Expression(parser);
		break;
	case ET_DIVIDE:
		SFS_Expression(parser);
		SFS_AddString(parser, "/");
		SFS_Expression(parser);
		break;
	case ET_MOD:
		SFS_Expression(parser);
		SFS_AddString(parser, "%");
		SFS_Expression(parser);
		break;
	case ET_PLUS:
		SFS_Expression(parser);
		SFS_AddString(parser, "+");
		SFS_Expression(parser);
		break;
	case ET_MINUS:
		SFS_Expression(parser);
		SFS_AddString(parser, "-");
		SFS_Expression(parser);
		break;
	case ET_LSHIFT:
		SFS_Expression(parser);
		SFS_AddString(parser, "<<");
		SFS_Expression(parser);
		break;
	case ET_RSHIFT:
		SFS_Expression(parser);
		SFS_AddString(parser, ">>");
		SFS_Expression(parser);
		break;
	case ET_RSHIFTFILL:
		SFS_Expression(parser);
		SFS_AddString(parser, ">>>");
		SFS_Expression(parser);
		break;
	case ET_AND:
		SFS_Expression(parser);
		SFS_AddString(parser, "&");
		SFS_Expression(parser);
		break;
	case ET_XOR:
		SFS_Expression(parser);
		SFS_AddString(parser, "^");
		SFS_Expression(parser);
		break;
	case ET_OR:
		SFS_Expression(parser);
		SFS_AddString(parser, "|");
		SFS_Expression(parser);
		break;
	case ET_LT:
		SFS_Expression(parser);
		SFS_AddString(parser, "<");
		SFS_Expression(parser);
		break;
	case ET_LE:
		SFS_Expression(parser);
		SFS_AddString(parser, "<=");
		SFS_Expression(parser);
		break;
	case ET_GT:
		SFS_Expression(parser);
		SFS_AddString(parser, ">");
		SFS_Expression(parser);
		break;
	case ET_GE:
		SFS_Expression(parser);
		SFS_AddString(parser, ">=");
		SFS_Expression(parser);
		break;
	case ET_EQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "==");
		SFS_Expression(parser);
		break;
	case ET_NE:
		SFS_Expression(parser);
		SFS_AddString(parser, "!=");
		SFS_Expression(parser);
		break;
	case ET_LAND:
		SFS_Expression(parser);
		SFS_AddString(parser, "&&");
		SFS_Expression(parser);
		break;
	case ET_LOR:
		SFS_Expression(parser);
		SFS_AddString(parser, "||");
		SFS_Expression(parser);
		break;
	case ET_ASSIGN:
		SFS_Expression(parser);
		SFS_AddString(parser, "=");
		SFS_Expression(parser);
		break;
	case ET_PLUSEQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "+=");
		SFS_Expression(parser);
		break;
	case ET_MINUSEQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "-=");
		SFS_Expression(parser);
		break;
	case ET_MULTIPLYEQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "*=");
		SFS_Expression(parser);
		break;
	case ET_DIVIDEEQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "/=");
		SFS_Expression(parser);
		break;
	case ET_MODEQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "%=");
		SFS_Expression(parser);
		break;
	case ET_LSHIFTEQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "<<=");
		SFS_Expression(parser);
		break;
	case ET_RSHIFTEQ:
		SFS_Expression(parser);
		SFS_AddString(parser, ">>=");
		SFS_Expression(parser);
		break;
	case ET_RSHIFTFILLEQ:
		SFS_Expression(parser);
		SFS_AddString(parser, ">>>=");
		SFS_Expression(parser);
		break;
	case ET_ANDEQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "&=");
		SFS_Expression(parser);
		break;
	case ET_XOREQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "^=");
		SFS_Expression(parser);
		break;
	case ET_OREQ:
		SFS_Expression(parser);
		SFS_AddString(parser, "|=");
		SFS_Expression(parser);
		break;
	case ET_BOOLEAN:
		SFS_GetBoolean(parser);
		break;
	case ET_VAR:
		SFS_AddString(parser, "var ");
		SFS_Arguments(parser, GF_TRUE);
		break;
	case ET_FUNCTION_ASSIGN:
		SFS_AddString(parser, "function ");
		SFS_Arguments(parser, GF_FALSE);
		SFS_StatementBlock(parser, GF_TRUE);
		break;
	default:
		parser->codec->LastError = GF_NON_COMPLIANT_BITSTREAM;
		break;
	}
}