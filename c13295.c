static void labeljumps(JF, js_JumpList *jump, int baddr, int caddr)
{
	while (jump) {
		if (jump->type == STM_BREAK)
			labelto(J, F, jump->inst, baddr);
		if (jump->type == STM_CONTINUE)
			labelto(J, F, jump->inst, caddr);
		jump = jump->next;
	}
}