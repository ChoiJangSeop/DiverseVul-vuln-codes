static char *stibp_state(void)
{
	if (spectre_v2_enabled == SPECTRE_V2_IBRS_ENHANCED)
		return "";

	switch (spectre_v2_user) {
	case SPECTRE_V2_USER_NONE:
		return ", STIBP: disabled";
	case SPECTRE_V2_USER_STRICT:
		return ", STIBP: forced";
	case SPECTRE_V2_USER_STRICT_PREFERRED:
		return ", STIBP: always-on";
	case SPECTRE_V2_USER_PRCTL:
	case SPECTRE_V2_USER_SECCOMP:
		if (static_key_enabled(&switch_to_cond_stibp))
			return ", STIBP: conditional";
	}
	return "";
}