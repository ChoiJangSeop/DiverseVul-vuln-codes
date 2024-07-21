void InstanceImpl::onRuntimeReady() {
  // Begin initializing secondary clusters after RTDS configuration has been applied.
  clusterManager().initializeSecondaryClusters(bootstrap_);

  if (bootstrap_.has_hds_config()) {
    const auto& hds_config = bootstrap_.hds_config();
    async_client_manager_ = std::make_unique<Grpc::AsyncClientManagerImpl>(
        *config_.clusterManager(), thread_local_, time_source_, *api_, grpc_context_.statNames());
    hds_delegate_ = std::make_unique<Upstream::HdsDelegate>(
        stats_store_,
        Config::Utility::factoryForGrpcApiConfigSource(*async_client_manager_, hds_config,
                                                       stats_store_, false)
            ->create(),
        hds_config.transport_api_version(), *dispatcher_, Runtime::LoaderSingleton::get(),
        stats_store_, *ssl_context_manager_, *random_generator_, info_factory_, access_log_manager_,
        *config_.clusterManager(), *local_info_, *admin_, *singleton_manager_, thread_local_,
        messageValidationContext().dynamicValidationVisitor(), *api_);
  }
}