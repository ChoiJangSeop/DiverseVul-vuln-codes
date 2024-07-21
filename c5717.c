static optional<Principal> parse_principal(CephContext* cct, TokenID t,
				    string&& s) {
  // Wildcard!
  if ((t == TokenID::AWS) && (s == "*")) {
    return Principal::wildcard();

    // Do nothing for now.
  } else if (t == TokenID::CanonicalUser) {

    // AWS ARNs
  } else if (t == TokenID::AWS) {
    auto a = ARN::parse(s);
    if (!a) {
      if (std::none_of(s.begin(), s.end(),
		       [](const char& c) {
			 return (c == ':') || (c == '/');
		       })) {
	// Since tenants are simply prefixes, there's no really good
	// way to see if one exists or not. So we return the thing and
	// let them try to match against it.
	return Principal::tenant(std::move(s));
      }
    }

    if (a->resource == "root") {
      return Principal::tenant(std::move(a->account));
    }

    static const char rx_str[] = "([^/]*)/(.*)";
    static const regex rx(rx_str, sizeof(rx_str) - 1,
			  ECMAScript | optimize);
    smatch match;
    if (regex_match(a->resource, match, rx)) {
      ceph_assert(match.size() == 3);

      if (match[1] == "user") {
	return Principal::user(std::move(a->account),
			       match[2]);
      }

      if (match[1] == "role") {
	return Principal::role(std::move(a->account),
			       match[2]);
      }
    }
  }

  ldout(cct, 0) << "Supplied principal is discarded: " << s << dendl;
  return boost::none;
}