static js_Ast *assignment(js_State *J, int notin)
{
	js_Ast *a;
	INCREC();
	a = conditional(J, notin);
	if (jsP_accept(J, '=')) a = EXP2(ASS, a, assignment(J, notin));
	else if (jsP_accept(J, TK_MUL_ASS)) a = EXP2(ASS_MUL, a, assignment(J, notin));
	else if (jsP_accept(J, TK_DIV_ASS)) a = EXP2(ASS_DIV, a, assignment(J, notin));
	else if (jsP_accept(J, TK_MOD_ASS)) a = EXP2(ASS_MOD, a, assignment(J, notin));
	else if (jsP_accept(J, TK_ADD_ASS)) a = EXP2(ASS_ADD, a, assignment(J, notin));
	else if (jsP_accept(J, TK_SUB_ASS)) a = EXP2(ASS_SUB, a, assignment(J, notin));
	else if (jsP_accept(J, TK_SHL_ASS)) a = EXP2(ASS_SHL, a, assignment(J, notin));
	else if (jsP_accept(J, TK_SHR_ASS)) a = EXP2(ASS_SHR, a, assignment(J, notin));
	else if (jsP_accept(J, TK_USHR_ASS)) a = EXP2(ASS_USHR, a, assignment(J, notin));
	else if (jsP_accept(J, TK_AND_ASS)) a = EXP2(ASS_BITAND, a, assignment(J, notin));
	else if (jsP_accept(J, TK_XOR_ASS)) a = EXP2(ASS_BITXOR, a, assignment(J, notin));
	else if (jsP_accept(J, TK_OR_ASS)) a = EXP2(ASS_BITOR, a, assignment(J, notin));
	DECREC();
	return a;
}