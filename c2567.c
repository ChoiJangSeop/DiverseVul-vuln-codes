int handle_request(struct ipa_extdom_ctx *ctx, struct extdom_req *req,
                   struct berval **berval)
{
    int ret;

    switch (req->input_type) {
    case INP_POSIX_UID:
        ret = handle_uid_request(req->request_type, req->data.posix_uid.uid,
                                 req->data.posix_uid.domain_name, berval);

        break;
    case INP_POSIX_GID:
        ret = handle_gid_request(req->request_type, req->data.posix_gid.gid,
                                 req->data.posix_uid.domain_name, berval);

        break;
    case INP_SID:
        ret = handle_sid_request(req->request_type, req->data.sid, berval);
        break;
    case INP_NAME:
        ret = handle_name_request(req->request_type, req->data.name.object_name,
                                  req->data.name.domain_name, berval);

        break;
    default:
        ret = LDAP_PROTOCOL_ERROR;
        goto done;
    }


done:

    return ret;
}