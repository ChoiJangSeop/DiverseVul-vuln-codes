void CIRCSock::ReadLine(const CString& sData) {
    CString sLine = sData;

    sLine.TrimRight("\n\r");

    DEBUG("(" << m_pNetwork->GetUser()->GetUserName() << "/"
              << m_pNetwork->GetName() << ") IRC -> ZNC [" << sLine << "]");

    bool bReturn = false;
    IRCSOCKMODULECALL(OnRaw(sLine), &bReturn);
    if (bReturn) return;

    CMessage Message(sLine);
    Message.SetNetwork(m_pNetwork);

    IRCSOCKMODULECALL(OnRawMessage(Message), &bReturn);
    if (bReturn) return;

    switch (Message.GetType()) {
        case CMessage::Type::Account:
            bReturn = OnAccountMessage(Message);
            break;
        case CMessage::Type::Action:
            bReturn = OnActionMessage(Message);
            break;
        case CMessage::Type::Away:
            bReturn = OnAwayMessage(Message);
            break;
        case CMessage::Type::Capability:
            bReturn = OnCapabilityMessage(Message);
            break;
        case CMessage::Type::CTCP:
            bReturn = OnCTCPMessage(Message);
            break;
        case CMessage::Type::Error:
            bReturn = OnErrorMessage(Message);
            break;
        case CMessage::Type::Invite:
            bReturn = OnInviteMessage(Message);
            break;
        case CMessage::Type::Join:
            bReturn = OnJoinMessage(Message);
            break;
        case CMessage::Type::Kick:
            bReturn = OnKickMessage(Message);
            break;
        case CMessage::Type::Mode:
            bReturn = OnModeMessage(Message);
            break;
        case CMessage::Type::Nick:
            bReturn = OnNickMessage(Message);
            break;
        case CMessage::Type::Notice:
            bReturn = OnNoticeMessage(Message);
            break;
        case CMessage::Type::Numeric:
            bReturn = OnNumericMessage(Message);
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
        case CMessage::Type::Wallops:
            bReturn = OnWallopsMessage(Message);
            break;
        default:
            break;
    }
    if (bReturn) return;

    m_pNetwork->PutUser(Message);
}