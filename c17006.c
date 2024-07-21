int ssh_scp_close(ssh_scp scp)
{
  char buffer[128];
  int err;
  if(scp==NULL)
    return SSH_ERROR;
  if(scp->channel != NULL){
    if(ssh_channel_send_eof(scp->channel) == SSH_ERROR){
      scp->state=SSH_SCP_ERROR;
      return SSH_ERROR;
    }
    /* avoid situations where data are buffered and
     * not yet stored on disk. This can happen if the close is sent
     * before we got the EOF back
     */
    while(!ssh_channel_is_eof(scp->channel)){
      err=ssh_channel_read(scp->channel,buffer,sizeof(buffer),0);
      if(err==SSH_ERROR || err==0)
        break;
    }
    if(ssh_channel_close(scp->channel) == SSH_ERROR){
      scp->state=SSH_SCP_ERROR;
      return SSH_ERROR;
    }
    ssh_channel_free(scp->channel);
    scp->channel=NULL;
  }
  scp->state=SSH_SCP_NEW;
  return SSH_OK;
}