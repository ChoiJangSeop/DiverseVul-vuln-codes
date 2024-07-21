ocsp_load_response(SSL_CTX *sctx, tls_ext_ctx_cb *cbinfo, const uschar *expanded)
{
BIO *bio;
OCSP_RESPONSE *resp;
OCSP_BASICRESP *basic_response;
OCSP_SINGLERESP *single_response;
ASN1_GENERALIZEDTIME *rev, *thisupd, *nextupd;
X509_STORE *store;
unsigned long verify_flags;
int status, reason, i;

cbinfo->u_ocsp.server.file_expanded = string_copy(expanded);
if (cbinfo->u_ocsp.server.response)
  {
  OCSP_RESPONSE_free(cbinfo->u_ocsp.server.response);
  cbinfo->u_ocsp.server.response = NULL;
  }

bio = BIO_new_file(CS cbinfo->u_ocsp.server.file_expanded, "rb");
if (!bio)
  {
  DEBUG(D_tls) debug_printf("Failed to open OCSP response file \"%s\"\n",
      cbinfo->u_ocsp.server.file_expanded);
  return;
  }

resp = d2i_OCSP_RESPONSE_bio(bio, NULL);
BIO_free(bio);
if (!resp)
  {
  DEBUG(D_tls) debug_printf("Error reading OCSP response.\n");
  return;
  }

status = OCSP_response_status(resp);
if (status != OCSP_RESPONSE_STATUS_SUCCESSFUL)
  {
  DEBUG(D_tls) debug_printf("OCSP response not valid: %s (%d)\n",
      OCSP_response_status_str(status), status);
  goto bad;
  }

basic_response = OCSP_response_get1_basic(resp);
if (!basic_response)
  {
  DEBUG(D_tls)
    debug_printf("OCSP response parse error: unable to extract basic response.\n");
  goto bad;
  }

store = SSL_CTX_get_cert_store(sctx);
verify_flags = OCSP_NOVERIFY; /* check sigs, but not purpose */

/* May need to expose ability to adjust those flags?
OCSP_NOSIGS OCSP_NOVERIFY OCSP_NOCHAIN OCSP_NOCHECKS OCSP_NOEXPLICIT
OCSP_TRUSTOTHER OCSP_NOINTERN */

i = OCSP_basic_verify(basic_response, NULL, store, verify_flags);
if (i <= 0)
  {
  DEBUG(D_tls) {
    ERR_error_string(ERR_get_error(), ssl_errstring);
    debug_printf("OCSP response verify failure: %s\n", US ssl_errstring);
    }
  goto bad;
  }

/* Here's the simplifying assumption: there's only one response, for the
one certificate we use, and nothing for anything else in a chain.  If this
proves false, we need to extract a cert id from our issued cert
(tls_certificate) and use that for OCSP_resp_find_status() (which finds the
right cert in the stack and then calls OCSP_single_get0_status()).

I'm hoping to avoid reworking a bunch more of how we handle state here. */
single_response = OCSP_resp_get0(basic_response, 0);
if (!single_response)
  {
  DEBUG(D_tls)
    debug_printf("Unable to get first response from OCSP basic response.\n");
  goto bad;
  }

status = OCSP_single_get0_status(single_response, &reason, &rev, &thisupd, &nextupd);
if (status != V_OCSP_CERTSTATUS_GOOD)
  {
  DEBUG(D_tls) debug_printf("OCSP response bad cert status: %s (%d) %s (%d)\n",
      OCSP_cert_status_str(status), status,
      OCSP_crl_reason_str(reason), reason);
  goto bad;
  }

if (!OCSP_check_validity(thisupd, nextupd, EXIM_OCSP_SKEW_SECONDS, EXIM_OCSP_MAX_AGE))
  {
  DEBUG(D_tls) debug_printf("OCSP status invalid times.\n");
  goto bad;
  }

supply_response:
  cbinfo->u_ocsp.server.response = resp;
return;

bad:
  if (running_in_test_harness)
    {
    extern char ** environ;
    uschar ** p;
    for (p = USS environ; *p != NULL; p++)
      if (Ustrncmp(*p, "EXIM_TESTHARNESS_DISABLE_OCSPVALIDITYCHECK", 42) == 0)
	{
	DEBUG(D_tls) debug_printf("Supplying known bad OCSP response\n");
	goto supply_response;
	}
    }
return;
}