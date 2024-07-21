int ssh_scp_pull_request(ssh_scp scp){
  char buffer[MAX_BUF_SIZE] = {0};
  char *mode=NULL;
  char *p,*tmp;
  uint64_t size;
  char *name=NULL;
  int err;
  if(scp==NULL)
      return SSH_ERROR;
  if(scp->state != SSH_SCP_READ_INITED){
    ssh_set_error(scp->session,SSH_FATAL,"ssh_scp_pull_request called under invalid state");
    return SSH_ERROR;
  }
  err=ssh_scp_read_string(scp,buffer,sizeof(buffer));
  if(err==SSH_ERROR){
	if(ssh_channel_is_eof(scp->channel)){
		scp->state=SSH_SCP_TERMINATED;
		return SSH_SCP_REQUEST_EOF;
	}
    return err;
  }
  p=strchr(buffer,'\n');
  if(p!=NULL)
	  *p='\0';
  SSH_LOG(SSH_LOG_PROTOCOL,"Received SCP request: '%s'",buffer);
  switch(buffer[0]){
    case 'C':
      /* File */
    case 'D':
      /* Directory */
      p=strchr(buffer,' ');
      if(p==NULL)
        goto error;
      *p='\0';
      p++;
      //mode=strdup(&buffer[1]);
      scp->request_mode=ssh_scp_integer_mode(&buffer[1]);
      tmp=p;
      p=strchr(p,' ');
      if(p==NULL)
        goto error;
      *p=0;
      size = strtoull(tmp,NULL,10);
      p++;
      name=strdup(p);
      SAFE_FREE(scp->request_name);
      scp->request_name=name;
      if(buffer[0]=='C'){
        scp->filelen=size;
        scp->request_type=SSH_SCP_REQUEST_NEWFILE;
      } else {
        scp->filelen='0';
        scp->request_type=SSH_SCP_REQUEST_NEWDIR;
      }
      scp->state=SSH_SCP_READ_REQUESTED;
      scp->processed = 0;
      return scp->request_type;
      break;
    case 'E':
    	scp->request_type=SSH_SCP_REQUEST_ENDDIR;
    	ssh_channel_write(scp->channel,"",1);
    	return scp->request_type;
    case 0x1:
    	ssh_set_error(scp->session,SSH_REQUEST_DENIED,"SCP: Warning: %s",&buffer[1]);
    	scp->request_type=SSH_SCP_REQUEST_WARNING;
    	SAFE_FREE(scp->warning);
    	scp->warning=strdup(&buffer[1]);
    	return scp->request_type;
    case 0x2:
    	ssh_set_error(scp->session,SSH_FATAL,"SCP: Error: %s",&buffer[1]);
    	return SSH_ERROR;
    case 'T':
      /* Timestamp */
    default:
      ssh_set_error(scp->session,SSH_FATAL,"Unhandled message: (%d)%s",buffer[0],buffer);
      return SSH_ERROR;
  }

  /* a parsing error occured */
  error:
  SAFE_FREE(name);
  SAFE_FREE(mode);
  ssh_set_error(scp->session,SSH_FATAL,"Parsing error while parsing message: %s",buffer);
  return SSH_ERROR;
}