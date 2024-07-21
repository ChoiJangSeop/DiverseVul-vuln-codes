  MonCapParser() : MonCapParser::base_type(moncap)
  {
    using qi::char_;
    using qi::int_;
    using qi::ulong_long;
    using qi::lexeme;
    using qi::alnum;
    using qi::_val;
    using qi::_1;
    using qi::_2;
    using qi::_3;
    using qi::eps;
    using qi::lit;

    quoted_string %=
      lexeme['"' >> +(char_ - '"') >> '"'] | 
      lexeme['\'' >> +(char_ - '\'') >> '\''];
    unquoted_word %= +char_("a-zA-Z0-9_.-");
    str %= quoted_string | unquoted_word;

    spaces = +(lit(' ') | lit('\n') | lit('\t'));

    // command := command[=]cmd [k1=v1 k2=v2 ...]
    str_match = '=' >> qi::attr(StringConstraint::MATCH_TYPE_EQUAL) >> str;
    str_prefix = spaces >> lit("prefix") >> spaces >>
                 qi::attr(StringConstraint::MATCH_TYPE_PREFIX) >> str;
    str_regex = spaces >> lit("regex") >> spaces >>
                 qi::attr(StringConstraint::MATCH_TYPE_REGEX) >> str;
    kv_pair = str >> (str_match | str_prefix | str_regex);
    kv_map %= kv_pair >> *(spaces >> kv_pair);
    command_match = -spaces >> lit("allow") >> spaces >> lit("command") >> (lit('=') | spaces)
			    >> qi::attr(string()) >> qi::attr(string())
			    >> str
			    >> -(spaces >> lit("with") >> spaces >> kv_map)
			    >> qi::attr(0);

    // service foo rwxa
    service_match %= -spaces >> lit("allow") >> spaces >> lit("service") >> (lit('=') | spaces)
			     >> str >> qi::attr(string()) >> qi::attr(string())
			     >> qi::attr(map<string,StringConstraint>())
                             >> spaces >> rwxa;

    // profile foo
    profile_match %= -spaces >> -(lit("allow") >> spaces)
                             >> lit("profile") >> (lit('=') | spaces)
			     >> qi::attr(string())
			     >> str
			     >> qi::attr(string())
			     >> qi::attr(map<string,StringConstraint>())
			     >> qi::attr(0);

    // rwxa
    rwxa_match %= -spaces >> lit("allow") >> spaces
			  >> qi::attr(string()) >> qi::attr(string()) >> qi::attr(string())
			  >> qi::attr(map<string,StringConstraint>())
			  >> rwxa;

    // rwxa := * | [r][w][x]
    rwxa =
      (lit("*")[_val = MON_CAP_ANY]) |
      (lit("all")[_val = MON_CAP_ANY]) |
      ( eps[_val = 0] >>
	( lit('r')[_val |= MON_CAP_R] ||
	  lit('w')[_val |= MON_CAP_W] ||
	  lit('x')[_val |= MON_CAP_X]
	  )
	);

    // grant := allow ...
    grant = -spaces >> (rwxa_match | profile_match | service_match | command_match) >> -spaces;

    // moncap := grant [grant ...]
    grants %= (grant % (*lit(' ') >> (lit(';') | lit(',')) >> *lit(' ')));
    moncap = grants  [_val = phoenix::construct<MonCap>(_1)]; 

  }