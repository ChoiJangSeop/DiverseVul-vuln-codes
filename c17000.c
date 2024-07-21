int ssh_scp_push_file64(ssh_scp scp, const char *filename, uint64_t size, int mode){
  char buffer[1024];
  int r;
  uint8_t code;
  char *file;
  char *perms;
  if(scp==NULL)
      return SSH_ERROR;
  if(scp->state != SSH_SCP_WRITE_INITED){
    ssh_set_error(scp->session,SSH_FATAL,"ssh_scp_push_file called under invalid state");
    return SSH_ERROR;
  }
  file=ssh_basename(filename);
  perms=ssh_scp_string_mode(mode);
  SSH_LOG(SSH_LOG_PROTOCOL,"SCP pushing file %s, size %" PRIu64 " with permissions '%s'",file,size,perms);
  snprintf(buffer, sizeof(buffer), "C%s %" PRIu64 " %s\n", perms, size, file);
  SAFE_FREE(file);
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
  scp->filelen = size;
  scp->processed = 0;
  scp->state=SSH_SCP_WRITE_WRITING;
  return SSH_OK;
}