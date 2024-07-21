int ssh_scp_push_directory(ssh_scp scp, const char *dirname, int mode){
  char buffer[1024];
  int r;
  uint8_t code;
  char *dir;
  char *perms;
  if(scp==NULL)
      return SSH_ERROR;
  if(scp->state != SSH_SCP_WRITE_INITED){
    ssh_set_error(scp->session,SSH_FATAL,"ssh_scp_push_directory called under invalid state");
    return SSH_ERROR;
  }
  dir=ssh_basename(dirname);
  perms=ssh_scp_string_mode(mode);
  snprintf(buffer, sizeof(buffer), "D%s 0 %s\n", perms, dir);
  SAFE_FREE(dir);
  SAFE_FREE(perms);
  r=ssh_channel_write(scp->channel,buffer,strlen(buffer));
  if(r==SSH_ERROR){
    scp->state=SSH_SCP_ERROR;
    return SSH_ERROR;
  }
  r=ssh_channel_read(scp->channel,&code,1,0);
  if(r<=0){
    ssh_set_error(scp->session,SSH_FATAL, "Error reading status code: %s",ssh_get_error(scp->session));
    scp->state=SSH_SCP_ERROR;
    return SSH_ERROR;
  }
  if(code != 0){
    ssh_set_error(scp->session,SSH_FATAL, "scp status code %ud not valid", code);
    scp->state=SSH_SCP_ERROR;
    return SSH_ERROR;
  }
  return SSH_OK;
}