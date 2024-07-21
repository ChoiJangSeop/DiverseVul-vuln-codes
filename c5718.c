bool ParseState::do_string(CephContext* cct, const char* s, size_t l) {
  auto k = pp->tokens.lookup(s, l);
  Policy& p = pp->policy;
  bool is_action = false;
  bool is_validaction = false;
  Statement* t = p.statements.empty() ? nullptr : &(p.statements.back());

  // Top level!
  if ((w->id == TokenID::Version) && k &&
      k->kind == TokenKind::version_key) {
    p.version = static_cast<Version>(k->specific);
  } else if (w->id == TokenID::Id) {
    p.id = string(s, l);

    // Statement

  } else if (w->id == TokenID::Sid) {
    t->sid.emplace(s, l);
  } else if ((w->id == TokenID::Effect) &&
	     k->kind == TokenKind::effect_key) {
    t->effect = static_cast<Effect>(k->specific);
  } else if (w->id == TokenID::Principal && s && *s == '*') {
    t->princ.emplace(Principal::wildcard());
  } else if (w->id == TokenID::NotPrincipal && s && *s == '*') {
    t->noprinc.emplace(Principal::wildcard());
  } else if ((w->id == TokenID::Action) ||
	     (w->id == TokenID::NotAction)) {
    is_action = true;
    for (auto& p : actpairs) {
      if (match_policy({s, l}, p.name, MATCH_POLICY_ACTION)) {
        is_validaction = true;
	(w->id == TokenID::Action ? t->action : t->notaction) |= p.bit;
      }
    }
  } else if (w->id == TokenID::Resource || w->id == TokenID::NotResource) {
    auto a = ARN::parse({s, l}, true);
    // You can't specify resources for someone ELSE'S account.
    if (a && (a->account.empty() || a->account == pp->tenant ||
	      a->account == "*")) {
      if (a->account.empty() || a->account == "*")
	a->account = pp->tenant;
      (w->id == TokenID::Resource ? t->resource : t->notresource)
	.emplace(std::move(*a));
    }
    else
      ldout(cct, 0) << "Supplied resource is discarded: " << string(s, l)
		    << dendl;
  } else if (w->kind == TokenKind::cond_key) {
    auto& t = pp->policy.statements.back();
    t.conditions.back().vals.emplace_back(s, l);

    // Principals

  } else if (w->kind == TokenKind::princ_type) {
    ceph_assert(pp->s.size() > 1);
    auto& pri = pp->s[pp->s.size() - 2].w->id == TokenID::Principal ?
      t->princ : t->noprinc;

    auto o = parse_principal(pp->cct, w->id, string(s, l));
    if (o)
      pri.emplace(std::move(*o));

    // Failure

  } else {
    return false;
  }

  if (!arraying) {
    pp->s.pop_back();
  }

  if (is_action && !is_validaction){
    return false;
  }

  return true;
}