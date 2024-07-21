static js_Ast *logor(js_State *J, int notin)
{
	js_Ast *a = logand(J, notin);
	if (jsP_accept(J, TK_OR))
		a = EXP2(LOGOR, a, logor(J, notin));
	return a;
}