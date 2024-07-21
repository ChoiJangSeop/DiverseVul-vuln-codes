int ssh_scp_init(ssh_scp scp)
{
  int r;
  char execbuffer[1024];
  uint8_t code;
  if(scp==NULL)
      return SSH_ERROR;
  if(scp->state != SSH_SCP_NEW){
    ssh_set_error(scp->session,SSH_FATAL,"ssh_scp_init called under invalid state");
    return SSH_ERROR;
  }
  SSH_LOG(SSH_LOG_PROTOCOL,"Initializing scp session %s %son location '%s'",
		  scp->mode==SSH_SCP_WRITE?"write":"read",
				  scp->recursive?"recursive ":"",
						  scp->location);
  scp->channel=ssh_channel_new(scp->session);
  if(scp->channel == NULL){
    scp->state=SSH_SCP_ERROR;
    return SSH_ERROR;
  }
  r= ssh_channel_open_session(scp->channel);
  if(r==SSH_ERROR){
    scp->state=SSH_SCP_ERROR;
    return SSH_ERROR;
  }
  if(scp->mode == SSH_SCP_WRITE)
    snprintf(execbuffer,sizeof(execbuffer),"scp -t %s %s",
    		scp->recursive ? "-r":"", scp->location);
  else
    snprintf(execbuffer,sizeof(execbuffer),"scp -f %s %s",
    		scp->recursive ? "-r":"", scp->location);
  if(ssh_channel_request_exec(scp->channel,execbuffer) == SSH_ERROR){
    scp->state=SSH_SCP_ERROR;
    return SSH_ERROR;
  }
  if(scp->mode == SSH_SCP_WRITE){
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
  } else {
	  ssh_channel_write(scp->channel,"",1);
  }
  if(scp->mode == SSH_SCP_WRITE)
    scp->state=SSH_SCP_WRITE_INITED;
  else
    scp->state=SSH_SCP_READ_INITED;
  return SSH_OK;
}