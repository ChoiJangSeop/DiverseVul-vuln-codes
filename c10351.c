TEST_F(HttpConnectionManagerConfigTest, NormalizePathRuntime) {
  const std::string yaml_string = R"EOF(
  stat_prefix: ingress_http
  route_config:
    name: local_route
  http_filters:
  - name: envoy.filters.http.router
  )EOF";

  EXPECT_CALL(context_.runtime_loader_.snapshot_,
              featureEnabled("http_connection_manager.normalize_path", An<uint64_t>()))
      .WillOnce(Return(true));
  HttpConnectionManagerConfig config(parseHttpConnectionManagerFromYaml(yaml_string), context_,
                                     date_provider_, route_config_provider_manager_,
                                     scoped_routes_config_provider_manager_, http_tracer_manager_,
                                     filter_config_provider_manager_);
  EXPECT_TRUE(config.shouldNormalizePath());
}