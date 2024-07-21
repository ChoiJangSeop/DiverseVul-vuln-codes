const char *ssh_scp_request_get_warning(ssh_scp scp){
  if(scp==NULL)
      return NULL;
	return scp->warning;
}