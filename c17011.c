int ssh_scp_deny_request(ssh_scp scp, const char *reason){
  char buffer[MAX_BUF_SIZE];
  int err;
  if(scp==NULL)
      return SSH_ERROR;
  if(scp->state != SSH_SCP_READ_REQUESTED){
    ssh_set_error(scp->session,SSH_FATAL,"ssh_scp_deny_request called under invalid state");
    return SSH_ERROR;
  }
  snprintf(buffer,sizeof(buffer),"%c%s\n",2,reason);
  err=ssh_channel_write(scp->channel,buffer,strlen(buffer));
  if(err==SSH_ERROR) {
    return SSH_ERROR;
  }
  else {
    scp->state=SSH_SCP_READ_INITED;
    return SSH_OK;
  }
}