TEST_F(HttpConnectionManagerConfigTest, NormalizePathDefault) {
  const std::string yaml_string = R"EOF(
  stat_prefix: ingress_http
  route_config:
    name: local_route
  http_filters:
  - name: envoy.filters.http.router
  )EOF";

  EXPECT_CALL(context_.runtime_loader_.snapshot_, featureEnabled(_, An<uint64_t>()))
      .WillOnce(Invoke(&context_.runtime_loader_.snapshot_,
                       &Runtime::MockSnapshot::featureEnabledDefault));
  HttpConnectionManagerConfig config(parseHttpConnectionManagerFromYaml(yaml_string), context_,
                                     date_provider_, route_config_provider_manager_,
                                     scoped_routes_config_provider_manager_, http_tracer_manager_,
                                     filter_config_provider_manager_);
#ifdef ENVOY_NORMALIZE_PATH_BY_DEFAULT
  EXPECT_TRUE(config.shouldNormalizePath());
#else
  EXPECT_FALSE(config.shouldNormalizePath());
#endif
}