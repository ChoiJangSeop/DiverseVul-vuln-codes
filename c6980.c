std::unique_ptr<HttpResponse> HttpConnection::receiveResponse()
{
  if (outstandingHttpRequests_.empty()) {
    throw DL_ABORT_EX(EX_NO_HTTP_REQUEST_ENTRY_FOUND);
  }
  if (socketRecvBuffer_->bufferEmpty()) {
    if (socketRecvBuffer_->recv() == 0 && !socket_->wantRead() &&
        !socket_->wantWrite()) {
      throw DL_RETRY_EX(EX_GOT_EOF);
    }
  }

  const auto& proc = outstandingHttpRequests_.front()->getHttpHeaderProcessor();
  if (proc->parse(socketRecvBuffer_->getBuffer(),
                  socketRecvBuffer_->getBufferLength())) {
    A2_LOG_INFO(
        fmt(MSG_RECEIVE_RESPONSE, cuid_, proc->getHeaderString().c_str()));
    auto result = proc->getResult();
    if (result->getStatusCode() / 100 == 1) {
      socketRecvBuffer_->drain(proc->getLastBytesProcessed());
      outstandingHttpRequests_.front()->resetHttpHeaderProcessor();
      return nullptr;
    }

    auto httpResponse = make_unique<HttpResponse>();
    httpResponse->setCuid(cuid_);
    httpResponse->setHttpHeader(std::move(result));
    httpResponse->setHttpRequest(
        outstandingHttpRequests_.front()->popHttpRequest());
    socketRecvBuffer_->drain(proc->getLastBytesProcessed());
    outstandingHttpRequests_.pop_front();
    return httpResponse;
  }

  socketRecvBuffer_->drain(proc->getLastBytesProcessed());
  return nullptr;
}