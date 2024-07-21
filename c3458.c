void CertDecoder::GetName(NameType nt)
{
    if (source_.GetError().What()) return;

    SHA    sha;
    word32 length = GetSequence();  // length of all distinguished names

    if (length >= ASN_NAME_MAX)
        return;
    if (source_.IsLeft(length) == false) return;
    length += source_.get_index();
    
    char* ptr;
    char* buf_end;

    if (nt == ISSUER) {
        ptr = issuer_;
        buf_end = ptr + sizeof(issuer_) - 1;   // 1 byte for trailing 0
    }
    else {
        ptr = subject_;
        buf_end = ptr + sizeof(subject_) - 1;  // 1 byte for trailing 0
    }

    while (source_.get_index() < length) {
        GetSet();
        if (source_.GetError().What() == SET_E) {
            source_.SetError(NO_ERROR_E);  // extensions may only have sequence 
            source_.prev();
        }
        GetSequence();

        byte b = source_.next();
        if (b != OBJECT_IDENTIFIER) {
            source_.SetError(OBJECT_ID_E);
            return;
        }

        word32 oidSz = GetLength(source_);
        if (source_.IsLeft(oidSz) == false) return;

        byte joint[2];
        if (source_.IsLeft(sizeof(joint)) == false) return;
        memcpy(joint, source_.get_current(), sizeof(joint));

        // v1 name types
        if (joint[0] == 0x55 && joint[1] == 0x04) {
            source_.advance(2);
            byte   id      = source_.next();  
            b              = source_.next();    // strType
            word32 strLen  = GetLength(source_);

            if (source_.IsLeft(strLen) == false) return;

            switch (id) {
            case COMMON_NAME:
                if (!(ptr = AddTag(ptr, buf_end, "/CN=", 4, strLen)))
                    return;
                break;
            case SUR_NAME:
                if (!(ptr = AddTag(ptr, buf_end, "/SN=", 4, strLen)))
                    return;
                break;
            case COUNTRY_NAME:
                if (!(ptr = AddTag(ptr, buf_end, "/C=", 3, strLen)))
                    return;
                break;
            case LOCALITY_NAME:
                if (!(ptr = AddTag(ptr, buf_end, "/L=", 3, strLen)))
                    return;
                break;
            case STATE_NAME:
                if (!(ptr = AddTag(ptr, buf_end, "/ST=", 4, strLen)))
                    return;
                break;
            case ORG_NAME:
                if (!(ptr = AddTag(ptr, buf_end, "/O=", 3, strLen)))
                    return;
                break;
            case ORGUNIT_NAME:
                if (!(ptr = AddTag(ptr, buf_end, "/OU=", 4, strLen)))
                    return;
                break;
            }

            sha.Update(source_.get_current(), strLen);
            source_.advance(strLen);
        }
        else { 
            bool email = false;
            if (joint[0] == 0x2a && joint[1] == 0x86)  // email id hdr
                email = true;

            source_.advance(oidSz + 1);
            word32 length = GetLength(source_);
            if (source_.IsLeft(length) == false) return;

            if (email) {
                if (!(ptr = AddTag(ptr, buf_end, "/emailAddress=", 14, length)))
                    return; 
            }

            source_.advance(length);
        }
    }

    *ptr = 0;

    if (nt == ISSUER)
        sha.Final(issuerHash_);
    else
        sha.Final(subjectHash_);
}