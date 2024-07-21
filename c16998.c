int ssh_scp_request_get_permissions(ssh_scp scp){
  if(scp==NULL)
      return -1;
  return scp->request_mode;
}