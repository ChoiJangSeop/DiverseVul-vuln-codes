dissect_gquic_tag(tvbuff_t *tvb, packet_info *pinfo, proto_tree *gquic_tree, guint offset, guint32 tag_number){
    guint32 tag_offset_start = offset + tag_number*4*2;
    guint32 tag_offset = 0, total_tag_len = 0;
    gint32 tag_len;

    while(tag_number){
        proto_tree *tag_tree, *ti_len, *ti_tag, *ti_type;
        guint32 offset_end, tag;
        const guint8* tag_str;

        ti_tag = proto_tree_add_item(gquic_tree, hf_gquic_tags, tvb, offset, 8, ENC_NA);
        tag_tree = proto_item_add_subtree(ti_tag, ett_gquic_tag_value);
        ti_type = proto_tree_add_item_ret_string(tag_tree, hf_gquic_tag_type, tvb, offset, 4, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
        tag = tvb_get_ntohl(tvb, offset);
        proto_item_append_text(ti_type, " (%s)", val_to_str(tag, tag_vals, "Unknown"));
        proto_item_append_text(ti_tag, ": %s (%s)", tag_str, val_to_str(tag, tag_vals, "Unknown"));
        offset += 4;

        proto_tree_add_item(tag_tree, hf_gquic_tag_offset_end, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset_end = tvb_get_guint32(tvb, offset, ENC_LITTLE_ENDIAN);

        tag_len = offset_end - tag_offset;
        total_tag_len += tag_len;
        ti_len = proto_tree_add_uint(tag_tree, hf_gquic_tag_length, tvb, offset, 4, tag_len);
        proto_item_append_text(ti_tag, " (l=%u)", tag_len);
        proto_item_set_generated(ti_len);
        offset += 4;

        /* Fix issue with CRT.. (Fragmentation ?) */
        if( tag_len > tvb_reported_length_remaining(tvb, tag_offset_start + tag_offset)){
            tag_len = tvb_reported_length_remaining(tvb, tag_offset_start + tag_offset);
            offset_end = tag_offset + tag_len;
            expert_add_info(pinfo, ti_len, &ei_gquic_tag_length);
        }

        proto_tree_add_item(tag_tree, hf_gquic_tag_value, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);

        switch(tag){
            case TAG_PAD:
                proto_tree_add_item(tag_tree, hf_gquic_tag_pad, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_SNI:
                proto_tree_add_item_ret_string(tag_tree, hf_gquic_tag_sni, tvb, tag_offset_start + tag_offset, tag_len, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
                proto_item_append_text(ti_tag, ": %s", tag_str);
                tag_offset += tag_len;
            break;
            case TAG_VER:
                proto_tree_add_item_ret_string(tag_tree, hf_gquic_tag_ver, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
                proto_item_append_text(ti_tag, ": %s", tag_str);
                tag_offset += 4;
            break;
            case TAG_CCS:
                while(offset_end - tag_offset >= 8){
                    proto_tree_add_item(tag_tree, hf_gquic_tag_ccs, tvb, tag_offset_start + tag_offset, 8, ENC_NA);
                    tag_offset += 8;
                }
            break;
            case TAG_PDMD:
                proto_tree_add_item_ret_string(tag_tree, hf_gquic_tag_pdmd, tvb, tag_offset_start + tag_offset, tag_len, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
                proto_item_append_text(ti_tag, ": %s", tag_str);
                tag_offset += tag_len;
            break;
            case TAG_UAID:
                proto_tree_add_item_ret_string(tag_tree, hf_gquic_tag_uaid, tvb, tag_offset_start + tag_offset, tag_len, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
                proto_item_append_text(ti_tag, ": %s", tag_str);
                tag_offset += tag_len;
            break;
            case TAG_STK:
                proto_tree_add_item(tag_tree, hf_gquic_tag_stk, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_SNO:
                proto_tree_add_item(tag_tree, hf_gquic_tag_sno, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_PROF:
                proto_tree_add_item(tag_tree, hf_gquic_tag_prof, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_SCFG:{
                guint32 scfg_tag_number;

                proto_tree_add_item(tag_tree, hf_gquic_tag_scfg, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA);
                tag_offset += 4;
                proto_tree_add_item(tag_tree, hf_gquic_tag_scfg_number, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                scfg_tag_number = tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN);
                tag_offset += 4;

                dissect_gquic_tag(tvb, pinfo, tag_tree, tag_offset_start + tag_offset, scfg_tag_number);
                tag_offset += tag_len - 4 - 4;
                }
            break;
            case TAG_RREJ:
                while(offset_end - tag_offset >= 4){
                    proto_tree_add_item(tag_tree, hf_gquic_tag_rrej, tvb, tag_offset_start + tag_offset, 4,  ENC_LITTLE_ENDIAN);
                    proto_item_append_text(ti_tag, ", Code %s", val_to_str_ext(tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN), &handshake_failure_reason_vals_ext, "Unknown"));
                    tag_offset += 4;
                }
            break;
            case TAG_CRT:
                proto_tree_add_item(tag_tree, hf_gquic_tag_crt, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_AEAD:
                while(offset_end - tag_offset >= 4){
                    proto_tree *ti_aead;
                    ti_aead = proto_tree_add_item(tag_tree, hf_gquic_tag_aead, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA);
                    proto_item_append_text(ti_aead, " (%s)", val_to_str(tvb_get_ntohl(tvb, tag_offset_start + tag_offset), tag_aead_vals, "Unknown"));
                    proto_item_append_text(ti_tag, ", %s", val_to_str(tvb_get_ntohl(tvb, tag_offset_start + tag_offset), tag_aead_vals, "Unknown"));
                    tag_offset += 4;
                }
            break;
            case TAG_SCID:
                proto_tree_add_item(tag_tree, hf_gquic_tag_scid, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_PUBS:
                /*TODO FIX: 24 Length + Pubs key?.. ! */
                proto_tree_add_item(tag_tree, hf_gquic_tag_pubs, tvb, tag_offset_start + tag_offset, 2, ENC_LITTLE_ENDIAN);
                tag_offset += 2;
                while(offset_end - tag_offset >= 3){
                    proto_tree_add_item(tag_tree, hf_gquic_tag_pubs, tvb, tag_offset_start + tag_offset, 3, ENC_LITTLE_ENDIAN);
                    tag_offset += 3;
                }
            break;
            case TAG_KEXS:
                while(offset_end - tag_offset >= 4){
                    proto_tree *ti_kexs;
                    ti_kexs = proto_tree_add_item(tag_tree, hf_gquic_tag_kexs, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA);
                    proto_item_append_text(ti_kexs, " (%s)", val_to_str(tvb_get_ntohl(tvb, tag_offset_start + tag_offset), tag_kexs_vals, "Unknown"));
                    proto_item_append_text(ti_tag, ", %s", val_to_str(tvb_get_ntohl(tvb, tag_offset_start + tag_offset), tag_kexs_vals, "Unknown"));
                    tag_offset += 4;
                }
            break;
            case TAG_OBIT:
                proto_tree_add_item(tag_tree, hf_gquic_tag_obit, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_EXPY:
                proto_tree_add_item(tag_tree, hf_gquic_tag_expy, tvb, tag_offset_start + tag_offset, 8, ENC_LITTLE_ENDIAN);
                tag_offset += 8;
            break;
            case TAG_NONC:
                /*TODO: Enhance display: 32 bytes consisting of 4 bytes of timestamp (big-endian, UNIX epoch seconds), 8 bytes of server orbit and 20 bytes of random data. */
                proto_tree_add_item(tag_tree, hf_gquic_tag_nonc, tvb, tag_offset_start + tag_offset, 32, ENC_NA);
                tag_offset += 32;
            break;
            case TAG_MSPC:
                proto_tree_add_item(tag_tree, hf_gquic_tag_mspc, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                proto_item_append_text(ti_tag, ": %u", tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN));
                tag_offset += 4;
            break;
            case TAG_TCID:
                proto_tree_add_item(tag_tree, hf_gquic_tag_tcid, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                tag_offset += 4;
            break;
            case TAG_SRBF:
                proto_tree_add_item(tag_tree, hf_gquic_tag_srbf, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                tag_offset += 4;
            break;
            case TAG_ICSL:
                proto_tree_add_item(tag_tree, hf_gquic_tag_icsl, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                tag_offset += 4;
            break;
            case TAG_SCLS:
                proto_tree_add_item(tag_tree, hf_gquic_tag_scls, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                tag_offset += 4;
            break;
            case TAG_COPT:
                while(offset_end - tag_offset >= 4){
                    proto_tree_add_item(tag_tree, hf_gquic_tag_copt, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA);
                    tag_offset += 4;
                }
            break;
            case TAG_CCRT:
                proto_tree_add_item(tag_tree, hf_gquic_tag_ccrt, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_IRTT:
                proto_tree_add_item(tag_tree, hf_gquic_tag_irtt, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                proto_item_append_text(ti_tag, ": %u", tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN));
                tag_offset += 4;
            break;
            case TAG_CFCW:
                proto_tree_add_item(tag_tree, hf_gquic_tag_cfcw, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                proto_item_append_text(ti_tag, ": %u", tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN));
                tag_offset += 4;
            break;
            case TAG_SFCW:
                proto_tree_add_item(tag_tree, hf_gquic_tag_sfcw, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                proto_item_append_text(ti_tag, ": %u", tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN));
                tag_offset += 4;
            break;
            case TAG_CETV:
                proto_tree_add_item(tag_tree, hf_gquic_tag_cetv, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_XLCT:
                proto_tree_add_item(tag_tree, hf_gquic_tag_xlct, tvb, tag_offset_start + tag_offset, 8, ENC_NA);
                tag_offset += 8;
            break;
            case TAG_NONP:
                proto_tree_add_item(tag_tree, hf_gquic_tag_nonp, tvb, tag_offset_start + tag_offset, 32, ENC_NA);
                tag_offset += 32;
            break;
            case TAG_CSCT:
                proto_tree_add_item(tag_tree, hf_gquic_tag_csct, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_CTIM:
                proto_tree_add_item(tag_tree, hf_gquic_tag_ctim, tvb, tag_offset_start + tag_offset, 8, ENC_LITTLE_ENDIAN|ENC_TIME_SECS_NSECS);
                tag_offset += 8;
            break;
            case TAG_RNON: /* Public Reset Tag */
                proto_tree_add_item(tag_tree, hf_gquic_tag_rnon, tvb, tag_offset_start + tag_offset, 8, ENC_LITTLE_ENDIAN);
                tag_offset += 8;
            break;
            case TAG_RSEQ: /* Public Reset Tag */
                proto_tree_add_item(tag_tree, hf_gquic_tag_rseq, tvb, tag_offset_start + tag_offset, 8, ENC_LITTLE_ENDIAN);
                tag_offset += 8;
            break;
            case TAG_CADR: /* Public Reset Tag */{
                guint32 addr_type;
                proto_tree_add_item_ret_uint(tag_tree, hf_gquic_tag_cadr_addr_type, tvb, tag_offset_start + tag_offset, 2, ENC_LITTLE_ENDIAN, &addr_type);
                tag_offset += 2;
                switch(addr_type){
                    case 2: /* IPv4 */
                    proto_tree_add_item(tag_tree, hf_gquic_tag_cadr_addr_ipv4, tvb, tag_offset_start + tag_offset, 4, ENC_NA);
                    tag_offset += 4;
                    break;
                    case 10: /* IPv6 */
                    proto_tree_add_item(tag_tree, hf_gquic_tag_cadr_addr_ipv6, tvb, tag_offset_start + tag_offset, 16, ENC_NA);
                    tag_offset += 16;
                    break;
                    default: /* Unknown */
                    proto_tree_add_item(tag_tree, hf_gquic_tag_cadr_addr, tvb, tag_offset_start + tag_offset, tag_len - 2 - 2, ENC_NA);
                    tag_offset += tag_len + 2 + 2 ;
                    break;
                }
                proto_tree_add_item(tag_tree, hf_gquic_tag_cadr_port, tvb, tag_offset_start + tag_offset, 2, ENC_LITTLE_ENDIAN);
                tag_offset += 2;
            }
            break;
            case TAG_MIDS:
                proto_tree_add_item(tag_tree, hf_gquic_tag_mids, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                proto_item_append_text(ti_tag, ": %u", tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN));
                tag_offset += 4;
            break;
            case TAG_FHOL:
                proto_tree_add_item(tag_tree, hf_gquic_tag_fhol, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                proto_item_append_text(ti_tag, ": %u", tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN));
                tag_offset += 4;
            break;
            case TAG_STTL:
                proto_tree_add_item(tag_tree, hf_gquic_tag_sttl, tvb, tag_offset_start + tag_offset, 8, ENC_LITTLE_ENDIAN);
                tag_offset += 8;
            break;
            case TAG_SMHL:
                proto_tree_add_item(tag_tree, hf_gquic_tag_smhl, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                proto_item_append_text(ti_tag, ": %u", tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN));
                tag_offset += 4;
            break;
            case TAG_TBKP:
                proto_tree_add_item_ret_string(tag_tree, hf_gquic_tag_tbkp, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
                proto_item_append_text(ti_tag, ": %s", tag_str);
                tag_offset += 4;
            break;
            case TAG_MAD0:
                proto_tree_add_item(tag_tree, hf_gquic_tag_mad0, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                proto_item_append_text(ti_tag, ": %u", tvb_get_guint32(tvb, tag_offset_start + tag_offset, ENC_LITTLE_ENDIAN));
                tag_offset += 4;
            break;
            default:
                proto_tree_add_item(tag_tree, hf_gquic_tag_unknown, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                expert_add_info_format(pinfo, ti_tag, &ei_gquic_tag_undecoded,
                                 "Dissector for (Google) QUIC Tag"
                                 " %s (%s) code not implemented, Contact"
                                 " Wireshark developers if you want this supported", tvb_get_string_enc(wmem_packet_scope(), tvb, offset-8, 4, ENC_ASCII|ENC_NA), val_to_str(tag, tag_vals, "Unknown"));
                tag_offset += tag_len;
            break;
        }
        if(tag_offset != offset_end){
            /* Wrong Tag len... */
            proto_tree_add_expert(tag_tree, pinfo, &ei_gquic_tag_unknown, tvb, tag_offset_start + tag_offset, tag_len);
            tag_offset = offset_end;
        }

        tag_number--;
    }
    return offset + total_tag_len;

}