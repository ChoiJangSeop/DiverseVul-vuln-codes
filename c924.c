handle_client_accesses(CMD_Request *rx_message, CMD_Reply *tx_message)
{
  CLG_Status result;
  RPT_ClientAccess_Report report;
  unsigned long nc;
  IPAddr ip;
  int i;
  struct timeval now;

  LCL_ReadCookedTime(&now, NULL);

  nc = ntohl(rx_message->data.client_accesses.n_clients);
  tx_message->status = htons(STT_SUCCESS);
  tx_message->reply = htons(RPY_CLIENT_ACCESSES);
  tx_message->data.client_accesses.n_clients = htonl(nc);

  printf("%d %d\n", (int)sizeof(RPY_ClientAccesses_Client), (int)offsetof(CMD_Reply, data.client_accesses.clients));

  for (i=0; i<nc; i++) {
    UTI_IPNetworkToHost(&rx_message->data.client_accesses.client_ips[i], &ip);
    UTI_IPHostToNetwork(&ip, &tx_message->data.client_accesses.clients[i].ip);

    result = CLG_GetClientAccessReportByIP(&ip, &report, now.tv_sec);
    switch (result) {
      case CLG_SUCCESS:
        tx_message->data.client_accesses.clients[i].client_hits = htonl(report.client_hits);
        tx_message->data.client_accesses.clients[i].peer_hits = htonl(report.peer_hits);
        tx_message->data.client_accesses.clients[i].cmd_hits_auth = htonl(report.cmd_hits_auth);
        tx_message->data.client_accesses.clients[i].cmd_hits_normal = htonl(report.cmd_hits_normal);
        tx_message->data.client_accesses.clients[i].cmd_hits_bad = htonl(report.cmd_hits_bad);
        tx_message->data.client_accesses.clients[i].last_ntp_hit_ago = htonl(report.last_ntp_hit_ago);
        tx_message->data.client_accesses.clients[i].last_cmd_hit_ago = htonl(report.last_cmd_hit_ago);
        printf("%s %lu %lu %lu %lu %lu %lu %lu\n", UTI_IPToString(&ip), report.client_hits, report.peer_hits, report.cmd_hits_auth, report.cmd_hits_normal, report.cmd_hits_bad, report.last_ntp_hit_ago, report.last_cmd_hit_ago);
        break;
      case CLG_EMPTYSUBNET:
        /* Signal back to the client that this single client address
           was unknown */
        ip.family = IPADDR_UNSPEC;
        UTI_IPHostToNetwork(&ip, &tx_message->data.client_accesses.clients[i].ip);
        break;
      case CLG_INACTIVE:
        tx_message->status = htons(STT_INACTIVE);
        return;
      default:
        assert(0);
        break;
    }
  }

}