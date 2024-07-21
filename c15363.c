static int check_assertion(struct config_module * config, json_t * j_params, const char * username, json_t * j_scheme_data, json_t * j_assertion) {
  int ret, res;
  unsigned char * client_data = NULL, * challenge_b64 = NULL, * auth_data = NULL, rpid_hash[32] = {0}, * flags, cdata_hash[32] = {0},
                  data_signed[128] = {0}, sig[128] = {0}, * counter;
  char * challenge_hash = NULL;
  const char * rpid = NULL;
  size_t client_data_len, challenge_b64_len, auth_data_len, rpid_hash_len = 32, cdata_hash_len = 32, sig_len = 128, counter_value = 0, rpid_len = 0;
  json_t * j_client_data = NULL, * j_credential = NULL, * j_query;
  gnutls_pubkey_t pubkey = NULL;
  gnutls_datum_t pubkey_dat, data, signature;

  if (j_scheme_data != NULL && j_assertion != NULL) {
    do {
      ret = G_OK;

      if (!json_is_string(json_object_get(json_object_get(j_scheme_data, "credential"), "rawId")) || !json_string_length(json_object_get(json_object_get(j_scheme_data, "credential"), "rawId"))) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - rawId missing");
        ret = G_ERROR_PARAM;
        break;
      }
      j_credential = get_credential(config, j_params, username, json_string_value(json_object_get(json_object_get(j_scheme_data, "credential"), "rawId")));
      if (check_result_value(j_credential, G_ERROR_NOT_FOUND)) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - credential ID not found");
        ret = G_ERROR_UNAUTHORIZED;
        break;
      }
      if (!json_is_string(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "clientDataJSON")) || !json_string_length(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "clientDataJSON"))) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - clientDataJSON mandatory");
        ret = G_ERROR_PARAM;
        break;
      }
      if ((client_data = o_malloc(json_string_length(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "clientDataJSON"))+1)) == NULL) {
        y_log_message(Y_LOG_LEVEL_ERROR, "check_assertion - Error allocating resources for client_data");
        ret = G_ERROR_MEMORY;
        break;
      }
      if (!o_base64_decode((const unsigned char *)json_string_value(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "clientDataJSON")), json_string_length(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "clientDataJSON")), client_data, &client_data_len)) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - Error o_base64_decode client_data");
        ret = G_ERROR_PARAM;
        break;
      }
      client_data[client_data_len] = '\0';
      j_client_data = json_loads((const char *)client_data, JSON_DECODE_ANY, NULL);
      if (j_client_data == NULL) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - Error parsing JSON client data %s", client_data);
        ret = G_ERROR_PARAM;
        break;
      }
      // Step 7
      if (0 != o_strcmp("webauthn.get", json_string_value(json_object_get(j_client_data, "type")))) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - clientDataJSON.type invalid");
        ret = G_ERROR_PARAM;
        break;
      }
      // Step 8
      if (!json_string_length(json_object_get(j_client_data, "challenge"))) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - clientDataJSON.challenge mandatory");
        ret = G_ERROR_PARAM;
        break;
      }
      if ((challenge_b64 = o_malloc(json_string_length(json_object_get(j_client_data, "challenge"))+3)) == NULL) {
        y_log_message(Y_LOG_LEVEL_ERROR, "register_new_attestation - Error allocating resources for challenge_b64");
        ret = G_ERROR_MEMORY;
        break;
      }
      if (!o_base64url_2_base64((unsigned char *)json_string_value(json_object_get(j_client_data, "challenge")), json_string_length(json_object_get(j_client_data, "challenge")), challenge_b64, &challenge_b64_len)) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - clientDataJSON.challenge invalid base64");
        ret = G_ERROR_PARAM;
        break;
      }
      challenge_b64[challenge_b64_len] = '\0';
      if ((challenge_hash = generate_hash(config->hash_algorithm, (const char *)challenge_b64)) == NULL) {
        y_log_message(Y_LOG_LEVEL_ERROR, "register_new_attestation - Error generate_hash for challenge_b64");
        ret = G_ERROR;
        break;
      }
      if (0 != o_strcmp(challenge_hash, json_string_value(json_object_get(j_assertion, "challenge_hash")))) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - clientDataJSON.challenge invalid");
        ret = G_ERROR_PARAM;
        break;
      }
      // Step 9
      if (!json_string_length(json_object_get(j_client_data, "origin"))) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - clientDataJSON.origin mandatory");
        ret = G_ERROR_PARAM;
        break;
      }
      if (0 != o_strcmp(json_string_value(json_object_get(j_params, "rp-origin")), json_string_value(json_object_get(j_client_data, "origin")))) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - clientDataJSON.origin invalid - Client send %s, required %s", json_string_value(json_object_get(j_client_data, "origin")), json_string_value(json_object_get(j_params, "rp-origin")));
        ret = G_ERROR_PARAM;
        break;
      }
      // Step 10 ??

      // Step 11
      if (!json_string_length(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "authenticatorData"))) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - authenticatorData mandatory");
        ret = G_ERROR_PARAM;
        break;
      }
      if ((auth_data = o_malloc(json_string_length(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "authenticatorData"))+1)) == NULL) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - Error allocating resources for auth_data");
        ret = G_ERROR_PARAM;
        break;
      }
      if (!o_base64_decode((const unsigned char *)json_string_value(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "authenticatorData")), json_string_length(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "authenticatorData")), auth_data, &auth_data_len)) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - Error o_base64_decode auth_data");
        ret = G_ERROR_PARAM;
        break;
      }
      if (auth_data_len < 37) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - Error authenticatorData invalid");
        ret = G_ERROR_PARAM;
        break;
      }

      if (o_strstr(json_string_value(json_object_get(j_params, "rp-origin")), "://") != NULL) {
        rpid = o_strstr(json_string_value(json_object_get(j_params, "rp-origin")), "://")+3;
      } else {
        rpid = json_string_value(json_object_get(j_params, "rp-origin"));
      }
      if (o_strchr(rpid, ':') != NULL) {
        rpid_len = o_strchr(rpid, ':') - rpid;
      } else {
        rpid_len = o_strlen(rpid);
      }

      if (!generate_digest_raw(digest_SHA256, (unsigned char *)rpid, rpid_len, rpid_hash, &rpid_hash_len)) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - Error generate_digest_raw for rpid_hash");
        ret = G_ERROR_PARAM;
        break;
      }

      if (0 != memcmp(auth_data, rpid_hash, rpid_hash_len)) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - authData.rpIdHash invalid");
        ret = G_ERROR_PARAM;
        break;
      }
      flags = auth_data + FLAGS_OFFSET;

      // Step 12
      if (!(*flags & FLAG_USER_PRESENT)) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - authData.userPresent not set");
        ret = G_ERROR_PARAM;
        break;
      }

      // Step 13 ignored for now
      //y_log_message(Y_LOG_LEVEL_DEBUG, "authData.userVerified: %d", !!(*flags & FLAG_USER_VERIFY));

      // Step 14 ignored for now (no extension)
      //y_log_message(Y_LOG_LEVEL_DEBUG, "authData.Extension: %d", !!(*flags & FLAG_ED));

      // Step 15
      if (!generate_digest_raw(digest_SHA256, client_data, client_data_len, cdata_hash, &cdata_hash_len)) {
        y_log_message(Y_LOG_LEVEL_ERROR, "check_assertion - Error generate_digest_raw for cdata_hash");
        ret = G_ERROR_PARAM;
        break;
      }
      counter = auth_data + COUNTER_OFFSET;
      counter_value = counter[3] | (counter[2] << 8) | (counter[1] << 16) | (counter[0] << 24);

      if (gnutls_pubkey_init(&pubkey) < 0) {
        y_log_message(Y_LOG_LEVEL_ERROR, "check_assertion - Error gnutls_pubkey_init");
        ret = G_ERROR;
        break;
      }
      pubkey_dat.data = (unsigned char *)json_string_value(json_object_get(json_object_get(j_credential, "credential"), "public_key"));
      pubkey_dat.size = json_string_length(json_object_get(json_object_get(j_credential, "credential"), "public_key"));
      if ((ret = gnutls_pubkey_import(pubkey, &pubkey_dat, GNUTLS_X509_FMT_PEM)) < 0) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - Error gnutls_pubkey_import: %d", ret);
        ret = G_ERROR;
        break;
      }

      if (!o_base64url_decode((const unsigned char *)json_string_value(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "signature")), json_string_length(json_object_get(json_object_get(json_object_get(j_scheme_data, "credential"), "response"), "signature")), sig, &sig_len)) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - Error o_base64url_decode signature");
        ret = G_ERROR_PARAM;
        break;
      }

      memcpy(data_signed, auth_data, auth_data_len);
      memcpy(data_signed+auth_data_len, cdata_hash, cdata_hash_len);

      // Let's verify sig over data_signed
      data.data = data_signed;
      data.size = (auth_data_len+cdata_hash_len);

      signature.data = sig;
      signature.size = sig_len;

      if ((res = gnutls_pubkey_verify_data2(pubkey, GNUTLS_SIGN_ECDSA_SHA256, 0, &data, &signature)) < 0) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - Invalid signature: %d", res);
        ret = G_ERROR_UNAUTHORIZED;
        break;
      }

      if ((json_integer_value(json_object_get(json_object_get(j_credential, "credential"), "counter")) || counter_value) && counter_value <= (size_t)json_integer_value(json_object_get(json_object_get(j_credential, "credential"), "counter"))) {
        y_log_message(Y_LOG_LEVEL_DEBUG, "check_assertion - counter invalid");
        ret = G_ERROR_UNAUTHORIZED;
        break;
      }
    } while (0); // This is not a loop, but a structure where you can easily cancel the rest of the process with breaks

    if (ret == G_OK) {
      // Update assertion
      j_query = json_pack("{sss{sisi}s{sO}}",
                          "table",
                          G_TABLE_WEBAUTHN_ASSERTION,
                          "set",
                            "gswa_counter",
                            counter_value,
                            "gswa_status",
                            1,
                          "where",
                            "gswa_id",
                            json_object_get(j_assertion, "gswa_id"));
      res = h_update(config->conn, j_query, NULL);
      json_decref(j_query);
      if (res != H_OK) {
        y_log_message(Y_LOG_LEVEL_ERROR, "check_assertion - Error executing j_query (1)");
        config->glewlwyd_module_callback_metrics_increment_counter(config, GLWD_METRICS_DATABSE_ERROR, 1, NULL);
        ret = G_ERROR_DB;
      } else {
        // Update counter in credential if necessary
        if (counter) {
          j_query = json_pack("{sss{si}s{sO}}",
                              "table",
                              G_TABLE_WEBAUTHN_CREDENTIAL,
                              "set",
                                "gswc_counter",
                                counter_value,
                              "where",
                                "gswc_id",
                                json_object_get(json_object_get(j_credential, "credential"), "gswc_id"));
          res = h_update(config->conn, j_query, NULL);
          json_decref(j_query);
          if (res != H_OK) {
            y_log_message(Y_LOG_LEVEL_ERROR, "check_assertion - Error executing j_query (2)");
            config->glewlwyd_module_callback_metrics_increment_counter(config, GLWD_METRICS_DATABSE_ERROR, 1, NULL);
            ret = G_ERROR_DB;
          }
        }
      }
    } else if (ret == G_ERROR_PARAM) {
      j_query = json_pack("{sss{sisi}s{sO}}",
                          "table",
                          G_TABLE_WEBAUTHN_ASSERTION,
                          "set",
                            "gswa_counter",
                            counter_value,
                            "gswa_status",
                            2,
                          "where",
                            "gswa_id",
                            json_object_get(j_assertion, "gswa_id"));
      res = h_update(config->conn, j_query, NULL);
      json_decref(j_query);
      if (res != H_OK) {
        y_log_message(Y_LOG_LEVEL_ERROR, "check_assertion - Error executing j_query (3)");
        config->glewlwyd_module_callback_metrics_increment_counter(config, GLWD_METRICS_DATABSE_ERROR, 1, NULL);
        ret = G_ERROR_DB;
      }
    } else {
      j_query = json_pack("{sss{sisi}s{sO}}",
                          "table",
                          G_TABLE_WEBAUTHN_ASSERTION,
                          "set",
                            "gswa_counter",
                            counter_value,
                            "gswa_status",
                            3,
                          "where",
                            "gswa_id",
                            json_object_get(j_assertion, "gswa_id"));
      res = h_update(config->conn, j_query, NULL);
      json_decref(j_query);
      if (res != H_OK) {
        y_log_message(Y_LOG_LEVEL_ERROR, "check_assertion - Error executing j_query (4)");
        config->glewlwyd_module_callback_metrics_increment_counter(config, GLWD_METRICS_DATABSE_ERROR, 1, NULL);
        ret = G_ERROR_DB;
      }
    }
    o_free(client_data);
    o_free(challenge_b64);
    o_free(challenge_hash);
    o_free(auth_data);
    json_decref(j_client_data);
    json_decref(j_credential);
    gnutls_pubkey_deinit(pubkey);
  } else {
    ret = G_ERROR_PARAM;
  }
  return ret;
}