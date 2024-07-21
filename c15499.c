de265_error decoder_context::read_sps_NAL(bitreader& reader)
{
  logdebug(LogHeaders,"----> read SPS\n");

  std::shared_ptr<seq_parameter_set> new_sps = std::make_shared<seq_parameter_set>();
  de265_error err;

  if ((err=new_sps->read(this, &reader)) != DE265_OK) {
    return err;
  }

  if (param_sps_headers_fd>=0) {
    new_sps->dump(param_sps_headers_fd);
  }

  sps[ new_sps->seq_parameter_set_id ] = new_sps;

  return DE265_OK;
}