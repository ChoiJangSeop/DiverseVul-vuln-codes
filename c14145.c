void CClient::ReadLine(const CString& sData) {
    CLanguageScope user_lang(GetUser() ? GetUser()->GetLanguage() : "");
    CString sLine = sData;

    sLine.TrimRight("\n\r");

    DEBUG("(" << GetFullName() << ") CLI -> ZNC ["
        << CDebug::Filter(sLine) << "]");

    bool bReturn = false;
    if (IsAttached()) {
        NETWORKMODULECALL(OnUserRaw(sLine), m_pUser, m_pNetwork, this,
                          &bReturn);
    } else {
        GLOBALMODULECALL(OnUnknownUserRaw(this, sLine), &bReturn);
    }
    if (bReturn) return;

    CMessage Message(sLine);
    Message.SetClient(this);

    if (IsAttached()) {
        NETWORKMODULECALL(OnUserRawMessage(Message), m_pUser, m_pNetwork, this,
                          &bReturn);
    } else {
        GLOBALMODULECALL(OnUnknownUserRawMessage(Message), &bReturn);
    }
    if (bReturn) return;

    CString sCommand = Message.GetCommand();

    if (!IsAttached()) {
        // The following commands happen before authentication with ZNC
        if (sCommand.Equals("PASS")) {
            m_bGotPass = true;

            CString sAuthLine = Message.GetParam(0);
            ParsePass(sAuthLine);

            AuthUser();
            // Don't forward this msg.  ZNC has already registered us.
            return;
        } else if (sCommand.Equals("NICK")) {
            CString sNick = Message.GetParam(0);

            m_sNick = sNick;
            m_bGotNick = true;

            AuthUser();
            // Don't forward this msg.  ZNC will handle nick changes until auth
            // is complete
            return;
        } else if (sCommand.Equals("USER")) {
            CString sAuthLine = Message.GetParam(0);

            if (m_sUser.empty() && !sAuthLine.empty()) {
                ParseUser(sAuthLine);
            }

            m_bGotUser = true;
            if (m_bGotPass) {
                AuthUser();
            } else if (!m_bInCap) {
                SendRequiredPasswordNotice();
            }

            // Don't forward this msg.  ZNC has already registered us.
            return;
        }
    }

    if (Message.GetType() == CMessage::Type::Capability) {
        HandleCap(Message);

        // Don't let the client talk to the server directly about CAP,
        // we don't want anything enabled that ZNC does not support.
        return;
    }

    if (!m_pUser) {
        // Only CAP, NICK, USER and PASS are allowed before login
        return;
    }

    switch (Message.GetType()) {
        case CMessage::Type::Action:
            bReturn = OnActionMessage(Message);
            break;
        case CMessage::Type::CTCP:
            bReturn = OnCTCPMessage(Message);
            break;
        case CMessage::Type::Join:
            bReturn = OnJoinMessage(Message);
            break;
        case CMessage::Type::Mode:
            bReturn = OnModeMessage(Message);
            break;
        case CMessage::Type::Notice:
            bReturn = OnNoticeMessage(Message);
            break;
        case CMessage::Type::Part:
            bReturn = OnPartMessage(Message);
            break;
        case CMessage::Type::Ping:
            bReturn = OnPingMessage(Message);
            break;
        case CMessage::Type::Pong:
            bReturn = OnPongMessage(Message);
            break;
        case CMessage::Type::Quit:
            bReturn = OnQuitMessage(Message);
            break;
        case CMessage::Type::Text:
            bReturn = OnTextMessage(Message);
            break;
        case CMessage::Type::Topic:
            bReturn = OnTopicMessage(Message);
            break;
        default:
            bReturn = OnOtherMessage(Message);
            break;
    }

    if (bReturn) return;

    PutIRC(Message.ToString(CMessage::ExcludePrefix | CMessage::ExcludeTags));
}