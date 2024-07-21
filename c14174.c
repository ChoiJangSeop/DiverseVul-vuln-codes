void HTTPSession::onCertificate(uint16_t certId,
                                std::unique_ptr<IOBuf> authenticator) {
  DestructorGuard dg(this);
  VLOG(4) << "CERTIFICATE on" << *this << ", certId=" << certId;

  bool isValid = false;
  auto fizzBase = getTransport()->getUnderlyingTransport<AsyncFizzBase>();
  if (fizzBase) {
    if (isUpstream()) {
      isValid = secondAuthManager_->validateAuthenticator(
          *fizzBase,
          TransportDirection::UPSTREAM,
          certId,
          std::move(authenticator));
    } else {
      isValid = secondAuthManager_->validateAuthenticator(
          *fizzBase,
          TransportDirection::DOWNSTREAM,
          certId,
          std::move(authenticator));
    }
  } else {
    VLOG(4) << "Underlying transport does not support secondary "
               "authentication.";
    return;
  }
  if (isValid) {
    VLOG(4) << "Successfully validated the authenticator provided by the peer.";
  } else {
    VLOG(4) << "Failed to validate the authenticator provided by the peer";
  }
}