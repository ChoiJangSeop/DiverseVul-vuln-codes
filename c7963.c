Http::FilterHeadersStatus RoleBasedAccessControlFilter::decodeHeaders(Http::HeaderMap& headers,
                                                                      bool) {
  ENVOY_LOG(debug,
            "checking request: remoteAddress: {}, localAddress: {}, ssl: {}, headers: {}, "
            "dynamicMetadata: {}",
            callbacks_->connection()->remoteAddress()->asString(),
            callbacks_->connection()->localAddress()->asString(),
            callbacks_->connection()->ssl()
                ? "uriSanPeerCertificate: " +
                      absl::StrJoin(callbacks_->connection()->ssl()->uriSanPeerCertificate(), ",") +
                      ", subjectPeerCertificate: " +
                      callbacks_->connection()->ssl()->subjectPeerCertificate()
                : "none",
            headers, callbacks_->streamInfo().dynamicMetadata().DebugString());

  std::string effective_policy_id;
  const auto shadow_engine =
      config_->engine(callbacks_->route(), Filters::Common::RBAC::EnforcementMode::Shadow);

  if (shadow_engine != nullptr) {
    std::string shadow_resp_code =
        Filters::Common::RBAC::DynamicMetadataKeysSingleton::get().EngineResultAllowed;
    if (shadow_engine->allowed(*callbacks_->connection(), headers, callbacks_->streamInfo(),
                               &effective_policy_id)) {
      ENVOY_LOG(debug, "shadow allowed");
      config_->stats().shadow_allowed_.inc();
    } else {
      ENVOY_LOG(debug, "shadow denied");
      config_->stats().shadow_denied_.inc();
      shadow_resp_code =
          Filters::Common::RBAC::DynamicMetadataKeysSingleton::get().EngineResultDenied;
    }

    ProtobufWkt::Struct metrics;

    auto& fields = *metrics.mutable_fields();
    if (!effective_policy_id.empty()) {
      *fields[Filters::Common::RBAC::DynamicMetadataKeysSingleton::get()
                  .ShadowEffectivePolicyIdField]
           .mutable_string_value() = effective_policy_id;
    }

    *fields[Filters::Common::RBAC::DynamicMetadataKeysSingleton::get().ShadowEngineResultField]
         .mutable_string_value() = shadow_resp_code;

    callbacks_->streamInfo().setDynamicMetadata(HttpFilterNames::get().Rbac, metrics);
  }

  const auto engine =
      config_->engine(callbacks_->route(), Filters::Common::RBAC::EnforcementMode::Enforced);
  if (engine != nullptr) {
    if (engine->allowed(*callbacks_->connection(), headers, callbacks_->streamInfo(), nullptr)) {
      ENVOY_LOG(debug, "enforced allowed");
      config_->stats().allowed_.inc();
      return Http::FilterHeadersStatus::Continue;
    } else {
      ENVOY_LOG(debug, "enforced denied");
      callbacks_->sendLocalReply(Http::Code::Forbidden, "RBAC: access denied", nullptr,
                                 absl::nullopt, RcDetails::get().RbacAccessDenied);
      config_->stats().denied_.inc();
      return Http::FilterHeadersStatus::StopIteration;
    }
  }

  ENVOY_LOG(debug, "no engine, allowed by default");
  return Http::FilterHeadersStatus::Continue;
}