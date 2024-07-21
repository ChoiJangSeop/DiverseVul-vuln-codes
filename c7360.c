void CUser::SetClientEncoding(const CString& s) {
    m_sClientEncoding = s;
    for (CClient* pClient : GetAllClients()) {
        pClient->SetEncoding(s);
    }
}