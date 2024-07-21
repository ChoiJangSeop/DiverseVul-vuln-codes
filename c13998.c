char *ogs_nas_5gs_suci_from_mobile_identity(
        ogs_nas_5gs_mobile_identity_t *mobile_identity)
{
    ogs_nas_5gs_mobile_identity_suci_t *mobile_identity_suci = NULL;
    ogs_plmn_id_t plmn_id;
    char tmp[OGS_MAX_IMSI_BCD_LEN+1];
    char routing_indicator[5];
    char *suci = NULL;

    ogs_assert(mobile_identity);

    mobile_identity_suci =
        (ogs_nas_5gs_mobile_identity_suci_t *)mobile_identity->buffer;
    ogs_assert(mobile_identity_suci);

    ogs_expect_or_return_val(mobile_identity_suci->h.supi_format ==
            OGS_NAS_5GS_SUPI_FORMAT_IMSI, NULL);
    ogs_expect_or_return_val(mobile_identity_suci->protection_scheme_id ==
            OGS_NAS_5GS_NULL_SCHEME, NULL);

    suci = ogs_msprintf("suci-%d-", mobile_identity_suci->h.supi_format);
    ogs_expect_or_return_val(suci, NULL);

    ogs_nas_to_plmn_id(&plmn_id, &mobile_identity_suci->nas_plmn_id);
    if (ogs_plmn_id_mnc_len(&plmn_id) == 2) {
        suci = ogs_mstrcatf(suci, "%03d-%02d-",
                ogs_plmn_id_mcc(&plmn_id), ogs_plmn_id_mnc(&plmn_id));
        ogs_expect_or_return_val(suci, NULL);
    } else {
        suci = ogs_mstrcatf(suci, "%03d-%03d-",
                ogs_plmn_id_mcc(&plmn_id), ogs_plmn_id_mnc(&plmn_id));
        ogs_expect_or_return_val(suci, NULL);
    }

    memset(routing_indicator, 0, sizeof(routing_indicator));
    if (mobile_identity_suci->routing_indicator1 != 0xf) {
        routing_indicator[0] =
            mobile_identity_suci->routing_indicator1 + '0';
        if (mobile_identity_suci->routing_indicator2 != 0xf) {
            routing_indicator[1] =
                mobile_identity_suci->routing_indicator2 + '0';
            if (mobile_identity_suci->routing_indicator3 != 0xf) {
                routing_indicator[2] =
                    mobile_identity_suci->routing_indicator3 + '0';
                if (mobile_identity_suci->routing_indicator4 != 0xf)
                    routing_indicator[3] =
                        mobile_identity_suci->routing_indicator4 + '0';
            }
        }
    }

    ogs_expect_or_return_val(mobile_identity->length > 8, NULL);
    ogs_buffer_to_bcd(mobile_identity_suci->scheme_output,
            mobile_identity->length - 8, tmp);

    suci = ogs_mstrcatf(suci, "%s-%d-%d-%s",
            routing_indicator,
            mobile_identity_suci->protection_scheme_id,
            mobile_identity_suci->home_network_pki_value,
            tmp);
    ogs_expect(suci);

    return suci;
}