int ssh_scp_read(ssh_scp scp, void *buffer, size_t size){
  int r;
  int code;
  if(scp==NULL)
      return SSH_ERROR;
  if(scp->state == SSH_SCP_READ_REQUESTED && scp->request_type == SSH_SCP_REQUEST_NEWFILE){
    r=ssh_scp_accept_request(scp);
    if(r==SSH_ERROR)
      return r;
  }
  if(scp->state != SSH_SCP_READ_READING){
    ssh_set_error(scp->session,SSH_FATAL,"ssh_scp_read called under invalid state");
    return SSH_ERROR;
  }
  if(scp->processed + size > scp->filelen)
    size = (size_t) (scp->filelen - scp->processed);
  if(size > 65536)
    size=65536; /* avoid too large reads */
  r=ssh_channel_read(scp->channel,buffer,size,0);
  if(r != SSH_ERROR)
    scp->processed += r;
  else {
    scp->state=SSH_SCP_ERROR;
    return SSH_ERROR;
  }
  /* Check if we arrived at end of file */
  if(scp->processed == scp->filelen) {
    scp->processed=scp->filelen=0;
    ssh_channel_write(scp->channel,"",1);
    code=ssh_scp_response(scp,NULL);
    if(code == 0){
    	scp->state=SSH_SCP_READ_INITED;
    	return r;
    }
    if(code==1){
    	scp->state=SSH_SCP_READ_INITED;
    	return SSH_ERROR;
    }
    scp->state=SSH_SCP_ERROR;
    return SSH_ERROR;
  }
  return r;
}