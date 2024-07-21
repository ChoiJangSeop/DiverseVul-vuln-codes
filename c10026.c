NOEXPORT void transfer(CLI *c) {
    int timeout; /* s_poll_wait timeout in seconds */
    int pending; /* either processed on unprocessed TLS data */
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    int has_pending=0, prev_has_pending;
#endif
    int watchdog=0; /* a counter to detect an infinite loop */
    ssize_t num;
    int err;
    /* logical channels (not file descriptors!) open for read or write */
    int sock_open_rd=1, sock_open_wr=1;
    /* awaited conditions on TLS file descriptors */
    int shutdown_wants_read=0, shutdown_wants_write=0;
    int read_wants_read=0, read_wants_write=0;
    int write_wants_read=0, write_wants_write=0;
    /* actual conditions on file descriptors */
    int sock_can_rd, sock_can_wr, ssl_can_rd, ssl_can_wr;
#ifdef USE_WIN32
    unsigned long bytes;
#else
    int bytes;
#endif

    c->sock_ptr=c->ssl_ptr=0;

    do { /* main loop of client data transfer */
        /****************************** initialize *_wants_* */
        read_wants_read|=!(SSL_get_shutdown(c->ssl)&SSL_RECEIVED_SHUTDOWN)
            && c->ssl_ptr<BUFFSIZE && !read_wants_write;
        write_wants_write|=!(SSL_get_shutdown(c->ssl)&SSL_SENT_SHUTDOWN)
            && c->sock_ptr && !write_wants_read;

        /****************************** setup c->fds structure */
        s_poll_init(c->fds, 0); /* initialize the structure */
        /* for plain socket open data strem = open file descriptor */
        /* make sure to add each open socket to receive exceptions! */
        if(sock_open_rd) /* only poll if the read file descriptor is open */
            s_poll_add(c->fds, c->sock_rfd->fd, c->sock_ptr<BUFFSIZE, 0);
        if(sock_open_wr) /* only poll if the write file descriptor is open */
            s_poll_add(c->fds, c->sock_wfd->fd, 0, c->ssl_ptr>0);
        /* poll TLS file descriptors unless TLS shutdown was completed */
        if(SSL_get_shutdown(c->ssl)!=
                (SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN)) {
            s_poll_add(c->fds, c->ssl_rfd->fd,
                read_wants_read || write_wants_read || shutdown_wants_read, 0);
            s_poll_add(c->fds, c->ssl_wfd->fd, 0,
                read_wants_write || write_wants_write || shutdown_wants_write);
        }

        /****************************** wait for an event */
        pending=SSL_pending(c->ssl);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
        /* only attempt to process SSL_has_pending() data once */
        prev_has_pending=has_pending;
        has_pending=SSL_has_pending(c->ssl);
        pending=pending || (has_pending && !prev_has_pending);
#endif
        if(read_wants_read && pending) {
            timeout=0; /* process any buffered data without delay */
        } else if((sock_open_rd && /* both peers open */
                !(SSL_get_shutdown(c->ssl)&SSL_RECEIVED_SHUTDOWN)) ||
                c->ssl_ptr /* data buffered to write to socket */ ||
                c->sock_ptr /* data buffered to write to TLS */) {
            timeout=c->opt->timeout_idle;
        } else {
            timeout=c->opt->timeout_close;
        }
        err=s_poll_wait(c->fds, timeout, 0);
        switch(err) {
        case -1:
            sockerror("transfer: s_poll_wait");
            throw_exception(c, 1);
        case 0: /* timeout */
            if(read_wants_read && pending)
                break;
            if((sock_open_rd &&
                    !(SSL_get_shutdown(c->ssl)&SSL_RECEIVED_SHUTDOWN)) ||
                    c->ssl_ptr || c->sock_ptr) {
                s_log(LOG_INFO, "transfer: s_poll_wait:"
                    " TIMEOUTidle exceeded: sending reset");
                s_poll_dump(c->fds, LOG_DEBUG);
                throw_exception(c, 1);
            }
            /* already closing connection */
            s_log(LOG_ERR, "transfer: s_poll_wait:"
                " TIMEOUTclose exceeded: closing");
            s_poll_dump(c->fds, LOG_DEBUG);
            return; /* OK */
        }

        /****************************** retrieve results from c->fds */
        sock_can_rd=s_poll_canread(c->fds, c->sock_rfd->fd);
        sock_can_wr=s_poll_canwrite(c->fds, c->sock_wfd->fd);
        ssl_can_rd=s_poll_canread(c->fds, c->ssl_rfd->fd);
        ssl_can_wr=s_poll_canwrite(c->fds, c->ssl_wfd->fd);

        /****************************** identify exceptions */
        if(c->sock_rfd->fd==c->sock_wfd->fd) {
            if((sock_can_rd || sock_can_wr) &&
                    s_poll_err(c->fds, c->sock_rfd->fd)) {
                err=get_socket_error(c->sock_rfd->fd);
                if(err)
                    log_error(LOG_INFO, err, "socket fd");
            }
        } else {
            if(sock_can_rd && s_poll_err(c->fds, c->sock_rfd->fd)) {
                err=get_socket_error(c->sock_rfd->fd);
                if(err)
                    log_error(LOG_INFO, err, "socket rfd");
            }
            if(sock_can_wr && s_poll_err(c->fds, c->sock_wfd->fd)) {
                err=get_socket_error(c->sock_wfd->fd);
                if(err)
                    log_error(LOG_INFO, err, "socket wfd");
            }
        }
        if(c->ssl_rfd->fd==c->ssl_wfd->fd) {
            if((ssl_can_rd || ssl_can_wr) &&
                    s_poll_err(c->fds, c->ssl_rfd->fd)) {
                err=get_socket_error(c->ssl_rfd->fd);
                if(err)
                    log_error(LOG_INFO, err, "TLS fd");
            }
        } else {
            if(ssl_can_rd && s_poll_err(c->fds, c->ssl_rfd->fd)) {
                err=get_socket_error(c->ssl_rfd->fd);
                if(err)
                    log_error(LOG_INFO, err, "TLS rfd");
            }
            if(c->ssl_rfd->fd!=c->ssl_wfd->fd &&
                    ssl_can_wr && s_poll_err(c->fds, c->ssl_wfd->fd)) {
                err=get_socket_error(c->ssl_wfd->fd);
                if(err)
                    log_error(LOG_INFO, err, "TLS wfd");
            }
        }

        /****************************** hangups without read or write */
        if(!(sock_can_rd || sock_can_wr || ssl_can_rd || ssl_can_wr)) {
            if(s_poll_hup(c->fds, c->sock_rfd->fd) ||
                    s_poll_hup(c->fds, c->sock_wfd->fd)) {
                if(c->ssl_ptr) {
                    s_log(LOG_ERR,
                        "Socket closed (HUP) with %ld unsent byte(s)",
                        (long)c->ssl_ptr);
                    throw_exception(c, 1); /* reset the sockets */
                }
                s_log(LOG_INFO, "Socket closed (HUP)");
                sock_open_rd=sock_open_wr=0;
            } else if(s_poll_hup(c->fds, c->ssl_rfd->fd) ||
                    s_poll_hup(c->fds, c->ssl_wfd->fd)) {
                if(c->sock_ptr) {
                    s_log(LOG_ERR,
                        "TLS socket closed (HUP) with %ld unsent byte(s)",
                        (long)c->sock_ptr);
                    throw_exception(c, 1); /* reset the sockets */
                }
                s_log(LOG_INFO, "TLS socket closed (HUP)");
                SSL_set_shutdown(c->ssl,
                    SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
            }
        }

        if(c->reneg_state==RENEG_DETECTED && !c->opt->option.renegotiation) {
            s_log(LOG_ERR, "Aborting due to renegotiation request");
            throw_exception(c, 1);
        }

        /****************************** send TLS close_notify alert */
        if(shutdown_wants_read || shutdown_wants_write) {
            num=SSL_shutdown(c->ssl); /* send close_notify alert */
            if(num<0) /* -1 - not completed */
                err=SSL_get_error(c->ssl, (int)num);
            else /* 0 or 1 - success */
                err=SSL_ERROR_NONE;
            switch(err) {
            case SSL_ERROR_NONE: /* the shutdown was successfully completed */
                s_log(LOG_INFO, "SSL_shutdown successfully sent close_notify alert");
                shutdown_wants_read=shutdown_wants_write=0;
                break;
            case SSL_ERROR_SYSCALL: /* socket error */
                if(parse_socket_error(c, "SSL_shutdown"))
                    break; /* a non-critical error: retry */
                SSL_set_shutdown(c->ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
                shutdown_wants_read=shutdown_wants_write=0;
                break;
            case SSL_ERROR_ZERO_RETURN: /* connection closed */
                SSL_set_shutdown(c->ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
                shutdown_wants_read=shutdown_wants_write=0;
                break;
            case SSL_ERROR_WANT_WRITE:
                s_log(LOG_DEBUG, "SSL_shutdown returned WANT_WRITE: retrying");
                shutdown_wants_read=0;
                shutdown_wants_write=1;
                break;
            case SSL_ERROR_WANT_READ:
                s_log(LOG_DEBUG, "SSL_shutdown returned WANT_READ: retrying");
                shutdown_wants_read=1;
                shutdown_wants_write=0;
                break;
            case SSL_ERROR_SSL: /* TLS error */
                sslerror("SSL_shutdown");
                throw_exception(c, 1);
            default:
                s_log(LOG_ERR, "SSL_shutdown/SSL_get_error returned %d", err);
                throw_exception(c, 1);
            }
        }

        /****************************** write to socket */
        if(sock_open_wr && sock_can_wr) {
            num=writesocket(c->sock_wfd->fd, c->ssl_buff, c->ssl_ptr);
            switch(num) {
            case -1: /* error */
                if(parse_socket_error(c, "writesocket"))
                    break; /* a non-critical error: retry */
                sock_open_rd=sock_open_wr=0;
                break;
            case 0: /* nothing was written: ignore */
                s_log(LOG_DEBUG, "writesocket returned 0");
                break; /* do not reset the watchdog */
            default:
                memmove(c->ssl_buff, c->ssl_buff+num, c->ssl_ptr-(size_t)num);
                c->ssl_ptr-=(size_t)num;
                memset(c->ssl_buff+c->ssl_ptr, 0, (size_t)num); /* paranoia */
                c->sock_bytes+=(size_t)num;
                watchdog=0; /* reset the watchdog */
            }
        }

        /****************************** read from socket */
        if(sock_open_rd && sock_can_rd) {
            num=readsocket(c->sock_rfd->fd,
                c->sock_buff+c->sock_ptr, BUFFSIZE-c->sock_ptr);
            switch(num) {
            case -1:
                if(parse_socket_error(c, "readsocket"))
                    break; /* a non-critical error: retry */
                sock_open_rd=sock_open_wr=0;
                break;
            case 0: /* close */
                s_log(LOG_INFO, "Read socket closed (readsocket)");
                sock_open_rd=0;
                break; /* do not reset the watchdog */
            default:
                c->sock_ptr+=(size_t)num;
                watchdog=0; /* reset the watchdog */
            }
        }

        /****************************** update *_wants_* based on new *_ptr */
        /* this update is also required for SSL_pending() to be used */
        read_wants_read|=!(SSL_get_shutdown(c->ssl)&SSL_RECEIVED_SHUTDOWN)
            && c->ssl_ptr<BUFFSIZE && !read_wants_write;
        write_wants_write|=!(SSL_get_shutdown(c->ssl)&SSL_SENT_SHUTDOWN)
            && c->sock_ptr && !write_wants_read;

        /****************************** write to TLS */
        if((write_wants_read && ssl_can_rd) ||
                (write_wants_write && ssl_can_wr)) {
            write_wants_read=0;
            write_wants_write=0;
            num=SSL_write(c->ssl, c->sock_buff, (int)(c->sock_ptr));
            switch(err=SSL_get_error(c->ssl, (int)num)) {
            case SSL_ERROR_NONE:
                if(num==0) { /* nothing was written: ignore */
                    s_log(LOG_DEBUG, "SSL_write returned 0");
                    break; /* do not reset the watchdog */
                }
                memmove(c->sock_buff, c->sock_buff+num,
                    c->sock_ptr-(size_t)num);
                c->sock_ptr-=(size_t)num;
                memset(c->sock_buff+c->sock_ptr, 0, (size_t)num); /* paranoia */
                c->ssl_bytes+=(size_t)num;
                watchdog=0; /* reset the watchdog */
                break;
            case SSL_ERROR_WANT_WRITE: /* buffered data? */
                s_log(LOG_DEBUG, "SSL_write returned WANT_WRITE: retrying");
                write_wants_write=1;
                break;
            case SSL_ERROR_WANT_READ:
                s_log(LOG_DEBUG, "SSL_write returned WANT_READ: retrying");
                write_wants_read=1;
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                s_log(LOG_DEBUG,
                    "SSL_write returned WANT_X509_LOOKUP: retrying");
                break;
            case SSL_ERROR_SYSCALL: /* socket error */
                if(num && parse_socket_error(c, "SSL_write"))
                    break; /* a non-critical error: retry */
                /* EOF -> buggy (e.g. Microsoft) peer:
                 * TLS socket closed without close_notify alert */
                if(c->sock_ptr) { /* TODO: what about buffered data? */
                    s_log(LOG_ERR,
                        "TLS socket closed (SSL_write) with %ld unsent byte(s)",
                        (long)c->sock_ptr);
                    throw_exception(c, 1); /* reset the sockets */
                }
                s_log(LOG_INFO, "TLS socket closed (SSL_write)");
                SSL_set_shutdown(c->ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
                break;
            case SSL_ERROR_ZERO_RETURN: /* close_notify alert received */
                s_log(LOG_INFO, "TLS closed (SSL_write)");
                if(SSL_version(c->ssl)==SSL2_VERSION)
                    SSL_set_shutdown(c->ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
                break;
            case SSL_ERROR_SSL:
                sslerror("SSL_write");
                throw_exception(c, 1);
            default:
                s_log(LOG_ERR, "SSL_write/SSL_get_error returned %d", err);
                throw_exception(c, 1);
            }
        }

        /****************************** read from TLS */
        if((read_wants_read && (ssl_can_rd || pending)) ||
                /* it may be possible to read some pending data after
                 * writesocket() above made some room in c->ssl_buff */
                (read_wants_write && ssl_can_wr)) {
            read_wants_read=0;
            read_wants_write=0;
            num=SSL_read(c->ssl, c->ssl_buff+c->ssl_ptr, (int)(BUFFSIZE-c->ssl_ptr));
            switch(err=SSL_get_error(c->ssl, (int)num)) {
            case SSL_ERROR_NONE:
                if(num==0) { /* nothing was read: ignore */
                    s_log(LOG_DEBUG, "SSL_read returned 0");
                    break; /* do not reset the watchdog */
                }
                c->ssl_ptr+=(size_t)num;
                watchdog=0; /* reset the watchdog */
                break;
            case SSL_ERROR_WANT_WRITE:
                s_log(LOG_DEBUG, "SSL_read returned WANT_WRITE: retrying");
                read_wants_write=1;
                break;
            case SSL_ERROR_WANT_READ: /* happens quite often */
#if 0
                s_log(LOG_DEBUG, "SSL_read returned WANT_READ: retrying");
#endif
                read_wants_read=1;
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                s_log(LOG_DEBUG,
                    "SSL_read returned WANT_X509_LOOKUP: retrying");
                break;
            case SSL_ERROR_SYSCALL:
                if(num && parse_socket_error(c, "SSL_read"))
                    break; /* a non-critical error: retry */
                /* EOF -> buggy (e.g. Microsoft) peer:
                 * TLS socket closed without close_notify alert */
                if(c->sock_ptr || write_wants_write) {
                    s_log(LOG_ERR,
                        "TLS socket closed (SSL_read) with %ld unsent byte(s)",
                        (long)c->sock_ptr);
                    throw_exception(c, 1); /* reset the sockets */
                }
                s_log(LOG_INFO, "TLS socket closed (SSL_read)");
                SSL_set_shutdown(c->ssl,
                    SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
                break;
            case SSL_ERROR_ZERO_RETURN: /* close_notify alert received */
                s_log(LOG_INFO, "TLS closed (SSL_read)");
                if(SSL_version(c->ssl)==SSL2_VERSION)
                    SSL_set_shutdown(c->ssl,
                        SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
                break;
            case SSL_ERROR_SSL:
                sslerror("SSL_read");
                throw_exception(c, 1);
            default:
                s_log(LOG_ERR, "SSL_read/SSL_get_error returned %d", err);
                throw_exception(c, 1);
            }
        }

        /****************************** check for hangup conditions */
        /* http://marc.info/?l=linux-man&m=128002066306087 */
        /* readsocket() must be the last sock_rfd operation before FIONREAD */
        if(sock_open_rd && s_poll_rdhup(c->fds, c->sock_rfd->fd) &&
                (ioctlsocket(c->sock_rfd->fd, FIONREAD, &bytes) || !bytes)) {
            s_log(LOG_INFO, "Read socket closed (read hangup)");
            sock_open_rd=0;
        }
        if(sock_open_wr && s_poll_hup(c->fds, c->sock_wfd->fd)) {
            if(c->ssl_ptr) {
                s_log(LOG_ERR,
                    "Write socket closed (write hangup) with %ld unsent byte(s)",
                    (long)c->ssl_ptr);
                throw_exception(c, 1); /* reset the sockets */
            }
            s_log(LOG_INFO, "Write socket closed (write hangup)");
            sock_open_wr=0;
        }
        /* SSL_read() must be the last ssl_rfd operation before FIONREAD */
        if(!(SSL_get_shutdown(c->ssl)&SSL_RECEIVED_SHUTDOWN) &&
                s_poll_rdhup(c->fds, c->ssl_rfd->fd) &&
                (ioctlsocket(c->ssl_rfd->fd, FIONREAD, &bytes) || !bytes)) {
            /* hangup -> buggy (e.g. Microsoft) peer:
             * TLS socket closed without close_notify alert */
            s_log(LOG_INFO, "TLS socket closed (read hangup)");
            SSL_set_shutdown(c->ssl,
                SSL_get_shutdown(c->ssl)|SSL_RECEIVED_SHUTDOWN);
        }
        if(!(SSL_get_shutdown(c->ssl)&SSL_SENT_SHUTDOWN) &&
                s_poll_hup(c->fds, c->ssl_wfd->fd)) {
            if(c->sock_ptr || write_wants_write) {
                s_log(LOG_ERR,
                    "TLS socket closed (write hangup) with %ld unsent byte(s)",
                    (long)c->sock_ptr);
                throw_exception(c, 1); /* reset the sockets */
            }
            s_log(LOG_INFO, "TLS socket closed (write hangup)");
            SSL_set_shutdown(c->ssl,
                SSL_get_shutdown(c->ssl)|SSL_SENT_SHUTDOWN);
        }

        /****************************** check write shutdown conditions */
        if(sock_open_wr && SSL_get_shutdown(c->ssl)&SSL_RECEIVED_SHUTDOWN &&
                !c->ssl_ptr) {
            sock_open_wr=0; /* no further write allowed */
            if(!c->sock_wfd->is_socket) {
                s_log(LOG_DEBUG, "Closing the file descriptor");
                sock_open_rd=0; /* file descriptor is ready to be closed */
            } else if(!shutdown(c->sock_wfd->fd, SHUT_WR)) { /* send TCP FIN */
                s_log(LOG_DEBUG, "Sent socket write shutdown");
            } else {
                s_log(LOG_DEBUG, "Failed to send socket write shutdown");
                sock_open_rd=0; /* file descriptor is ready to be closed */
            }
        }
        if(!(SSL_get_shutdown(c->ssl)&SSL_SENT_SHUTDOWN) && !sock_open_rd &&
                !c->sock_ptr && !write_wants_write) {
            if(SSL_version(c->ssl)!=SSL2_VERSION) {
                s_log(LOG_DEBUG, "Sending close_notify alert");
                shutdown_wants_write=1;
            } else { /* no alerts in SSLv2, including the close_notify alert */
                s_log(LOG_DEBUG, "Closing SSLv2 socket");
                if(c->ssl_rfd->is_socket)
                    shutdown(c->ssl_rfd->fd, SHUT_RD); /* notify the kernel */
                if(c->ssl_wfd->is_socket)
                    shutdown(c->ssl_wfd->fd, SHUT_WR); /* send TCP FIN */
                /* notify the OpenSSL library */
                SSL_set_shutdown(c->ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
            }
        }

        /****************************** check watchdog */
        if(++watchdog>100) { /* loop executes without transferring any data */
            s_log(LOG_ERR,
                "transfer() loop executes not transferring any data");
            s_log(LOG_ERR,
                "please report the problem to Michal.Trojnara@stunnel.org");
            stunnel_info(LOG_ERR);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
            s_log(LOG_ERR, "protocol=%s, SSL_pending=%d, SSL_has_pending=%d",
                SSL_get_version(c->ssl),
                SSL_pending(c->ssl), SSL_has_pending(c->ssl));
#else
            s_log(LOG_ERR, "protocol=%s, SSL_pending=%d",
                SSL_get_version(c->ssl), SSL_pending(c->ssl));
#endif
            s_log(LOG_ERR, "sock_open_rd=%s, sock_open_wr=%s",
                sock_open_rd ? "Y" : "n", sock_open_wr ? "Y" : "n");
            s_log(LOG_ERR, "SSL_RECEIVED_SHUTDOWN=%s, SSL_SENT_SHUTDOWN=%s",
                (SSL_get_shutdown(c->ssl) & SSL_RECEIVED_SHUTDOWN) ? "Y" : "n",
                (SSL_get_shutdown(c->ssl) & SSL_SENT_SHUTDOWN) ? "Y" : "n");
            s_log(LOG_ERR, "sock_can_rd=%s, sock_can_wr=%s",
                sock_can_rd ? "Y" : "n", sock_can_wr ? "Y" : "n");
            s_log(LOG_ERR, "ssl_can_rd=%s, ssl_can_wr=%s",
                ssl_can_rd ? "Y" : "n", ssl_can_wr ? "Y" : "n");
            s_log(LOG_ERR, "read_wants_read=%s, read_wants_write=%s",
                read_wants_read ? "Y" : "n", read_wants_write ? "Y" : "n");
            s_log(LOG_ERR, "write_wants_read=%s, write_wants_write=%s",
                write_wants_read ? "Y" : "n", write_wants_write ? "Y" : "n");
            s_log(LOG_ERR, "shutdown_wants_read=%s, shutdown_wants_write=%s",
                shutdown_wants_read ? "Y" : "n",
                shutdown_wants_write ? "Y" : "n");
            s_log(LOG_ERR, "socket input buffer: %ld byte(s), "
                "TLS input buffer: %ld byte(s)",
                (long)c->sock_ptr, (long)c->ssl_ptr);
            throw_exception(c, 1);
        }

    } while(sock_open_wr || !(SSL_get_shutdown(c->ssl)&SSL_SENT_SHUTDOWN) ||
        shutdown_wants_read || shutdown_wants_write);
}