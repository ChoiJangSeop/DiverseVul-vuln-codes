dissect_fb_zero_tag(tvbuff_t *tvb, packet_info *pinfo, proto_tree *fb_zero_tree, guint offset, guint32 tag_number){
    guint32 tag_offset_start = offset + tag_number*4*2;
    guint32 tag_offset = 0, total_tag_len = 0;
    gint32 tag_len;

    while(tag_number){
        proto_tree *tag_tree, *ti_len, *ti_tag, *ti_type;
        guint32 offset_end, tag;
        const guint8* tag_str;

        ti_tag = proto_tree_add_item(fb_zero_tree, hf_fb_zero_tags, tvb, offset, 8, ENC_NA);
        tag_tree = proto_item_add_subtree(ti_tag, ett_fb_zero_tag_value);
        ti_type = proto_tree_add_item_ret_string(tag_tree, hf_fb_zero_tag_type, tvb, offset, 4, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
        tag = tvb_get_ntohl(tvb, offset);
        proto_item_append_text(ti_type, " (%s)", val_to_str(tag, tag_vals, "Unknown"));
        proto_item_append_text(ti_tag, ": %s (%s)", tag_str, val_to_str(tag, tag_vals, "Unknown"));
        offset += 4;

        proto_tree_add_item(tag_tree, hf_fb_zero_tag_offset_end, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset_end = tvb_get_letohl(tvb, offset);

        tag_len = offset_end - tag_offset;
        total_tag_len += tag_len;
        ti_len = proto_tree_add_uint(tag_tree, hf_fb_zero_tag_length, tvb, offset, 4, tag_len);
        proto_item_append_text(ti_tag, " (l=%u)", tag_len);
        proto_item_set_generated(ti_len);
        offset += 4;

        /* Fix issue with CRT.. (Fragmentation ?) */
        if( tag_len > tvb_reported_length_remaining(tvb, tag_offset_start + tag_offset)){
            tag_len = tvb_reported_length_remaining(tvb, tag_offset_start + tag_offset);
            offset_end = tag_offset + tag_len;
            expert_add_info(pinfo, ti_len, &ei_fb_zero_tag_length);
        }

        proto_tree_add_item(tag_tree, hf_fb_zero_tag_value, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);

        switch(tag){
            case TAG_SNI:
                proto_tree_add_item_ret_string(tag_tree, hf_fb_zero_tag_sni, tvb, tag_offset_start + tag_offset, tag_len, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
                proto_item_append_text(ti_tag, ": %s", tag_str);
                tag_offset += tag_len;
            break;
            case TAG_VERS:
                proto_tree_add_item_ret_string(tag_tree, hf_fb_zero_tag_vers, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
                proto_item_append_text(ti_tag, ": %s", tag_str);
                tag_offset += 4;
            break;
            case TAG_SNO:
                proto_tree_add_item(tag_tree, hf_fb_zero_tag_sno, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_AEAD:
                while(offset_end - tag_offset >= 4){
                    proto_tree *ti_aead;
                    ti_aead = proto_tree_add_item(tag_tree, hf_fb_zero_tag_aead, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA);
                    proto_item_append_text(ti_aead, " (%s)", val_to_str(tvb_get_ntohl(tvb, tag_offset_start + tag_offset), tag_aead_vals, "Unknown"));
                    proto_item_append_text(ti_tag, ", %s", val_to_str(tvb_get_ntohl(tvb, tag_offset_start + tag_offset), tag_aead_vals, "Unknown"));
                    tag_offset += 4;
                }
            break;
            case TAG_SCID:
                proto_tree_add_item(tag_tree, hf_fb_zero_tag_scid, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                tag_offset += tag_len;
            break;
            case TAG_TIME:
                proto_tree_add_item(tag_tree, hf_fb_zero_tag_time, tvb, tag_offset_start + tag_offset, 4, ENC_LITTLE_ENDIAN);
                proto_item_append_text(ti_tag, ": %u", tvb_get_letohl(tvb, tag_offset_start + tag_offset));
                tag_offset += 4;
            break;
            case TAG_ALPN:
                proto_tree_add_item_ret_string(tag_tree, hf_fb_zero_tag_alpn, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA, wmem_packet_scope(), &tag_str);
                proto_item_append_text(ti_tag, ": %s", tag_str);
                tag_offset += 4;
            break;
            case TAG_PUBS:
                /*TODO FIX: 24 Length + Pubs key?.. ! */
                proto_tree_add_item(tag_tree, hf_fb_zero_tag_pubs, tvb, tag_offset_start + tag_offset, 2, ENC_LITTLE_ENDIAN);
                tag_offset += 2;
                while(offset_end - tag_offset >= 3){
                    proto_tree_add_item(tag_tree, hf_fb_zero_tag_pubs, tvb, tag_offset_start + tag_offset, 3, ENC_LITTLE_ENDIAN);
                    tag_offset += 3;
                }
            break;
            case TAG_KEXS:
                while(offset_end - tag_offset >= 4){
                    proto_tree *ti_kexs;
                    ti_kexs = proto_tree_add_item(tag_tree, hf_fb_zero_tag_kexs, tvb, tag_offset_start + tag_offset, 4, ENC_ASCII|ENC_NA);
                    proto_item_append_text(ti_kexs, " (%s)", val_to_str(tvb_get_ntohl(tvb, tag_offset_start + tag_offset), tag_kexs_vals, "Unknown"));
                    proto_item_append_text(ti_tag, ", %s", val_to_str(tvb_get_ntohl(tvb, tag_offset_start + tag_offset), tag_kexs_vals, "Unknown"));
                    tag_offset += 4;
                }
            break;
            case TAG_NONC:
                /*TODO: Enhance display: 32 bytes consisting of 4 bytes of timestamp (big-endian, UNIX epoch seconds), 8 bytes of server orbit and 20 bytes of random data. */
                proto_tree_add_item(tag_tree, hf_fb_zero_tag_nonc, tvb, tag_offset_start + tag_offset, 32, ENC_NA);
                tag_offset += 32;
            break;

            default:
                proto_tree_add_item(tag_tree, hf_fb_zero_tag_unknown, tvb, tag_offset_start + tag_offset, tag_len, ENC_NA);
                expert_add_info_format(pinfo, ti_tag, &ei_fb_zero_tag_undecoded,
                                 "Dissector for FB Zero Tag"
                                 " %s (%s) code not implemented, Contact"
                                 " Wireshark developers if you want this supported", tvb_get_string_enc(wmem_packet_scope(), tvb, offset-8, 4, ENC_ASCII|ENC_NA), val_to_str(tag, tag_vals, "Unknown"));
                tag_offset += tag_len;
            break;
        }

        if(tag_offset != offset_end){
            /* Wrong Tag len... */
            proto_tree_add_expert(tag_tree, pinfo, &ei_fb_zero_tag_unknown, tvb, tag_offset_start + tag_offset, offset_end - tag_offset);
            tag_offset = offset_end;
        }

        tag_number--;
    }
    return offset + total_tag_len;

}