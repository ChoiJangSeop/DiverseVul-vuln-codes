Opal::Call::parse_info (OpalConnection & connection)
{
  char start_special_chars [] = "$";
  char end_special_chars [] = "([;=";
  
  std::string l_party_name;
  std::string r_party_name;
  std::string app;

  if (!PIsDescendant(&connection, OpalPCSSConnection)) {

    remote_uri = (const char *) connection.GetRemotePartyAddress ();

    l_party_name = (const char *) connection.GetLocalPartyName ();
    r_party_name = (const char *) connection.GetRemotePartyName ();
    app = (const char *) connection.GetRemoteProductInfo ().AsString ();
    start_time = connection.GetConnectionStartTime ();
    if (!start_time.IsValid ())
      start_time = PTime ();

    if (!l_party_name.empty ())
      local_party_name = (const char *) SIPURL (l_party_name).GetUserName ();
    if (!r_party_name.empty ())
      remote_party_name = r_party_name;
    if (!app.empty ())
      remote_application = app;
    
    strip_special_chars (remote_party_name, end_special_chars, false);
    strip_special_chars (remote_application, end_special_chars, false);
    strip_special_chars (remote_uri, end_special_chars, false);

    strip_special_chars (remote_party_name, start_special_chars, true);
    strip_special_chars (remote_uri, start_special_chars, true);
  }
}