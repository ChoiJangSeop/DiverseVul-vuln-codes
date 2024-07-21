HttpConnectionManagerConfig::HttpConnectionManagerConfig(
    const envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager&
        config,
    Server::Configuration::FactoryContext& context, Http::DateProvider& date_provider,
    Router::RouteConfigProviderManager& route_config_provider_manager,
    Config::ConfigProviderManager& scoped_routes_config_provider_manager,
    Tracing::HttpTracerManager& http_tracer_manager,
    Filter::Http::FilterConfigProviderManager& filter_config_provider_manager)
    : context_(context), stats_prefix_(fmt::format("http.{}.", config.stat_prefix())),
      stats_(Http::ConnectionManagerImpl::generateStats(stats_prefix_, context_.scope())),
      tracing_stats_(
          Http::ConnectionManagerImpl::generateTracingStats(stats_prefix_, context_.scope())),
      use_remote_address_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(config, use_remote_address, false)),
      internal_address_config_(createInternalAddressConfig(config)),
      xff_num_trusted_hops_(config.xff_num_trusted_hops()),
      skip_xff_append_(config.skip_xff_append()), via_(config.via()),
      route_config_provider_manager_(route_config_provider_manager),
      scoped_routes_config_provider_manager_(scoped_routes_config_provider_manager),
      filter_config_provider_manager_(filter_config_provider_manager),
      http3_options_(Http3::Utility::initializeAndValidateOptions(
          config.http3_protocol_options(), config.has_stream_error_on_invalid_http_message(),
          config.stream_error_on_invalid_http_message())),
      http2_options_(Http2::Utility::initializeAndValidateOptions(
          config.http2_protocol_options(), config.has_stream_error_on_invalid_http_message(),
          config.stream_error_on_invalid_http_message())),
      http1_settings_(Http::Http1::parseHttp1Settings(
          config.http_protocol_options(), context.messageValidationVisitor(),
          config.stream_error_on_invalid_http_message(),
          xff_num_trusted_hops_ == 0 && use_remote_address_)),
      max_request_headers_kb_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(
          config, max_request_headers_kb, Http::DEFAULT_MAX_REQUEST_HEADERS_KB)),
      max_request_headers_count_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(
          config.common_http_protocol_options(), max_headers_count,
          context.runtime().snapshot().getInteger(Http::MaxRequestHeadersCountOverrideKey,
                                                  Http::DEFAULT_MAX_HEADERS_COUNT))),
      idle_timeout_(PROTOBUF_GET_OPTIONAL_MS(config.common_http_protocol_options(), idle_timeout)),
      max_connection_duration_(
          PROTOBUF_GET_OPTIONAL_MS(config.common_http_protocol_options(), max_connection_duration)),
      max_stream_duration_(
          PROTOBUF_GET_OPTIONAL_MS(config.common_http_protocol_options(), max_stream_duration)),
      stream_idle_timeout_(
          PROTOBUF_GET_MS_OR_DEFAULT(config, stream_idle_timeout, StreamIdleTimeoutMs)),
      request_timeout_(PROTOBUF_GET_MS_OR_DEFAULT(config, request_timeout, RequestTimeoutMs)),
      request_headers_timeout_(
          PROTOBUF_GET_MS_OR_DEFAULT(config, request_headers_timeout, RequestHeaderTimeoutMs)),
      drain_timeout_(PROTOBUF_GET_MS_OR_DEFAULT(config, drain_timeout, 5000)),
      generate_request_id_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(config, generate_request_id, true)),
      preserve_external_request_id_(config.preserve_external_request_id()),
      always_set_request_id_in_response_(config.always_set_request_id_in_response()),
      date_provider_(date_provider),
      listener_stats_(Http::ConnectionManagerImpl::generateListenerStats(stats_prefix_,
                                                                         context_.listenerScope())),
      proxy_100_continue_(config.proxy_100_continue()),
      stream_error_on_invalid_http_messaging_(
          PROTOBUF_GET_WRAPPED_OR_DEFAULT(config, stream_error_on_invalid_http_message, false)),
      delayed_close_timeout_(PROTOBUF_GET_MS_OR_DEFAULT(config, delayed_close_timeout, 1000)),
#ifdef ENVOY_NORMALIZE_PATH_BY_DEFAULT
      normalize_path_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(
          config, normalize_path,
          // TODO(htuch): we should have a boolean variant of featureEnabled() here.
          context.runtime().snapshot().featureEnabled("http_connection_manager.normalize_path",
                                                      100))),
#else
      normalize_path_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(
          config, normalize_path,
          // TODO(htuch): we should have a boolean variant of featureEnabled() here.
          context.runtime().snapshot().featureEnabled("http_connection_manager.normalize_path",
                                                      0))),
