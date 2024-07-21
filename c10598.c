static void cil_reset_classpermission(struct cil_classpermission *cp)
{
	if (cp == NULL) {
		return;
	}

	cil_reset_classperms_list(cp->classperms);
}