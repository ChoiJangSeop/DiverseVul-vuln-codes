read_from_cmd_socket(void *anything)
{
  int status;
  int read_length; /* Length of packet read */
  int expected_length; /* Expected length of packet without auth data */
  unsigned long flags;
  CMD_Request rx_message;
  CMD_Reply tx_message, *prev_tx_message;
  int rx_message_length, tx_message_length;
  int sock_fd;
  union sockaddr_in46 where_from;
  socklen_t from_length;
  IPAddr remote_ip;
  unsigned short remote_port;
  int auth_length;
  int auth_ok;
  int utoken_ok, token_ok;
  int issue_token;
  int valid_ts;
  int authenticated;
  int localhost;
  int allowed;
  unsigned short rx_command;
  unsigned long rx_message_token;
  unsigned long tx_message_token;
  unsigned long rx_message_seq;
  unsigned long rx_attempt;
  struct timeval now;
  struct timeval cooked_now;

  flags = 0;
  rx_message_length = sizeof(rx_message);
  from_length = sizeof(where_from);

  sock_fd = (long)anything;
  status = recvfrom(sock_fd, (char *)&rx_message, rx_message_length, flags,
                    &where_from.u, &from_length);

  if (status < 0) {
    LOG(LOGS_WARN, LOGF_CmdMon, "Error [%s] reading from control socket %d",
        strerror(errno), sock_fd);
    return;
  }

  read_length = status;
  expected_length = PKL_CommandLength(&rx_message);
  rx_command = ntohs(rx_message.command);

  LCL_ReadRawTime(&now);
  LCL_CookTime(&now, &cooked_now, NULL);

  tx_message.version = PROTO_VERSION_NUMBER;
  tx_message.pkt_type = PKT_TYPE_CMD_REPLY;
  tx_message.res1 = 0;
  tx_message.res2 = 0;
  tx_message.command = rx_message.command;
  tx_message.sequence = rx_message.sequence;
  tx_message.reply = htons(RPY_NULL);
  tx_message.number = htons(1);
  tx_message.total = htons(1);
  tx_message.pad1 = 0;
  tx_message.utoken = htonl(utoken);
  /* Set this to a default (invalid) value.  This protects against the
     token field being set to an arbitrary value if we reject the
     message, e.g. due to the host failing the access check. */
  tx_message.token = htonl(0xffffffffUL);
  memset(&tx_message.auth, 0, sizeof(tx_message.auth));

  switch (where_from.u.sa_family) {
    case AF_INET:
      remote_ip.family = IPADDR_INET4;
      remote_ip.addr.in4 = ntohl(where_from.in4.sin_addr.s_addr);
      remote_port = ntohs(where_from.in4.sin_port);
      localhost = (remote_ip.addr.in4 == 0x7f000001UL);
      break;
#ifdef HAVE_IPV6
    case AF_INET6:
      remote_ip.family = IPADDR_INET6;
      memcpy(&remote_ip.addr.in6, where_from.in6.sin6_addr.s6_addr,
          sizeof (remote_ip.addr.in6));
      remote_port = ntohs(where_from.in6.sin6_port);
      /* Check for ::1 */
      for (localhost = 0; localhost < 16; localhost++)
        if (remote_ip.addr.in6[localhost] != 0)
          break;
      localhost = (localhost == 15 && remote_ip.addr.in6[localhost] == 1);
      break;
#endif
    default:
      assert(0);
  }

  allowed = ADF_IsAllowed(access_auth_table, &remote_ip) || localhost;

  if (read_length < offsetof(CMD_Request, data) ||
      rx_message.pkt_type != PKT_TYPE_CMD_REQUEST ||
      rx_message.res1 != 0 ||
      rx_message.res2 != 0) {

    /* We don't know how to process anything like this */
    if (allowed)
      CLG_LogCommandAccess(&remote_ip, CLG_CMD_BAD_PKT, cooked_now.tv_sec);
    
    return;
  }

  if (rx_message.version != PROTO_VERSION_NUMBER) {
    tx_message.status = htons(STT_NOHOSTACCESS);
    if (!LOG_RateLimited()) {
      LOG(LOGS_WARN, LOGF_CmdMon, "Read command packet with protocol version %d (expected %d) from %s:%hu", rx_message.version, PROTO_VERSION_NUMBER, UTI_IPToString(&remote_ip), remote_port);
    }
    if (allowed)
      CLG_LogCommandAccess(&remote_ip, CLG_CMD_BAD_PKT, cooked_now.tv_sec);

    if (rx_message.version >= PROTO_VERSION_MISMATCH_COMPAT) {
      tx_message.status = htons(STT_BADPKTVERSION);
      /* add empty MD5 auth so older clients will not drop
         the reply due to bad length */
      memset(((char *)&tx_message) + PKL_ReplyLength(&tx_message), 0, 16);
      transmit_reply(&tx_message, &where_from, 16);
    }
    return;
  }

  if (rx_command >= N_REQUEST_TYPES) {
    if (!LOG_RateLimited()) {
      LOG(LOGS_WARN, LOGF_CmdMon, "Read command packet with invalid command %d from %s:%hu", rx_command, UTI_IPToString(&remote_ip), remote_port);
    }
    if (allowed)
      CLG_LogCommandAccess(&remote_ip, CLG_CMD_BAD_PKT, cooked_now.tv_sec);

    tx_message.status = htons(STT_INVALID);
    transmit_reply(&tx_message, &where_from, 0);
    return;
  }

  if (read_length < expected_length) {
    if (!LOG_RateLimited()) {
      LOG(LOGS_WARN, LOGF_CmdMon, "Read incorrectly sized command packet from %s:%hu", UTI_IPToString(&remote_ip), remote_port);
    }
    if (allowed)
      CLG_LogCommandAccess(&remote_ip, CLG_CMD_BAD_PKT, cooked_now.tv_sec);

    tx_message.status = htons(STT_BADPKTLENGTH);
    transmit_reply(&tx_message, &where_from, 0);
    return;
  }

  if (!allowed) {
    /* The client is not allowed access, so don't waste any more time
       on him.  Note that localhost is always allowed access
       regardless of the defined access rules - otherwise, we could
       shut ourselves out completely! */

    if (!LOG_RateLimited()) {
      LOG(LOGS_WARN, LOGF_CmdMon, "Command packet received from unauthorised host %s port %d",
          UTI_IPToString(&remote_ip),
          remote_port);
    }

    tx_message.status = htons(STT_NOHOSTACCESS);
    transmit_reply(&tx_message, &where_from, 0);

    return;
  }

  /* OK, we have a valid message.  Now dispatch on message type and process it. */

  /* Do authentication stuff and command tokens here.  Well-behaved
     clients will set their utokens to 0 to save us wasting our time
     if the packet is unauthenticatable. */
  if (rx_message.utoken != 0) {
    auth_ok = check_rx_packet_auth(&rx_message, read_length);
  } else {
    auth_ok = 0;
  }

  /* All this malarky is to protect the system against various forms
     of attack.

     Simple packet forgeries are blocked by requiring the packet to
     authenticate properly with MD5 or other crypto hash.  (The
     assumption is that the command key is in a read-only keys file
     read by the daemon, and is known only to administrators.)

     Replay attacks are prevented by 2 fields in the packet.  The
     'token' field is where the client plays back to us a token that
     he was issued in an earlier reply.  Each time we reply to a
     suitable packet, we issue a new token.  The 'utoken' field is set
     to a new (hopefully increasing) value each time the daemon is
     run.  This prevents packets from a previous incarnation being
     played back at us when the same point in the 'token' sequence
     comes up.  (The token mechanism also prevents a non-idempotent
     command from being executed twice from the same client, if the
     client fails to receive our reply the first time and tries a
     resend.)

     The problem is how a client should get its first token.  Our
     token handling only remembers a finite number of issued tokens
     (actually 32) - if a client replies with a (legitimate) token
     older than that, it will be treated as though a duplicate token
     has been supplied.  If a simple token-request protocol were used,
     the whole thing would be vulnerable to a denial of service
     attack, where an attacker just replays valid token-request
     packets at us, causing us to keep issuing new tokens,
     invalidating all the ones we have given out to true clients
     already.

     To protect against this, the token-request (REQ_LOGON) packet
     includes a timestamp field.  To issue a token, we require that
     this field is different from any we have processed before.  To
     bound our storage, we require that the timestamp is within a
     certain period of our current time.  For clients running on the
     same host this will be easily satisfied.

     */

  utoken_ok = (ntohl(rx_message.utoken) == utoken);

  /* Avoid binning a valid user's token if we merely get a forged
     packet */
  rx_message_token = ntohl(rx_message.token);
  rx_message_seq = ntohl(rx_message.sequence);
  rx_attempt = ntohs(rx_message.attempt);

  if (auth_ok && utoken_ok) {
    token_ok = check_token(rx_message_token);
  } else {
    token_ok = 0;
  }

  if (auth_ok && utoken_ok && !token_ok) {
    /* This might be a resent message, due to the client not getting
       our reply to the first attempt.  See if we can find the message. */
    prev_tx_message = lookup_reply(rx_message_token, rx_message_seq, rx_attempt);
    if (prev_tx_message) {
      /* Just send this message again */
      tx_message_length = PKL_ReplyLength(prev_tx_message);
      status = sendto(sock_fd, (void *) prev_tx_message, tx_message_length, 0,
                      &where_from.u, from_length);
      if (status < 0 && !LOG_RateLimited()) {
        LOG(LOGS_WARN, LOGF_CmdMon, "Could not send response to %s:%hu", UTI_IPToString(&remote_ip), remote_port);
      }
      return;
    }
    /* Otherwise, just fall through into normal processing */

  }

  if (auth_ok && utoken_ok && token_ok) {
    /* See whether we can discard the previous reply from storage */
    token_acknowledged(rx_message_token, &now);
  }

  valid_ts = 0;

  if (auth_ok) {
    struct timeval ts;

    UTI_TimevalNetworkToHost(&rx_message.data.logon.ts, &ts);
    if ((utoken_ok && token_ok) ||
        ((ntohl(rx_message.utoken) == SPECIAL_UTOKEN) &&
         (rx_command == REQ_LOGON) &&
         (valid_ts = ts_is_unique_and_not_stale(&ts, &now))))
      issue_token = 1;
    else 
      issue_token = 0;
  } else {
    issue_token = 0;
  }

  authenticated = auth_ok & utoken_ok & token_ok;

  if (authenticated) {
    CLG_LogCommandAccess(&remote_ip, CLG_CMD_AUTH, cooked_now.tv_sec);
  } else {
    CLG_LogCommandAccess(&remote_ip, CLG_CMD_NORMAL, cooked_now.tv_sec);
  }

  if (issue_token) {
    /* Only command clients where the user has apparently 'logged on'
       get a token to allow them to emit an authenticated command next
       time */
    tx_message_token = get_token();
  } else {
    tx_message_token = 0xffffffffUL;
  }

  tx_message.token = htonl(tx_message_token);


  if (rx_command >= N_REQUEST_TYPES) {
    /* This should be already handled */
    assert(0);
  } else {
    allowed = 0;

    /* Check level of authority required to issue the command */
    switch(permissions[rx_command]) {
      case PERMIT_AUTH:
        if (authenticated) {
          allowed = 1;
        } else {
          allowed = 0;
        }
        break;
      case PERMIT_LOCAL:
        if (authenticated || localhost) {
          allowed = 1;
        } else {
          allowed = 0;
        }
        break;
      case PERMIT_OPEN:
        allowed = 1;
        break;
      default:
        assert(0);
    }

    if (allowed) {
      switch(rx_command) {
        case REQ_NULL:
          handle_null(&rx_message, &tx_message);
          break;

        case REQ_ONLINE:
          handle_online(&rx_message, &tx_message);
          break;

        case REQ_OFFLINE:
          handle_offline(&rx_message, &tx_message);
          break;

        case REQ_BURST:
          handle_burst(&rx_message, &tx_message);
          break;

        case REQ_MODIFY_MINPOLL:
          handle_modify_minpoll(&rx_message, &tx_message);
          break;

        case REQ_MODIFY_MAXPOLL:
          handle_modify_maxpoll(&rx_message, &tx_message);
          break;

        case REQ_DUMP:
          SRC_DumpSources();
          tx_message.status = htons(STT_SUCCESS);
          break;

        case REQ_MODIFY_MAXDELAY:
          handle_modify_maxdelay(&rx_message, &tx_message);
          break;

        case REQ_MODIFY_MAXDELAYRATIO:
          handle_modify_maxdelayratio(&rx_message, &tx_message);
          break;

        case REQ_MODIFY_MAXDELAYDEVRATIO:
          handle_modify_maxdelaydevratio(&rx_message, &tx_message);
          break;

        case REQ_MODIFY_MAXUPDATESKEW:
          handle_modify_maxupdateskew(&rx_message, &tx_message);
          break;

        case REQ_LOGON:
          /* If the log-on fails, record the reason why */
          if (!issue_token && !LOG_RateLimited()) {
            LOG(LOGS_WARN, LOGF_CmdMon,
                "Bad command logon from %s port %d (auth_ok=%d valid_ts=%d)",
                UTI_IPToString(&remote_ip),
                remote_port,
                auth_ok, valid_ts);
          }

          if (issue_token == 1) {
            tx_message.status = htons(STT_SUCCESS);
          } else if (!auth_ok) {
            tx_message.status = htons(STT_UNAUTH);
          } else if (!valid_ts) {
            tx_message.status = htons(STT_INVALIDTS);
          } else {
            tx_message.status = htons(STT_FAILED);
          }
            
          break;

        case REQ_SETTIME:
          handle_settime(&rx_message, &tx_message);
          break;
        
        case REQ_LOCAL:
          handle_local(&rx_message, &tx_message);
          break;

        case REQ_MANUAL:
          handle_manual(&rx_message, &tx_message);
          break;

        case REQ_N_SOURCES:
          handle_n_sources(&rx_message, &tx_message);
          break;

        case REQ_SOURCE_DATA:
          handle_source_data(&rx_message, &tx_message);
          break;

        case REQ_REKEY:
          handle_rekey(&rx_message, &tx_message);
          break;

        case REQ_ALLOW:
          handle_allow(&rx_message, &tx_message);
          break;

        case REQ_ALLOWALL:
          handle_allowall(&rx_message, &tx_message);
          break;

        case REQ_DENY:
          handle_deny(&rx_message, &tx_message);
          break;

        case REQ_DENYALL:
          handle_denyall(&rx_message, &tx_message);
          break;

        case REQ_CMDALLOW:
          handle_cmdallow(&rx_message, &tx_message);
          break;

        case REQ_CMDALLOWALL:
          handle_cmdallowall(&rx_message, &tx_message);
          break;

        case REQ_CMDDENY:
          handle_cmddeny(&rx_message, &tx_message);
          break;

        case REQ_CMDDENYALL:
          handle_cmddenyall(&rx_message, &tx_message);
          break;

        case REQ_ACCHECK:
          handle_accheck(&rx_message, &tx_message);
          break;

        case REQ_CMDACCHECK:
          handle_cmdaccheck(&rx_message, &tx_message);
          break;

        case REQ_ADD_SERVER:
          handle_add_source(NTP_SERVER, &rx_message, &tx_message);
          break;

        case REQ_ADD_PEER:
          handle_add_source(NTP_PEER, &rx_message, &tx_message);
          break;

        case REQ_DEL_SOURCE:
          handle_del_source(&rx_message, &tx_message);
          break;

        case REQ_WRITERTC:
          handle_writertc(&rx_message, &tx_message);
          break;
          
        case REQ_DFREQ:
          handle_dfreq(&rx_message, &tx_message);
          break;

        case REQ_DOFFSET:
          handle_doffset(&rx_message, &tx_message);
          break;

        case REQ_TRACKING:
          handle_tracking(&rx_message, &tx_message);
          break;

        case REQ_SOURCESTATS:
          handle_sourcestats(&rx_message, &tx_message);
          break;

        case REQ_RTCREPORT:
          handle_rtcreport(&rx_message, &tx_message);
          break;
          
        case REQ_TRIMRTC:
          handle_trimrtc(&rx_message, &tx_message);
          break;

        case REQ_CYCLELOGS:
          handle_cyclelogs(&rx_message, &tx_message);
          break;

        case REQ_SUBNETS_ACCESSED:
          handle_subnets_accessed(&rx_message, &tx_message);
          break;

        case REQ_CLIENT_ACCESSES:
          handle_client_accesses(&rx_message, &tx_message);
          break;

        case REQ_CLIENT_ACCESSES_BY_INDEX:
          handle_client_accesses_by_index(&rx_message, &tx_message);
          break;

        case REQ_MANUAL_LIST:
          handle_manual_list(&rx_message, &tx_message);
          break;

        case REQ_MANUAL_DELETE:
          handle_manual_delete(&rx_message, &tx_message);
          break;

        case REQ_MAKESTEP:
          handle_make_step(&rx_message, &tx_message);
          break;

        case REQ_ACTIVITY:
          handle_activity(&rx_message, &tx_message);
          break;

        case REQ_RESELECTDISTANCE:
          handle_reselect_distance(&rx_message, &tx_message);
          break;

        case REQ_RESELECT:
          handle_reselect(&rx_message, &tx_message);
          break;

        case REQ_MODIFY_MINSTRATUM:
          handle_modify_minstratum(&rx_message, &tx_message);
          break;

        case REQ_MODIFY_POLLTARGET:
          handle_modify_polltarget(&rx_message, &tx_message);
          break;

        default:
          assert(0);
          break;
      }
    } else {
      tx_message.status = htons(STT_UNAUTH);
    }
  }

  if (auth_ok) {
    auth_length = generate_tx_packet_auth(&tx_message);
  } else {
    auth_length = 0;
  }

  if (token_ok) {
    save_reply(&tx_message,
               rx_message_token,
               tx_message_token,
               rx_message_seq,
               rx_attempt,
               &now);
  }

  /* Transmit the response */
  {
    /* Include a simple way to lose one message in three to test resend */

    static int do_it=1;

    if (do_it) {
      transmit_reply(&tx_message, &where_from, auth_length);
    }

#if 0
    do_it = ((do_it + 1) % 3);
#endif
  }
}