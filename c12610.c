AsyncSocket::WriteResult AsyncSSLSocket::performWrite(
    const iovec* vec,
    uint32_t count,
    WriteFlags flags,
    uint32_t* countWritten,
    uint32_t* partialWritten) {
  if (sslState_ == STATE_UNENCRYPTED) {
    return AsyncSocket::performWrite(
        vec, count, flags, countWritten, partialWritten);
  }
  if (sslState_ != STATE_ESTABLISHED) {
    LOG(ERROR) << "AsyncSSLSocket(fd=" << fd_ << ", state=" << int(state_)
               << ", sslState=" << sslState_ << ", events=" << eventFlags_
               << "): "
               << "TODO: AsyncSSLSocket currently does not support calling "
               << "write() before the handshake has fully completed";
    return WriteResult(
        WRITE_ERROR, std::make_unique<SSLException>(SSLError::EARLY_WRITE));
  }

  // Declare a buffer used to hold small write requests.  It could point to a
  // memory block either on stack or on heap. If it is on heap, we release it
  // manually when scope exits
  char* combinedBuf{nullptr};
  SCOPE_EXIT {
    // Note, always keep this check consistent with what we do below
    if (combinedBuf != nullptr && minWriteSize_ > MAX_STACK_BUF_SIZE) {
      delete[] combinedBuf;
    }
  };

  *countWritten = 0;
  *partialWritten = 0;
  ssize_t totalWritten = 0;
  size_t bytesStolenFromNextBuffer = 0;
  for (uint32_t i = 0; i < count; i++) {
    const iovec* v = vec + i;
    size_t offset = bytesStolenFromNextBuffer;
    bytesStolenFromNextBuffer = 0;
    size_t len = v->iov_len - offset;
    const void* buf;
    if (len == 0) {
      (*countWritten)++;
      continue;
    }
    buf = ((const char*)v->iov_base) + offset;

    ssize_t bytes;
    uint32_t buffersStolen = 0;
    auto sslWriteBuf = buf;
    if ((len < minWriteSize_) && ((i + 1) < count)) {
      // Combine this buffer with part or all of the next buffers in
      // order to avoid really small-grained calls to SSL_write().
      // Each call to SSL_write() produces a separate record in
      // the egress SSL stream, and we've found that some low-end
      // mobile clients can't handle receiving an HTTP response
      // header and the first part of the response body in two
      // separate SSL records (even if those two records are in
      // the same TCP packet).

      if (combinedBuf == nullptr) {
        if (minWriteSize_ > MAX_STACK_BUF_SIZE) {
          // Allocate the buffer on heap
          combinedBuf = new char[minWriteSize_];
        } else {
          // Allocate the buffer on stack
          combinedBuf = (char*)alloca(minWriteSize_);
        }
      }
      assert(combinedBuf != nullptr);
      sslWriteBuf = combinedBuf;

      memcpy(combinedBuf, buf, len);
      do {
        // INVARIANT: i + buffersStolen == complete chunks serialized
        uint32_t nextIndex = i + buffersStolen + 1;
        bytesStolenFromNextBuffer =
            std::min(vec[nextIndex].iov_len, minWriteSize_ - len);
        if (bytesStolenFromNextBuffer > 0) {
          assert(vec[nextIndex].iov_base != nullptr);
          ::memcpy(
              combinedBuf + len,
              vec[nextIndex].iov_base,
              bytesStolenFromNextBuffer);
        }
        len += bytesStolenFromNextBuffer;
        if (bytesStolenFromNextBuffer < vec[nextIndex].iov_len) {
          // couldn't steal the whole buffer
          break;
        } else {
          bytesStolenFromNextBuffer = 0;
          buffersStolen++;
        }
      } while ((i + buffersStolen + 1) < count && (len < minWriteSize_));
    }

    // Advance any empty buffers immediately after.
    if (bytesStolenFromNextBuffer == 0) {
      while ((i + buffersStolen + 1) < count &&
             vec[i + buffersStolen + 1].iov_len == 0) {
        buffersStolen++;
      }
    }

    // cork the current write if the original flags included CORK or if there
    // are remaining iovec to write
    corkCurrentWrite_ =
        isSet(flags, WriteFlags::CORK) || (i + buffersStolen + 1 < count);

    // track the EoR if:
    //  (1) there are write flags that require EoR tracking (EOR / TIMESTAMP_TX)
    //  (2) if the buffer includes the EOR byte
    appEorByteWriteFlags_ = flags & kEorRelevantWriteFlags;
    bool trackEor = appEorByteWriteFlags_ != folly::WriteFlags::NONE &&
        (i + buffersStolen + 1 == count);
    bytes = eorAwareSSLWrite(ssl_, sslWriteBuf, int(len), trackEor);

    if (bytes <= 0) {
      int error = SSL_get_error(ssl_.get(), int(bytes));
      if (error == SSL_ERROR_WANT_WRITE) {
        // The caller will register for write event if not already.
        *partialWritten = uint32_t(offset);
        return WriteResult(totalWritten);
      }
      auto writeResult = interpretSSLError(int(bytes), error);
      if (writeResult.writeReturn < 0) {
        return writeResult;
      } // else fall through to below to correctly record totalWritten
    }

    totalWritten += bytes;

    if (bytes == (ssize_t)len) {
      // The full iovec is written.
      (*countWritten) += 1 + buffersStolen;
      i += buffersStolen;
      // continue
    } else {
      bytes += offset; // adjust bytes to account for all of v
      while (bytes >= (ssize_t)v->iov_len) {
        // We combined this buf with part or all of the next one, and
        // we managed to write all of this buf but not all of the bytes
        // from the next one that we'd hoped to write.
        bytes -= v->iov_len;
        (*countWritten)++;
        v = &(vec[++i]);
      }
      *partialWritten = uint32_t(bytes);
      return WriteResult(totalWritten);
    }
  }

  return WriteResult(totalWritten);
}