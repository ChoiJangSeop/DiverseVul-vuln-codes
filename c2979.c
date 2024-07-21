parse_socks(const char *data, size_t datalen, socks_request_t *req,
            int log_sockstype, int safe_socks, ssize_t *drain_out,
            size_t *want_length_out)
{
  unsigned int len;
  char tmpbuf[TOR_ADDR_BUF_LEN+1];
  tor_addr_t destaddr;
  uint32_t destip;
  uint8_t socksver;
  char *next, *startaddr;
  unsigned char usernamelen, passlen;
  struct in_addr in;

  if (datalen < 2) {
    /* We always need at least 2 bytes. */
    *want_length_out = 2;
    return 0;
  }

  if (req->socks_version == 5 && !req->got_auth) {
    /* See if we have received authentication.  Strictly speaking, we should
       also check whether we actually negotiated username/password
       authentication.  But some broken clients will send us authentication
       even if we negotiated SOCKS_NO_AUTH. */
    if (*data == 1) { /* username/pass version 1 */
      /* Format is: authversion [1 byte] == 1
                    usernamelen [1 byte]
                    username    [usernamelen bytes]
                    passlen     [1 byte]
                    password    [passlen bytes] */
      usernamelen = (unsigned char)*(data + 1);
      if (datalen < 2u + usernamelen + 1u) {
        *want_length_out = 2u + usernamelen + 1u;
        return 0;
      }
      passlen = (unsigned char)*(data + 2u + usernamelen);
      if (datalen < 2u + usernamelen + 1u + passlen) {
        *want_length_out = 2u + usernamelen + 1u + passlen;
        return 0;
      }
      req->replylen = 2; /* 2 bytes of response */
      req->reply[0] = 1; /* authversion == 1 */
      req->reply[1] = 0; /* authentication successful */
      log_debug(LD_APP,
               "socks5: Accepted username/password without checking.");
      if (usernamelen) {
        req->username = tor_memdup(data+2u, usernamelen);
        req->usernamelen = usernamelen;
      }
      if (passlen) {
        req->password = tor_memdup(data+3u+usernamelen, passlen);
        req->passwordlen = passlen;
      }
      *drain_out = 2u + usernamelen + 1u + passlen;
      req->got_auth = 1;
      *want_length_out = 7; /* Minimal socks5 command. */
      return 0;
    } else if (req->auth_type == SOCKS_USER_PASS) {
      /* unknown version byte */
      log_warn(LD_APP, "Socks5 username/password version %d not recognized; "
               "rejecting.", (int)*data);
      return -1;
    }
  }

  socksver = *data;

  switch (socksver) { /* which version of socks? */
    case 5: /* socks5 */

      if (req->socks_version != 5) { /* we need to negotiate a method */
        unsigned char nummethods = (unsigned char)*(data+1);
        int have_user_pass, have_no_auth;
        int r=0;
        tor_assert(!req->socks_version);
        if (datalen < 2u+nummethods) {
          *want_length_out = 2u+nummethods;
          return 0;
        }
        if (!nummethods)
          return -1;
        req->replylen = 2; /* 2 bytes of response */
        req->reply[0] = 5; /* socks5 reply */
        have_user_pass = (memchr(data+2, SOCKS_USER_PASS, nummethods) !=NULL);
        have_no_auth   = (memchr(data+2, SOCKS_NO_AUTH,   nummethods) !=NULL);
        if (have_user_pass && !(have_no_auth && req->socks_prefer_no_auth)) {
          req->auth_type = SOCKS_USER_PASS;
          req->reply[1] = SOCKS_USER_PASS; /* tell client to use "user/pass"
                                              auth method */
          req->socks_version = 5; /* remember we've already negotiated auth */
          log_debug(LD_APP,"socks5: accepted method 2 (username/password)");
          r=0;
        } else if (have_no_auth) {
          req->reply[1] = SOCKS_NO_AUTH; /* tell client to use "none" auth
                                            method */
          req->socks_version = 5; /* remember we've already negotiated auth */
          log_debug(LD_APP,"socks5: accepted method 0 (no authentication)");
          r=0;
        } else {
          log_warn(LD_APP,
                    "socks5: offered methods don't include 'no auth' or "
                    "username/password. Rejecting.");
          req->reply[1] = '\xFF'; /* reject all methods */
          r=-1;
        }
        /* Remove packet from buf. Some SOCKS clients will have sent extra
         * junk at this point; let's hope it's an authentication message. */
        *drain_out = 2u + nummethods;

        return r;
      }
      if (req->auth_type != SOCKS_NO_AUTH && !req->got_auth) {
        log_warn(LD_APP,
                 "socks5: negotiated authentication, but none provided");
        return -1;
      }
      /* we know the method; read in the request */
      log_debug(LD_APP,"socks5: checking request");
      if (datalen < 7) {/* basic info plus >=1 for addr plus 2 for port */
        *want_length_out = 7;
        return 0; /* not yet */
      }
      req->command = (unsigned char) *(data+1);
      if (req->command != SOCKS_COMMAND_CONNECT &&
          req->command != SOCKS_COMMAND_RESOLVE &&
          req->command != SOCKS_COMMAND_RESOLVE_PTR) {
        /* not a connect or resolve or a resolve_ptr? we don't support it. */
        socks_request_set_socks5_error(req,SOCKS5_COMMAND_NOT_SUPPORTED);

        log_warn(LD_APP,"socks5: command %d not recognized. Rejecting.",
                 req->command);
        return -1;
      }
      switch (*(data+3)) { /* address type */
        case 1: /* IPv4 address */
        case 4: /* IPv6 address */ {
          const int is_v6 = *(data+3) == 4;
          const unsigned addrlen = is_v6 ? 16 : 4;
          log_debug(LD_APP,"socks5: ipv4 address type");
          if (datalen < 6+addrlen) {/* ip/port there? */
            *want_length_out = 6+addrlen;
            return 0; /* not yet */
          }

          if (is_v6)
            tor_addr_from_ipv6_bytes(&destaddr, data+4);
          else
            tor_addr_from_ipv4n(&destaddr, get_uint32(data+4));

          tor_addr_to_str(tmpbuf, &destaddr, sizeof(tmpbuf), 1);

          if (strlen(tmpbuf)+1 > MAX_SOCKS_ADDR_LEN) {
            socks_request_set_socks5_error(req, SOCKS5_GENERAL_ERROR);
            log_warn(LD_APP,
                     "socks5 IP takes %d bytes, which doesn't fit in %d. "
                     "Rejecting.",
                     (int)strlen(tmpbuf)+1,(int)MAX_SOCKS_ADDR_LEN);
            return -1;
          }
          strlcpy(req->address,tmpbuf,sizeof(req->address));
          req->port = ntohs(get_uint16(data+4+addrlen));
          *drain_out = 6+addrlen;
          if (req->command != SOCKS_COMMAND_RESOLVE_PTR &&
              !addressmap_have_mapping(req->address,0)) {
            log_unsafe_socks_warning(5, req->address, req->port, safe_socks);
            if (safe_socks) {
              socks_request_set_socks5_error(req, SOCKS5_NOT_ALLOWED);
              return -1;
            }
          }
          return 1;
        }
        case 3: /* fqdn */
          log_debug(LD_APP,"socks5: fqdn address type");
          if (req->command == SOCKS_COMMAND_RESOLVE_PTR) {
            socks_request_set_socks5_error(req,
                                           SOCKS5_ADDRESS_TYPE_NOT_SUPPORTED);
            log_warn(LD_APP, "socks5 received RESOLVE_PTR command with "
                     "hostname type. Rejecting.");
            return -1;
          }
          len = (unsigned char)*(data+4);
          if (datalen < 7+len) { /* addr/port there? */
            *want_length_out = 7+len;
            return 0; /* not yet */
          }
          if (len+1 > MAX_SOCKS_ADDR_LEN) {
            socks_request_set_socks5_error(req, SOCKS5_GENERAL_ERROR);
            log_warn(LD_APP,
                     "socks5 hostname is %d bytes, which doesn't fit in "
                     "%d. Rejecting.", len+1,MAX_SOCKS_ADDR_LEN);
            return -1;
          }
          memcpy(req->address,data+5,len);
          req->address[len] = 0;
          req->port = ntohs(get_uint16(data+5+len));
          *drain_out = 5+len+2;

          if (string_is_valid_ipv4_address(req->address) ||
              string_is_valid_ipv6_address(req->address)) {
            log_unsafe_socks_warning(5,req->address,req->port,safe_socks);

            if (safe_socks) {
              socks_request_set_socks5_error(req, SOCKS5_NOT_ALLOWED);
              return -1;
            }
          } else if (!string_is_valid_hostname(req->address)) {
            socks_request_set_socks5_error(req, SOCKS5_GENERAL_ERROR);

            log_warn(LD_PROTOCOL,
                     "Your application (using socks5 to port %d) gave Tor "
                     "a malformed hostname: %s. Rejecting the connection.",
                     req->port, escaped(req->address));
            return -1;
          }
          if (log_sockstype)
            log_notice(LD_APP,
                  "Your application (using socks5 to port %d) instructed "
                  "Tor to take care of the DNS resolution itself if "
                  "necessary. This is good.", req->port);
          return 1;
        default: /* unsupported */
          socks_request_set_socks5_error(req,
                                         SOCKS5_ADDRESS_TYPE_NOT_SUPPORTED);
          log_warn(LD_APP,"socks5: unsupported address type %d. Rejecting.",
                   (int) *(data+3));
          return -1;
      }
      tor_assert(0);
    case 4: { /* socks4 */
      enum {socks4, socks4a} socks4_prot = socks4a;
      const char *authstart, *authend;
      /* http://ss5.sourceforge.net/socks4.protocol.txt */
      /* http://ss5.sourceforge.net/socks4A.protocol.txt */

      req->socks_version = 4;
      if (datalen < SOCKS4_NETWORK_LEN) {/* basic info available? */
        *want_length_out = SOCKS4_NETWORK_LEN;
        return 0; /* not yet */
      }
      // buf_pullup(buf, 1280, 0);
      req->command = (unsigned char) *(data+1);
      if (req->command != SOCKS_COMMAND_CONNECT &&
          req->command != SOCKS_COMMAND_RESOLVE) {
        /* not a connect or resolve? we don't support it. (No resolve_ptr with
         * socks4.) */
        log_warn(LD_APP,"socks4: command %d not recognized. Rejecting.",
                 req->command);
        return -1;
      }

      req->port = ntohs(get_uint16(data+2));
      destip = ntohl(get_uint32(data+4));
      if ((!req->port && req->command!=SOCKS_COMMAND_RESOLVE) || !destip) {
        log_warn(LD_APP,"socks4: Port or DestIP is zero. Rejecting.");
        return -1;
      }
      if (destip >> 8) {
        log_debug(LD_APP,"socks4: destip not in form 0.0.0.x.");
        in.s_addr = htonl(destip);
        tor_inet_ntoa(&in,tmpbuf,sizeof(tmpbuf));
        if (strlen(tmpbuf)+1 > MAX_SOCKS_ADDR_LEN) {
          log_debug(LD_APP,"socks4 addr (%d bytes) too long. Rejecting.",
                    (int)strlen(tmpbuf));
          return -1;
        }
        log_debug(LD_APP,
                  "socks4: successfully read destip (%s)",
                  safe_str_client(tmpbuf));
        socks4_prot = socks4;
      }

      authstart = data + SOCKS4_NETWORK_LEN;
      next = memchr(authstart, 0,
                    datalen-SOCKS4_NETWORK_LEN);
      if (!next) {
        if (datalen >= 1024) {
          log_debug(LD_APP, "Socks4 user name too long; rejecting.");
          return -1;
        }
        log_debug(LD_APP,"socks4: Username not here yet.");
        *want_length_out = datalen+1024; /* More than we need, but safe */
        return 0;
      }
      authend = next;
      tor_assert(next < data+datalen);

      startaddr = NULL;
      if (socks4_prot != socks4a &&
          !addressmap_have_mapping(tmpbuf,0)) {
        log_unsafe_socks_warning(4, tmpbuf, req->port, safe_socks);

        if (safe_socks)
          return -1;
      }
      if (socks4_prot == socks4a) {
        if (next+1 == data+datalen) {
          log_debug(LD_APP,"socks4: No part of destaddr here yet.");
          *want_length_out = datalen + 1024; /* More than we need, but safe */
          return 0;
        }
        startaddr = next+1;
        next = memchr(startaddr, 0, data + datalen - startaddr);
        if (!next) {
          if (datalen >= 1024) {
            log_debug(LD_APP,"socks4: Destaddr too long.");
            return -1;
          }
          log_debug(LD_APP,"socks4: Destaddr not all here yet.");
          *want_length_out = datalen + 1024; /* More than we need, but safe */
          return 0;
        }
        if (MAX_SOCKS_ADDR_LEN <= next-startaddr) {
          log_warn(LD_APP,"socks4: Destaddr too long. Rejecting.");
          return -1;
        }
        // tor_assert(next < buf->cur+buf->datalen);

        if (log_sockstype)
          log_notice(LD_APP,
                     "Your application (using socks4a to port %d) instructed "
                     "Tor to take care of the DNS resolution itself if "
                     "necessary. This is good.", req->port);
      }
      log_debug(LD_APP,"socks4: Everything is here. Success.");
      strlcpy(req->address, startaddr ? startaddr : tmpbuf,
              sizeof(req->address));
      if (!tor_strisprint(req->address) || strchr(req->address,'\"')) {
        log_warn(LD_PROTOCOL,
                 "Your application (using socks4 to port %d) gave Tor "
                 "a malformed hostname: %s. Rejecting the connection.",
                 req->port, escaped(req->address));
        return -1;
      }
      if (authend != authstart) {
        req->got_auth = 1;
        req->usernamelen = authend - authstart;
        req->username = tor_memdup(authstart, authend - authstart);
      }
      /* next points to the final \0 on inbuf */
      *drain_out = next - data + 1;
      return 1;
    }
    case 'G': /* get */
    case 'H': /* head */
    case 'P': /* put/post */
    case 'C': /* connect */
      strlcpy((char*)req->reply,
"HTTP/1.0 501 Tor is not an HTTP Proxy\r\n"
"Content-Type: text/html; charset=iso-8859-1\r\n\r\n"
"<html>\n"
"<head>\n"
"<title>Tor is not an HTTP Proxy</title>\n"
"</head>\n"
"<body>\n"
"<h1>Tor is not an HTTP Proxy</h1>\n"
"<p>\n"
"It appears you have configured your web browser to use Tor as an HTTP proxy."
"\n"
"This is not correct: Tor is a SOCKS proxy, not an HTTP proxy.\n"
"Please configure your client accordingly.\n"
"</p>\n"
"<p>\n"
"See <a href=\"https://www.torproject.org/documentation.html\">"
           "https://www.torproject.org/documentation.html</a> for more "
           "information.\n"
"<!-- Plus this comment, to make the body response more than 512 bytes, so "
"     IE will be willing to display it. Comment comment comment comment "
"     comment comment comment comment comment comment comment comment.-->\n"
"</p>\n"
"</body>\n"
"</html>\n"
             , MAX_SOCKS_REPLY_LEN);
      req->replylen = strlen((char*)req->reply)+1;
      /* fall through */
    default: /* version is not socks4 or socks5 */
      log_warn(LD_APP,
               "Socks version %d not recognized. (Tor is not an http proxy.)",
               *(data));
      {
        /* Tell the controller the first 8 bytes. */
        char *tmp = tor_strndup(data, datalen < 8 ? datalen : 8);
        control_event_client_status(LOG_WARN,
                                    "SOCKS_UNKNOWN_PROTOCOL DATA=\"%s\"",
                                    escaped(tmp));
        tor_free(tmp);
      }
      return -1;
  }
}