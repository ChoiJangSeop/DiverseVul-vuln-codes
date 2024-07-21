ListenerImpl::ListenerImpl(const envoy::config::listener::v3::Listener& config,
                           const std::string& version_info, ListenerManagerImpl& parent,
                           const std::string& name, bool added_via_api, bool workers_started,
                           uint64_t hash, uint32_t concurrency)
    : parent_(parent), address_(Network::Address::resolveProtoAddress(config.address())),
      bind_to_port_(PROTOBUF_GET_WRAPPED_OR_DEFAULT(config.deprecated_v1(), bind_to_port, true)),
      hand_off_restored_destination_connections_(
          PROTOBUF_GET_WRAPPED_OR_DEFAULT(config, hidden_envoy_deprecated_use_original_dst, false)),
      per_connection_buffer_limit_bytes_(
          PROTOBUF_GET_WRAPPED_OR_DEFAULT(config, per_connection_buffer_limit_bytes, 1024 * 1024)),
      listener_tag_(parent_.factory_.nextListenerTag()), name_(name), added_via_api_(added_via_api),
      workers_started_(workers_started), hash_(hash),
      validation_visitor_(
          added_via_api_ ? parent_.server_.messageValidationContext().dynamicValidationVisitor()
                         : parent_.server_.messageValidationContext().staticValidationVisitor()),
      listener_init_target_(fmt::format("Listener-init-target {}", name),
                            [this]() { dynamic_init_manager_->initialize(local_init_watcher_); }),
      dynamic_init_manager_(std::make_unique<Init::ManagerImpl>(
          fmt::format("Listener-local-init-manager {} {}", name, hash))),
      config_(config), version_info_(version_info),
      listener_filters_timeout_(
          PROTOBUF_GET_MS_OR_DEFAULT(config, listener_filters_timeout, 15000)),
      continue_on_listener_filters_timeout_(config.continue_on_listener_filters_timeout()),
      listener_factory_context_(std::make_shared<PerListenerFactoryContextImpl>(
          parent.server_, validation_visitor_, config, this, *this,
          parent.factory_.createDrainManager(config.drain_type()))),
      filter_chain_manager_(address_, listener_factory_context_->parentFactoryContext(),
                            initManager()),
      local_init_watcher_(fmt::format("Listener-local-init-watcher {}", name), [this] {
        if (workers_started_) {
          parent_.onListenerWarmed(*this);
        } else {
          // Notify Server that this listener is
          // ready.
          listener_init_target_.ready();
        }
      }) {
  buildAccessLog();
  auto socket_type = Network::Utility::protobufAddressSocketType(config.address());
  buildListenSocketOptions(socket_type);
  buildUdpListenerFactory(socket_type, concurrency);
  createListenerFilterFactories(socket_type);
  validateFilterChains(socket_type);
  buildFilterChains();
  if (socket_type == Network::Socket::Type::Datagram) {
    return;
  }
  buildSocketOptions();
  buildOriginalDstListenerFilter();
  buildProxyProtocolListenerFilter();
  buildTlsInspectorListenerFilter();
  if (!workers_started_) {
    // Initialize dynamic_init_manager_ from Server's init manager if it's not initialized.
    // NOTE: listener_init_target_ should be added to parent's initManager at the end of the
    // listener constructor so that this listener's children entities could register their targets
    // with their parent's initManager.
    parent_.server_.initManager().add(listener_init_target_);
  }
}