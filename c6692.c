mon_rwxa_t MonCapGrant::get_allowed(CephContext *cct,
				    int daemon_type,
				    EntityName name,
				    const std::string& s, const std::string& c,
				    const map<string,string>& c_args) const
{
  if (profile.length()) {
    expand_profile(daemon_type, name);
    mon_rwxa_t a;
    for (list<MonCapGrant>::const_iterator p = profile_grants.begin();
	 p != profile_grants.end(); ++p)
      a = a | p->get_allowed(cct, daemon_type, name, s, c, c_args);
    return a;
  }
  if (service.length()) {
    if (service != s)
      return 0;
    return allow;
  }
  if (command.length()) {
    if (command != c)
      return 0;
    for (map<string,StringConstraint>::const_iterator p = command_args.begin(); p != command_args.end(); ++p) {
      map<string,string>::const_iterator q = c_args.find(p->first);
      // argument must be present if a constraint exists
      if (q == c_args.end())
	return 0;
      switch (p->second.match_type) {
      case StringConstraint::MATCH_TYPE_EQUAL:
	if (p->second.value != q->second)
	  return 0;
        break;
      case StringConstraint::MATCH_TYPE_PREFIX:
	if (q->second.find(p->second.value) != 0)
	  return 0;
        break;
      case StringConstraint::MATCH_TYPE_REGEX:
        try {
	  std::regex pattern(
            p->second.value, std::regex::extended);
          if (!std::regex_match(q->second, pattern))
	    return 0;
        } catch(const std::regex_error&) {
	  return 0;
	}
        break;
      default:
        break;
      }
    }
    return MON_CAP_ALL;
  }
  return allow;
}