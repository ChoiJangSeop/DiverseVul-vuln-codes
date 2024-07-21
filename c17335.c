CLG_LogCommandAccess(IPAddr *client, CLG_Command_Type type, time_t now)
{
  uint32_t ip6[4];
  Node *node;

  if (active) {
    switch (client->family) {
      case IPADDR_INET4:
        node = (Node *) find_subnet(&top_subnet4, &client->addr.in4, 1, 0);
        break;
      case IPADDR_INET6:
        split_ip6(client, ip6);
        node = (Node *) find_subnet(&top_subnet6, ip6, 4, 0);
        break;
      default:
        assert(0);
    }

    node->ip_addr = *client;
    node->last_cmd_hit = now;
    switch (type) {
      case CLG_CMD_AUTH:
        ++node->cmd_hits_auth;
        break;
      case CLG_CMD_NORMAL:
        ++node->cmd_hits_normal;
        break;
      case CLG_CMD_BAD_PKT:
        ++node->cmd_hits_bad;
        break;
      default:
        CROAK("Impossible");
        break;
    }
  }
}