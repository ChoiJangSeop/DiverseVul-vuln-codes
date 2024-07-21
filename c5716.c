optional<ARN> ARN::parse(const string& s, bool wildcards) {
  static const char str_wild[] = "arn:([^:]*):([^:]*):([^:]*):([^:]*):([^:]*)";
  static const regex rx_wild(str_wild,
				    sizeof(str_wild) - 1,
				    ECMAScript | optimize);
  static const char str_no_wild[]
    = "arn:([^:*]*):([^:*]*):([^:*]*):([^:*]*):([^:*]*)";
  static const regex rx_no_wild(str_no_wild,
				sizeof(str_no_wild) - 1,
				ECMAScript | optimize);

  smatch match;

  if ((s == "*") && wildcards) {
    return ARN(Partition::wildcard, Service::wildcard, "*", "*", "*");
  } else if (regex_match(s, match, wildcards ? rx_wild : rx_no_wild)) {
    ceph_assert(match.size() == 6);

    ARN a;
    {
      auto p = to_partition(match[1], wildcards);
      if (!p)
	return none;

      a.partition = *p;
    }
    {
      auto s = to_service(match[2], wildcards);
      if (!s) {
	return none;
      }
      a.service = *s;
    }

    a.region = match[3];
    a.account = match[4];
    a.resource = match[5];

    return a;
  }
  return none;
}