  ConnectionManagerUtilityTest()
      : request_id_extension_(std::make_shared<NiceMock<MockRequestIDExtension>>(random_)),
        request_id_extension_to_return_(request_id_extension_),
        local_reply_(LocalReply::Factory::createDefault()) {
    ON_CALL(config_, userAgent()).WillByDefault(ReturnRef(user_agent_));

    envoy::type::v3::FractionalPercent percent1;
    percent1.set_numerator(100);
    envoy::type::v3::FractionalPercent percent2;
    percent2.set_numerator(10000);
    percent2.set_denominator(envoy::type::v3::FractionalPercent::TEN_THOUSAND);
    tracing_config_ = {
        Tracing::OperationName::Ingress, {}, percent1, percent2, percent1, false, 256};
    ON_CALL(config_, tracingConfig()).WillByDefault(Return(&tracing_config_));
    ON_CALL(config_, localReply()).WillByDefault(ReturnRef(*local_reply_));

    ON_CALL(config_, via()).WillByDefault(ReturnRef(via_));
    ON_CALL(config_, requestIDExtension())
        .WillByDefault(ReturnRef(request_id_extension_to_return_));
  }