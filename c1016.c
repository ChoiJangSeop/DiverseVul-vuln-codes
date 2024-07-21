Opal::Call::OnEstablished (OpalConnection & connection)
{
  RTP_Session *session = NULL; 
  OpalMediaStreamPtr stream;

  NoAnswerTimer.Stop (false);

  if (!PIsDescendant(&connection, OpalPCSSConnection)) {

    parse_info (connection);
    Ekiga::Runtime::run_in_main (boost::bind (&Opal::Call::emit_established_in_main, this));
  }

  if (PIsDescendant(&connection, OpalRTPConnection)) {

    stream = connection.GetMediaStream (OpalMediaType::Audio (), false);
    if (stream != NULL) {

      session = PDownCast (OpalRTPConnection, &connection)->GetSession (stream->GetSessionID ());
      if (session) {
      
        session->SetIgnorePayloadTypeChanges (TRUE);
        session->SetRxStatisticsInterval(50);
        session->SetTxStatisticsInterval(50);
      }
    }

    stream = connection.GetMediaStream (OpalMediaType::Video (), false);
    if (stream != NULL) { 

      session = PDownCast (OpalRTPConnection, &connection)->GetSession (stream->GetSessionID ());
      if (session) {

        session->SetIgnorePayloadTypeChanges (TRUE);
        session->SetRxStatisticsInterval(50);
        session->SetTxStatisticsInterval(50);
      }
    }
  }
  
  return OpalCall::OnEstablished (connection);
}