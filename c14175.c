void HTTPSession::onCertificateRequest(uint16_t requestId,
                                       std::unique_ptr<IOBuf> authRequest) {
  DestructorGuard dg(this);
  VLOG(4) << "CERTIFICATE_REQUEST on" << *this << ", requestId=" << requestId;

  std::pair<uint16_t, std::unique_ptr<folly::IOBuf>> authenticator;
  auto fizzBase = getTransport()->getUnderlyingTransport<AsyncFizzBase>();
  if (fizzBase) {
    if (isUpstream()) {
      authenticator =
          secondAuthManager_->getAuthenticator(*fizzBase,
                                               TransportDirection::UPSTREAM,
                                               requestId,
                                               std::move(authRequest));
    } else {
      authenticator =
          secondAuthManager_->getAuthenticator(*fizzBase,
                                               TransportDirection::DOWNSTREAM,
                                               requestId,
                                               std::move(authRequest));
    }
  } else {
    VLOG(4) << "Underlying transport does not support secondary "
               "authentication.";
    return;
  }
  if (codec_->generateCertificate(writeBuf_,
                                  authenticator.first,
                                  std::move(authenticator.second)) > 0) {
    scheduleWrite();
  }
}