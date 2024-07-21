ListenerImpl::ListenerImpl(const ListenerImpl& origin,
                           const envoy::config::listener::v3::Listener& config,
                           const std::string& version_info, ListenerManagerImpl& parent,
                           const std::string& name, bool added_via_api, bool workers_started,
                           uint64_t hash, uint32_t concurrency)
    : parent_(parent), address_(origin.address_),
      bind_to_port_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(config.deprecated_v1(), bind_to_port, true)),
      hand_off_restored_destination_connections_(
          PROTOBUF_GET_WRAPPED_OR_DEFAULT(config, hidden_envoy_deprecated_use_original_dst, false)),
      per_connection_buffer_limit_bytes_(
          PROTOBUF_GET_WRAPPED_OR_DEFAULT(config, per_connection_buffer_limit_bytes, 1024 * 1024)),
      listener_tag_(origin.listener_tag_), name_(name), added_via_api_(added_via_api),
      workers_started_(workers_started), hash_(hash),
      validation_visitor_(
          added_via_api_ ? parent_.server_.messageValidationContext().dynamicValidationVisitor()
                         : parent_.server_.messageValidationContext().staticValidationVisitor()),
      // listener_init_target_ is not used during in place update because we expect server started.
      listener_init_target_("", nullptr),
      dynamic_init_manager_(std::make_unique<Init::ManagerImpl>(
          fmt::format("Listener-local-init-manager {} {}", name, hash))),
      config_(config), version_info_(version_info),
      listener_filters_timeout_(
          PROTOBUF_GET_MS_OR_DEFAULT(config, listener_filters_timeout, 15000)),
      continue_on_listener_filters_timeout_(config.continue_on_listener_filters_timeout()),
      listener_factory_context_(std::make_shared<PerListenerFactoryContextImpl>(
          origin.listener_factory_context_->listener_factory_context_base_, this, *this)),
      filter_chain_manager_(address_, origin.listener_factory_context_->parentFactoryContext(),
                            initManager(), origin.filter_chain_manager_),
      local_init_watcher_(fmt::format("Listener-local-init-watcher {}", name), [this] {
        ASSERT(workers_started_);
        parent_.inPlaceFilterChainUpdate(*this);
      }) {
  buildAccessLog();
  auto socket_type = Network::Utility::protobufAddressSocketType(config.address());
  buildListenSocketOptions(socket_type);
  buildUdpListenerFactory(socket_type, concurrency);
  createListenerFilterFactories(socket_type);
  validateFilterChains(socket_type);
  buildFilterChains();
  // In place update is tcp only so it's safe to apply below tcp only initialization.
  buildSocketOptions();
  buildOriginalDstListenerFilter();
  buildProxyProtocolListenerFilter();
  buildTlsInspectorListenerFilter();
}