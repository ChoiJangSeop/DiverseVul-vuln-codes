pk_transaction_role_to_action_only_trusted (PkRoleEnum role)
{
	const gchar *policy = NULL;

	switch (role) {
	case PK_ROLE_ENUM_UPDATE_PACKAGES:
		policy = "org.freedesktop.packagekit.system-update";
		break;
	case PK_ROLE_ENUM_INSTALL_SIGNATURE:
		policy = "org.freedesktop.packagekit.system-trust-signing-key";
		break;
	case PK_ROLE_ENUM_REPO_ENABLE:
	case PK_ROLE_ENUM_REPO_SET_DATA:
	case PK_ROLE_ENUM_REPO_REMOVE:
		policy = "org.freedesktop.packagekit.system-sources-configure";
		break;
	case PK_ROLE_ENUM_REFRESH_CACHE:
		policy = "org.freedesktop.packagekit.system-sources-refresh";
		break;
	case PK_ROLE_ENUM_REMOVE_PACKAGES:
		policy = "org.freedesktop.packagekit.package-remove";
		break;
	case PK_ROLE_ENUM_INSTALL_PACKAGES:
		policy = "org.freedesktop.packagekit.package-install";
		break;
	case PK_ROLE_ENUM_INSTALL_FILES:
		policy = "org.freedesktop.packagekit.package-install";
		break;
	case PK_ROLE_ENUM_ACCEPT_EULA:
		policy = "org.freedesktop.packagekit.package-eula-accept";
		break;
	case PK_ROLE_ENUM_CANCEL:
		policy = "org.freedesktop.packagekit.cancel-foreign";
		break;
	case PK_ROLE_ENUM_REPAIR_SYSTEM:
		policy = "org.freedesktop.packagekit.repair-system";
		break;
	default:
		break;
	}
	return policy;
}