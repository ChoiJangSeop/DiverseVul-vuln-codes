  CdsIntegrationTest()
      : HttpIntegrationTest(Http::CodecType::HTTP2, ipVersion(),
                            ConfigHelper::discoveredClustersBootstrap(
                                sotwOrDelta() == Grpc::SotwOrDelta::Sotw ||
                                        sotwOrDelta() == Grpc::SotwOrDelta::UnifiedSotw
                                    ? "GRPC"
                                    : "DELTA_GRPC")) {
    if (sotwOrDelta() == Grpc::SotwOrDelta::UnifiedSotw ||
        sotwOrDelta() == Grpc::SotwOrDelta::UnifiedDelta) {
      config_helper_.addRuntimeOverride("envoy.reloadable_features.unified_mux", "true");
    }
    use_lds_ = false;
    sotw_or_delta_ = sotwOrDelta();
  }