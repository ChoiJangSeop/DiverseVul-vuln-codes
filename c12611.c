AsyncSocket::WriteResult AsyncSSLSocket::interpretSSLError(int rc, int error) {
  if (error == SSL_ERROR_WANT_READ) {
    // Even though we are attempting to write data, SSL_write() may
    // need to read data if renegotiation is being performed.  We currently
    // don't support this and just fail the write.
    LOG(ERROR) << "AsyncSSLSocket(fd=" << fd_ << ", state=" << int(state_)
               << ", sslState=" << sslState_ << ", events=" << eventFlags_
               << "): "
               << "unsupported SSL renegotiation during write";
    return WriteResult(
        WRITE_ERROR,
        std::make_unique<SSLException>(SSLError::INVALID_RENEGOTIATION));
  } else {
    if (zero_return(error, rc, errno)) {
      return WriteResult(0);
    }
    auto errError = ERR_get_error();
    VLOG(3) << "ERROR: AsyncSSLSocket(fd=" << fd_ << ", state=" << int(state_)
            << ", sslState=" << sslState_ << ", events=" << eventFlags_ << "): "
            << "SSL error: " << error << ", errno: " << errno
            << ", func: " << ERR_func_error_string(errError)
            << ", reason: " << ERR_reason_error_string(errError);
    return WriteResult(
        WRITE_ERROR,
        std::make_unique<SSLException>(error, errError, rc, errno));
  }
}