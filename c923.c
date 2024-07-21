handle_subnets_accessed(CMD_Request *rx_message, CMD_Reply *tx_message)
{
  int i, j;
  unsigned long ns, bits_specd;
  IPAddr ip;
  CLG_Status result;
  
  ns = ntohl(rx_message->data.subnets_accessed.n_subnets);
  tx_message->status = htons(STT_SUCCESS);
  tx_message->reply = htons(RPY_SUBNETS_ACCESSED);
  tx_message->data.subnets_accessed.n_subnets = htonl(ns);

  for (i=0; i<ns; i++) {
    UTI_IPNetworkToHost(&rx_message->data.subnets_accessed.subnets[i].ip, &ip);
    bits_specd = ntohl(rx_message->data.subnets_accessed.subnets[i].bits_specd);

    UTI_IPHostToNetwork(&ip, &tx_message->data.subnets_accessed.subnets[i].ip);
    tx_message->data.subnets_accessed.subnets[i].bits_specd = htonl(bits_specd);
    
    result = CLG_GetSubnetBitmap(&ip, bits_specd, tx_message->data.subnets_accessed.subnets[i].bitmap);
    switch (result) {
      case CLG_SUCCESS:
      case CLG_EMPTYSUBNET:
        /* Flip endianness of each 4 byte word.  Don't care if subnet
           is empty - just return an all-zero bitmap. */
        for (j=0; j<8; j++) {
          FLIPL(tx_message->data.subnets_accessed.subnets[i].bitmap[j]);
        }
        break;
      case CLG_BADSUBNET:
        tx_message->status = htons(STT_BADSUBNET);
        return;
      case CLG_INACTIVE:
        tx_message->status = htons(STT_INACTIVE);
        return;
      default:
        assert(0);
        break;
    }
  }
}