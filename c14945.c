folly::Optional<Param> ReadRecordLayer::readEvent(
    folly::IOBufQueue& socketBuf) {
  if (!unparsedHandshakeData_.empty()) {
    auto param = decodeHandshakeMessage(unparsedHandshakeData_);
    if (param) {
      VLOG(8) << "Received handshake message "
              << toString(boost::apply_visitor(EventVisitor(), *param));
      return param;
    }
  }

  while (true) {
    // Read one record. We read one record at a time since records could cause
    // a change in the record layer.
    auto message = read(socketBuf);
    if (!message) {
      return folly::none;
    }

    if (!unparsedHandshakeData_.empty() &&
        message->type != ContentType::handshake) {
      throw std::runtime_error("spliced handshake data");
    }

    switch (message->type) {
      case ContentType::alert: {
        auto alert = decode<Alert>(std::move(message->fragment));
        if (alert.description == AlertDescription::close_notify) {
          return Param(CloseNotify(socketBuf.move()));
        } else {
          return Param(std::move(alert));
        }
      }
      case ContentType::handshake: {
        unparsedHandshakeData_.append(std::move(message->fragment));
        auto param = decodeHandshakeMessage(unparsedHandshakeData_);
        if (param) {
          VLOG(8) << "Received handshake message "
                  << toString(boost::apply_visitor(EventVisitor(), *param));
          return param;
        } else {
          // If we read handshake data but didn't have enough to get a full
          // message we immediately try to read another record.
          // TODO: add limits on number of records we buffer
          continue;
        }
      }
      case ContentType::application_data:
        return Param(AppData(std::move(message->fragment)));
      default:
        throw std::runtime_error("unknown content type");
    }
  }
}