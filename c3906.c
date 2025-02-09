dissect_usb_video_control_interface_descriptor(proto_tree *parent_tree, tvbuff_t *tvb,
                                               guint8 descriptor_len, packet_info *pinfo, usb_conv_info_t *usb_conv_info)
{
    video_conv_info_t *video_conv_info = NULL;
    video_entity_t    *entity          = NULL;
    proto_item *item          = NULL;
    proto_item *subtype_item  = NULL;
    proto_tree *tree          = NULL;
    guint8      entity_id     = 0;
    guint16     terminal_type = 0;
    int         offset        = 0;
    guint8      subtype;

    subtype = tvb_get_guint8(tvb, offset+2);

    if (parent_tree)
    {
        const gchar *subtype_str;

        subtype_str = val_to_str_ext(subtype, &vc_if_descriptor_subtypes_ext, "Unknown (0x%x)");

        tree = proto_tree_add_subtree_format(parent_tree, tvb, offset, descriptor_len,
                                   ett_descriptor_video_control, &item, "VIDEO CONTROL INTERFACE DESCRIPTOR [%s]",
                                   subtype_str);
    }

    /* Common fields */
    dissect_usb_descriptor_header(tree, tvb, offset, &vid_descriptor_type_vals_ext);
    subtype_item = proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_subtype, tvb, offset+2, 1, ENC_LITTLE_ENDIAN);
    offset += 3;

    if (subtype == VC_HEADER)
    {
        guint8 num_vs_interfaces;

        proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_bcdUVC,            tvb, offset,   2, ENC_LITTLE_ENDIAN);
        proto_tree_add_item(tree, hf_usb_vid_ifdesc_wTotalLength,              tvb, offset+2, 2, ENC_LITTLE_ENDIAN);
        proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_dwClockFrequency,  tvb, offset+4, 4, ENC_LITTLE_ENDIAN);

        num_vs_interfaces = tvb_get_guint8(tvb, offset+8);
        proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_bInCollection,     tvb, offset+8, 1, ENC_LITTLE_ENDIAN);

        if (num_vs_interfaces > 0)
        {
            proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_baInterfaceNr, tvb, offset+9, num_vs_interfaces, ENC_NA);
        }

        offset += 9 + num_vs_interfaces;
    }
    else if ((subtype == VC_INPUT_TERMINAL) || (subtype == VC_OUTPUT_TERMINAL))
    {
        /* Fields common to input and output terminals */
        entity_id     = tvb_get_guint8(tvb, offset);
        terminal_type = tvb_get_letohs(tvb, offset+1);

        proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_terminal_id,    tvb, offset,   1, ENC_LITTLE_ENDIAN);
        proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_terminal_type,  tvb, offset+1, 2, ENC_LITTLE_ENDIAN);
        proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_assoc_terminal, tvb, offset+3, 1, ENC_LITTLE_ENDIAN);
        offset += 4;

        if (subtype == VC_OUTPUT_TERMINAL)
        {
            proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_src_id, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            ++offset;
        }

        proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_iTerminal, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        ++offset;

        if (subtype == VC_INPUT_TERMINAL)
        {
            if (terminal_type == ITT_CAMERA)
            {
                offset = dissect_usb_video_camera_terminal(tree, tvb, offset);
            }
            else if (terminal_type == ITT_MEDIA_TRANSPORT_INPUT)
            {
                /* @todo */
            }
        }

        if (subtype == VC_OUTPUT_TERMINAL)
        {
            if (terminal_type == OTT_MEDIA_TRANSPORT_OUTPUT)
            {
                /* @todo */
            }
        }
    }
    else
    {
        /* Field common to extension / processing / selector / encoding units */
        entity_id = tvb_get_guint8(tvb, offset);
        proto_tree_add_item(tree, hf_usb_vid_control_ifdesc_unit_id, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        ++offset;

        if (subtype == VC_PROCESSING_UNIT)
        {
            offset = dissect_usb_video_processing_unit(tree, tvb, offset);
        }
        else if (subtype == VC_SELECTOR_UNIT)
        {
            offset = dissect_usb_video_selector_unit(tree, tvb, offset);
        }
        else if (subtype == VC_EXTENSION_UNIT)
        {
            offset = dissect_usb_video_extension_unit(tree, tvb, offset);
        }
        else if (subtype == VC_ENCODING_UNIT)
        {
            /* @todo UVC 1.5 */
        }
        else
        {
            expert_add_info_format(pinfo, subtype_item, &ei_usb_vid_subtype_unknown,
                                   "Unknown VC subtype %u", subtype);
        }
    }

    /* Soak up descriptor bytes beyond those we know how to dissect */
    if (offset < descriptor_len)
    {
        proto_tree_add_item(tree, hf_usb_vid_descriptor_data, tvb, offset, descriptor_len-offset, ENC_NA);
        /* offset = descriptor_len; */
    }

    if (entity_id != 0)
        proto_item_append_text(item, " (Entity %d)", entity_id);

    if (subtype != VC_HEADER && usb_conv_info)
    {
        /* Switch to the usb_conv_info of the Video Control interface */
        usb_conv_info = get_usb_iface_conv_info(pinfo, usb_conv_info->interfaceNum);
        video_conv_info = (video_conv_info_t *)usb_conv_info->class_data;

        if (!video_conv_info)
        {
            video_conv_info = wmem_new(wmem_file_scope(), video_conv_info_t);
            video_conv_info->entities = wmem_tree_new(wmem_file_scope());
            usb_conv_info->class_data = video_conv_info;
        }

        entity = (video_entity_t*) wmem_tree_lookup32(video_conv_info->entities, entity_id);
        if (!entity)
        {
            entity = wmem_new(wmem_file_scope(), video_entity_t);
            entity->entityID     = entity_id;
            entity->subtype      = subtype;
            entity->terminalType = terminal_type;

            wmem_tree_insert32(video_conv_info->entities, entity_id, entity);
        }
    }

    return descriptor_len;
}