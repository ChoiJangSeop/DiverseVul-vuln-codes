void CZNC::ForceEncoding() {
    m_uiForceEncoding++;
#ifdef HAVE_ICU
    for (Csock* pSock : GetManager()) {
        if (pSock->GetEncoding().empty()) {
            pSock->SetEncoding("UTF-8");
        }
    }
#endif
}