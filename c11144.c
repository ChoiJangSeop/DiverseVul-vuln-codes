void ServerSecurityFeature::collectOptions(std::shared_ptr<ProgramOptions> options) {
  options->addOption("--server.harden",
                     "lock down REST APIs that reveal version information or server "
                     "internals for non-admin users",
                     new BooleanParameter(&_hardenedRestApi))
                     .setIntroducedIn(30500);

  options->addOption("--foxx.api", "enables Foxx management REST APIs",
                     new BooleanParameter(&_enableFoxxApi),
                     arangodb::options::makeFlags(
                     arangodb::options::Flags::DefaultNoComponents,
                     arangodb::options::Flags::OnCoordinator,
                     arangodb::options::Flags::OnSingle))
                     .setIntroducedIn(30500);
  options->addOption("--foxx.store", "enables Foxx store in web interface",
                     new BooleanParameter(&_enableFoxxStore),
                     arangodb::options::makeFlags(
                     arangodb::options::Flags::DefaultNoComponents,
                     arangodb::options::Flags::OnCoordinator,
                     arangodb::options::Flags::OnSingle))
                     .setIntroducedIn(30500);

}