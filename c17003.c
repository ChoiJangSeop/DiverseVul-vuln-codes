int ssh_scp_read_string(ssh_scp scp, char *buffer, size_t len){
  size_t r=0;
  int err=SSH_OK;
  if(scp==NULL)
      return SSH_ERROR;
  while(r<len-1){
    err=ssh_channel_read(scp->channel,&buffer[r],1,0);
    if(err==SSH_ERROR){
      break;
    }
    if(err==0){
      ssh_set_error(scp->session,SSH_FATAL,"End of file while reading string");
      err=SSH_ERROR;
      break;
    }
    r++;
    if(buffer[r-1] == '\n')
      break;
  }
  buffer[r]=0;
  return err;
}