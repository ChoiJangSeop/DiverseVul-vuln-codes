static js_Ast *bitand(js_State *J, int notin)
{
	js_Ast *a = equality(J, notin);
	while (jsP_accept(J, '&'))
		a = EXP2(BITAND, a, equality(J, notin));
	return a;
}