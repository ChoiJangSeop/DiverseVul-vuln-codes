Proto_SecurityCheckRequest(ServiceConnection *conn,
                           ProtoRequest *req)
{
   VGAuthError err;
   gboolean isSecure = ServiceNetworkIsConnectionPrivateSuperUser(conn);

   switch (req->reqType) {
      /*
       * This comes over the public connection; alwsys let it through.
       */
   case PROTO_REQUEST_SESSION_REQ:
      err = VGAUTH_E_OK;
      break;
      /*
       * No security issues with Connect or QueryMappedCerts
       */
   case PROTO_REQUEST_CONN:
   case PROTO_REQUEST_QUERYMAPPEDALIASES:
      err = VGAUTH_E_OK;
      break;
      /*
       * These request can come over any user connection; always let
       * them through if they are coming from root or the owner of
       * the certstore being changed.
       */
   case PROTO_REQUEST_ADDALIAS:
   case PROTO_REQUEST_REMOVEALIAS:
   case PROTO_REQUEST_QUERYALIASES:
   case PROTO_REQUEST_CREATETICKET:
      if (isSecure) {
         err = VGAUTH_E_OK;
      } else {
         const gchar *connOwner = conn->userName;
         const gchar *reqUser = NULL;

         if (req->reqType == PROTO_REQUEST_ADDALIAS) {
            reqUser = req->reqData.addAlias.userName;
         } else if (req->reqType == PROTO_REQUEST_REMOVEALIAS) {
            reqUser = req->reqData.removeAlias.userName;
         } else if (req->reqType == PROTO_REQUEST_QUERYALIASES) {
            reqUser = req->reqData.queryAliases.userName;
         } else if (req->reqType == PROTO_REQUEST_CREATETICKET) {
            reqUser = req->reqData.createTicket.userName;
         } else {
            ASSERT(0);
         }

         if (Usercheck_CompareByName(connOwner, reqUser)) {
            err = VGAUTH_E_OK;
         } else {
            Audit_Event(FALSE,
                        SU_(proto.attack, "Possible security attack!  Request type %d has a "
                        "userName (%s) which doesn't match the pipe owner (%s)!"),
                        req->reqType, reqUser, connOwner);
            Warning("%s: Possible security attack!  Request type %d has a "
                    "userName (%s) which doesn't match the pipe owner (%s)!\n",
                    __FUNCTION__, req->reqType, reqUser, connOwner);
            err = VGAUTH_E_PERMISSION_DENIED;
         }
      }
      break;
      /*
       * These requests must come through a super-user owned private
       * connection.
       */
   case PROTO_REQUEST_VALIDATETICKET:
      err = (isSecure) ? VGAUTH_E_OK : VGAUTH_E_PERMISSION_DENIED;
      break;
   case PROTO_REQUEST_VALIDATE_SAML_BEARER_TOKEN:
      /*
       * CAF wants to be able to validate as any user.
       */
      err = VGAUTH_E_OK;
      break;
   case PROTO_REQUEST_REVOKETICKET:
      /*
       * We want to allow just SUPERUSER and the ticket's owner to do the
       * Revoke.  But returning VGAUTH_E_PERMISSION_DENIED is also a hint
       * to an attacker that the ticket is valid.  So rather than
       * blow it off, we just ignore security at this layer,
       * and let the request fall through to ServiceRevokeTicket(),
       * which will turn a security issue into a no-op.
       */
      err = VGAUTH_E_OK;
      break;
   default:
      Warning("%s: Unrecognized request type '%d'\n",
              __FUNCTION__, req->reqType);
      err = VGAUTH_E_PERMISSION_DENIED;
      break;
   }

   return err;
}