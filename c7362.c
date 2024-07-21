void CIRCNetwork::SetEncoding(const CString& s) {
    m_sEncoding = s;
    if (GetIRCSock()) {
        GetIRCSock()->SetEncoding(s);
    }
}