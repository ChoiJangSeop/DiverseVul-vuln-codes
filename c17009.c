size_t ssh_scp_request_get_size(ssh_scp scp){
  if(scp==NULL)
      return 0;
  return (size_t)scp->filelen;
}