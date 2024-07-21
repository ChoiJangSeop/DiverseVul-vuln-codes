Opal::Call::OnSetUp (OpalConnection & connection)
{
  outgoing = !IsNetworkOriginated ();
  parse_info (connection);

  Ekiga::Runtime::run_in_main (boost::bind (&Opal::Call::emit_setup_in_main, this));
  call_setup = true;

  cleared.connect (boost::bind (&Opal::Call::on_cleared_call, this, _1));
  missed.connect (boost::bind (&Opal::Call::on_missed_call, this));

  new CallSetup (*this, connection);

  return true; 
}