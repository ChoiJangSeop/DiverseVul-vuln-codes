ServerSecurityFeature::ServerSecurityFeature(application_features::ApplicationServer& server)
    : ApplicationFeature(server, "ServerSecurity"),
      _enableFoxxApi(true),
      _enableFoxxStore(true),
      _hardenedRestApi(false) {
  setOptional(false);
  startsAfter<application_features::GreetingsFeaturePhase>();
}