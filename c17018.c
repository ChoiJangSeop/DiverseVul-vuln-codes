uint64_t ssh_scp_request_get_size64(ssh_scp scp){
  if(scp==NULL)
      return 0;
  return scp->filelen;
}