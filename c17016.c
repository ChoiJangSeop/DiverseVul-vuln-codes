const char *ssh_scp_request_get_filename(ssh_scp scp){
  if(scp==NULL)
      return NULL;
  return scp->request_name;
}