#endif
      merge_slashes_(config.merge_slashes()),
      headers_with_underscores_action_(
          config.common_http_protocol_options().headers_with_underscores_action()),
      local_reply_(LocalReply::Factory::create(config.local_reply_config(), context)) {
  // If idle_timeout_ was not configured in common_http_protocol_options, use value in deprecated
  // idle_timeout field.
  // TODO(asraa): Remove when idle_timeout is removed.
  if (!idle_timeout_) {
    idle_timeout_ = PROTOBUF_GET_OPTIONAL_MS(config, hidden_envoy_deprecated_idle_timeout);
  }
  if (!idle_timeout_) {
    idle_timeout_ = std::chrono::hours(1);
  } else if (idle_timeout_.value().count() == 0) {
    idle_timeout_ = absl::nullopt;
  }

  if (config.strip_any_host_port() && config.strip_matching_host_port()) {
    throw EnvoyException(fmt::format(
        "Error: Only one of `strip_matching_host_port` or `strip_any_host_port` can be set."));
  }

  if (config.strip_any_host_port()) {
    strip_port_type_ = Http::StripPortType::Any;
  } else if (config.strip_matching_host_port()) {
    strip_port_type_ = Http::StripPortType::MatchingHost;
  } else {
    strip_port_type_ = Http::StripPortType::None;
  }

  // If we are provided a different request_id_extension implementation to use try and create a
  // new instance of it, otherwise use default one.
  envoy::extensions::filters::network::http_connection_manager::v3::RequestIDExtension
      final_rid_config = config.request_id_extension();
  if (!final_rid_config.has_typed_config()) {
    // This creates a default version of the UUID extension which is a required extension in the
    // build.
    final_rid_config.mutable_typed_config()->PackFrom(
        envoy::extensions::request_id::uuid::v3::UuidRequestIdConfig());
  }
  request_id_extension_ = Http::RequestIDExtensionFactory::fromProto(final_rid_config, context_);

  // If scoped RDS is enabled, avoid creating a route config provider. Route config providers will
  // be managed by the scoped routing logic instead.
  switch (config.route_specifier_case()) {
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      RouteSpecifierCase::kRds:
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      RouteSpecifierCase::kRouteConfig:
    route_config_provider_ = Router::RouteConfigProviderUtil::create(
        config, context_.getServerFactoryContext(), context_.messageValidationVisitor(),
        context_.initManager(), stats_prefix_, route_config_provider_manager_);
    break;
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      RouteSpecifierCase::kScopedRoutes:
    scoped_routes_config_provider_ = Router::ScopedRoutesConfigProviderUtil::create(
        config, context_.getServerFactoryContext(), context_.initManager(), stats_prefix_,
        scoped_routes_config_provider_manager_);
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  switch (config.forward_client_cert_details()) {
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      SANITIZE:
    forward_client_cert_ = Http::ForwardClientCertType::Sanitize;
    break;
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      FORWARD_ONLY:
    forward_client_cert_ = Http::ForwardClientCertType::ForwardOnly;
    break;
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      APPEND_FORWARD:
    forward_client_cert_ = Http::ForwardClientCertType::AppendForward;
    break;
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      SANITIZE_SET:
    forward_client_cert_ = Http::ForwardClientCertType::SanitizeSet;
    break;
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      ALWAYS_FORWARD_ONLY:
    forward_client_cert_ = Http::ForwardClientCertType::AlwaysForwardOnly;
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  const auto& set_current_client_cert_details = config.set_current_client_cert_details();
  if (set_current_client_cert_details.cert()) {
    set_current_client_cert_details_.push_back(Http::ClientCertDetailsType::Cert);
  }
  if (set_current_client_cert_details.chain()) {
    set_current_client_cert_details_.push_back(Http::ClientCertDetailsType::Chain);
  }
  if (PROTOBUF_GET_WRAPPED_OR_DEFAULT(set_current_client_cert_details, subject, false)) {
    set_current_client_cert_details_.push_back(Http::ClientCertDetailsType::Subject);
  }
  if (set_current_client_cert_details.uri()) {
    set_current_client_cert_details_.push_back(Http::ClientCertDetailsType::URI);
  }
  if (set_current_client_cert_details.dns()) {
    set_current_client_cert_details_.push_back(Http::ClientCertDetailsType::DNS);
  }

  if (config.has_add_user_agent() && config.add_user_agent().value()) {
    user_agent_ = context_.localInfo().clusterName();
  }

  if (config.has_tracing()) {
    http_tracer_ = http_tracer_manager.getOrCreateHttpTracer(getPerFilterTracerConfig(config));

    const auto& tracing_config = config.tracing();

    Tracing::OperationName tracing_operation_name;

    // Listener level traffic direction overrides the operation name
    switch (context.direction()) {
    case envoy::config::core::v3::UNSPECIFIED: {
      switch (tracing_config.hidden_envoy_deprecated_operation_name()) {
      case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
          Tracing::INGRESS:
        tracing_operation_name = Tracing::OperationName::Ingress;
        break;
      case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
          Tracing::EGRESS:
        tracing_operation_name = Tracing::OperationName::Egress;
        break;
      default:
        NOT_REACHED_GCOVR_EXCL_LINE;
      }
      break;
    }
    case envoy::config::core::v3::INBOUND:
      tracing_operation_name = Tracing::OperationName::Ingress;
      break;
    case envoy::config::core::v3::OUTBOUND:
      tracing_operation_name = Tracing::OperationName::Egress;
      break;
    default:
      NOT_REACHED_GCOVR_EXCL_LINE;
    }

    Tracing::CustomTagMap custom_tags;
    for (const std::string& header :
         tracing_config.hidden_envoy_deprecated_request_headers_for_tags()) {
      envoy::type::tracing::v3::CustomTag::Header headerTag;
      headerTag.set_name(header);
      custom_tags.emplace(
          header, std::make_shared<const Tracing::RequestHeaderCustomTag>(header, headerTag));
    }
    for (const auto& tag : tracing_config.custom_tags()) {
      custom_tags.emplace(tag.tag(), Tracing::HttpTracerUtility::createCustomTag(tag));
    }

    envoy::type::v3::FractionalPercent client_sampling;
    client_sampling.set_numerator(
        tracing_config.has_client_sampling() ? tracing_config.client_sampling().value() : 100);
    envoy::type::v3::FractionalPercent random_sampling;
    // TODO: Random sampling historically was an integer and default to out of 10,000. We should
    // deprecate that and move to a straight fractional percent config.
    uint64_t random_sampling_numerator{PROTOBUF_PERCENT_TO_ROUNDED_INTEGER_OR_DEFAULT(
        tracing_config, random_sampling, 10000, 10000)};
    random_sampling.set_numerator(random_sampling_numerator);
    random_sampling.set_denominator(envoy::type::v3::FractionalPercent::TEN_THOUSAND);
    envoy::type::v3::FractionalPercent overall_sampling;
    overall_sampling.set_numerator(
        tracing_config.has_overall_sampling() ? tracing_config.overall_sampling().value() : 100);

    const uint32_t max_path_tag_length = PROTOBUF_GET_WRAPPED_OR_DEFAULT(
        tracing_config, max_path_tag_length, Tracing::DefaultMaxPathTagLength);

    tracing_config_ =
        std::make_unique<Http::TracingConnectionManagerConfig>(Http::TracingConnectionManagerConfig{
            tracing_operation_name, custom_tags, client_sampling, random_sampling, overall_sampling,
            tracing_config.verbose(), max_path_tag_length});
  }

  for (const auto& access_log : config.access_log()) {
    AccessLog::InstanceSharedPtr current_access_log =
        AccessLog::AccessLogFactory::fromProto(access_log, context_);
    access_logs_.push_back(current_access_log);
  }

  server_transformation_ = config.server_header_transformation();

  if (!config.server_name().empty()) {
    server_name_ = config.server_name();
  } else {
    server_name_ = Http::DefaultServerString::get();
  }

  switch (config.codec_type()) {
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      AUTO:
    codec_type_ = CodecType::AUTO;
    break;
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      HTTP1:
    codec_type_ = CodecType::HTTP1;
    break;
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      HTTP2:
    codec_type_ = CodecType::HTTP2;
    break;
  case envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      HTTP3:
#ifdef ENVOY_ENABLE_QUIC
    codec_type_ = CodecType::HTTP3;
#else
    throw EnvoyException("HTTP3 configured but not enabled in the build.");
#endif
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  const auto& filters = config.http_filters();
  DependencyManager dependency_manager;
  for (int32_t i = 0; i < filters.size(); i++) {
    processFilter(filters[i], i, "http", "http", i == filters.size() - 1, filter_factories_,
                  dependency_manager);
  }
  // TODO(auni53): Validate encode dependencies too.
  auto status = dependency_manager.validDecodeDependencies();
  if (!status.ok()) {
    throw EnvoyException(std::string(status.message()));
  }

  for (const auto& upgrade_config : config.upgrade_configs()) {
    const std::string& name = upgrade_config.upgrade_type();
    const bool enabled = upgrade_config.has_enabled() ? upgrade_config.enabled().value() : true;
    if (findUpgradeCaseInsensitive(upgrade_filter_factories_, name) !=
        upgrade_filter_factories_.end()) {
      throw EnvoyException(
          fmt::format("Error: multiple upgrade configs with the same name: '{}'", name));
    }
    if (!upgrade_config.filters().empty()) {
      std::unique_ptr<FilterFactoriesList> factories = std::make_unique<FilterFactoriesList>();
      DependencyManager upgrade_dependency_manager;
      for (int32_t j = 0; j < upgrade_config.filters().size(); j++) {
        processFilter(upgrade_config.filters(j), j, name, "http upgrade",
                      j == upgrade_config.filters().size() - 1, *factories,
                      upgrade_dependency_manager);
      }
      // TODO(auni53): Validate encode dependencies too.
      auto status = upgrade_dependency_manager.validDecodeDependencies();
      if (!status.ok()) {
        throw EnvoyException(std::string(status.message()));
      }

      upgrade_filter_factories_.emplace(
          std::make_pair(name, FilterConfig{std::move(factories), enabled}));
    } else {
      std::unique_ptr<FilterFactoriesList> factories(nullptr);
      upgrade_filter_factories_.emplace(
          std::make_pair(name, FilterConfig{std::move(factories), enabled}));
    }
  }
}