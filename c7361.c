CString CZNC::FixupEncoding(const CString& sEncoding) const {
    if (sEncoding.empty() && m_uiForceEncoding) {
        return "UTF-8";
    }
    return sEncoding;
}