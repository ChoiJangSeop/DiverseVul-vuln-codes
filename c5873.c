static js_Ast *logand(js_State *J, int notin)
{
	js_Ast *a = bitor(J, notin);
	if (jsP_accept(J, TK_AND))
		a = EXP2(LOGAND, a, logand(J, notin));
	return a;
}