pg_role_aclcheck(Oid role_oid, Oid roleid, AclMode mode)
{
	if (mode & ACL_GRANT_OPTION_FOR(ACL_CREATE))
	{
		if (is_admin_of_role(roleid, role_oid))
			return ACLCHECK_OK;
	}
	if (mode & ACL_CREATE)
	{
		if (is_member_of_role(roleid, role_oid))
			return ACLCHECK_OK;
	}
	if (mode & ACL_USAGE)
	{
		if (has_privs_of_role(roleid, role_oid))
			return ACLCHECK_OK;
	}
	return ACLCHECK_NO_PRIV;
}