pk_transaction_role_to_action_allow_untrusted (PkRoleEnum role)
{
	const gchar *policy = NULL;

	switch (role) {
	case PK_ROLE_ENUM_INSTALL_PACKAGES:
	case PK_ROLE_ENUM_INSTALL_FILES:
	case PK_ROLE_ENUM_UPDATE_PACKAGES:
		policy = "org.freedesktop.packagekit.package-install-untrusted";
		break;
	default:
		policy = pk_transaction_role_to_action_only_trusted (role);
	}
	return policy;
}