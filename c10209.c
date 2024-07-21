lasso_saml20_login_process_response_status_and_assertion(LassoLogin *login)
{
	LassoSamlp2StatusResponse *response;
	LassoSamlp2Response *samlp2_response = NULL;
	LassoProfile *profile;
	char *status_value;
	lasso_error_t rc = 0;
	lasso_error_t assertion_signature_status = 0;
	LassoProfileSignatureVerifyHint verify_hint;

	profile = &login->parent;
	lasso_extract_node_or_fail(response, profile->response, SAMLP2_STATUS_RESPONSE,
			LASSO_PROFILE_ERROR_INVALID_MSG);
	lasso_extract_node_or_fail(samlp2_response, response, SAMLP2_RESPONSE,
			LASSO_PROFILE_ERROR_INVALID_MSG);

	if (response->Status == NULL || ! LASSO_IS_SAMLP2_STATUS(response->Status) ||
			response->Status->StatusCode == NULL ||
			response->Status->StatusCode->Value == NULL) {
		return LASSO_PROFILE_ERROR_MISSING_STATUS_CODE;
	}

	status_value = response->Status->StatusCode->Value;
	if (status_value && lasso_strisnotequal(status_value,LASSO_SAML2_STATUS_CODE_SUCCESS)) {
		if (lasso_strisequal(status_value,LASSO_SAML2_STATUS_CODE_REQUEST_DENIED))
			return LASSO_LOGIN_ERROR_REQUEST_DENIED;
		if (lasso_strisequal(status_value,LASSO_SAML2_STATUS_CODE_RESPONDER) ||
				lasso_strisequal(status_value,LASSO_SAML2_STATUS_CODE_REQUESTER)) {
			/* samlp:Responder */
			if (response->Status->StatusCode->StatusCode &&
					response->Status->StatusCode->StatusCode->Value) {
				status_value = response->Status->StatusCode->StatusCode->Value;
				if (lasso_strisequal(status_value,LASSO_LIB_STATUS_CODE_FEDERATION_DOES_NOT_EXIST)) {
					return LASSO_LOGIN_ERROR_FEDERATION_NOT_FOUND;
				}
				if (lasso_strisequal(status_value,LASSO_LIB_STATUS_CODE_UNKNOWN_PRINCIPAL)) {
					return LASSO_LOGIN_ERROR_UNKNOWN_PRINCIPAL;
				}
			}
		}
		return LASSO_LOGIN_ERROR_STATUS_NOT_SUCCESS;
	}

	/* Decrypt all EncryptedAssertions */
	_lasso_saml20_login_decrypt_assertion(login, samlp2_response);
	/* traverse all assertions */
	goto_cleanup_if_fail_with_rc (samlp2_response->Assertion != NULL,
			LASSO_PROFILE_ERROR_MISSING_ASSERTION);

	verify_hint = lasso_profile_get_signature_verify_hint(profile);

	lasso_foreach_full_begin(LassoSaml2Assertion*, assertion, it, samlp2_response->Assertion);
		LassoSaml2Subject *subject = NULL;

		lasso_assign_gobject (login->private_data->saml2_assertion, assertion);

		/* If signature has already been verified on the message, and assertion has the same
		 * issuer as the message, the assertion is covered. So no need to verify a second
		 * time */
		if (profile->signature_status != 0 
			|| ! _lasso_check_assertion_issuer(assertion,
				profile->remote_providerID)
			|| verify_hint == LASSO_PROFILE_SIGNATURE_VERIFY_HINT_FORCE) {
			assertion_signature_status = lasso_saml20_login_check_assertion_signature(login,
					assertion);
			/* If signature validation fails, it is the return code for this function */
			if (assertion_signature_status) {
				rc = LASSO_PROFILE_ERROR_CANNOT_VERIFY_SIGNATURE;
			}
		}

		lasso_extract_node_or_fail(subject, assertion->Subject, SAML2_SUBJECT,
				LASSO_PROFILE_ERROR_MISSING_SUBJECT);

		/* Verify Subject->SubjectConfirmationData->InResponseTo */
		if (login->private_data->request_id) {
			const char *in_response_to = lasso_saml2_assertion_get_in_response_to(assertion);

			if (lasso_strisnotequal(in_response_to,login->private_data->request_id)) {
				rc = LASSO_LOGIN_ERROR_ASSERTION_DOES_NOT_MATCH_REQUEST_ID;
				goto cleanup;
			}
		}

		/** Handle nameid */
		lasso_check_good_rc(lasso_saml20_profile_process_name_identifier_decryption(profile,
					&subject->NameID, &subject->EncryptedID));
	lasso_foreach_full_end();

	switch (verify_hint) {
		case LASSO_PROFILE_SIGNATURE_VERIFY_HINT_FORCE:
		case LASSO_PROFILE_SIGNATURE_VERIFY_HINT_MAYBE:
			break;
		case LASSO_PROFILE_SIGNATURE_VERIFY_HINT_IGNORE:
			/* ignore signature errors */
			if (rc == LASSO_PROFILE_ERROR_CANNOT_VERIFY_SIGNATURE) {
				rc = 0;
			}
			break;
		default:
			g_assert(0);
	}
cleanup:
	return rc;
}