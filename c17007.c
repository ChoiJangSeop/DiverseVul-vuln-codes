void ssh_scp_free(ssh_scp scp)
{
  if(scp==NULL)
      return;
  if(scp->state != SSH_SCP_NEW)
    ssh_scp_close(scp);
  if(scp->channel)
    ssh_channel_free(scp->channel);
  SAFE_FREE(scp->location);
  SAFE_FREE(scp->request_name);
  SAFE_FREE(scp->warning);
  SAFE_FREE(scp);
}