dissect_usb_ms_bulk(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data)
{
    usb_conv_info_t *usb_conv_info;
    usb_ms_conv_info_t *usb_ms_conv_info;
    proto_tree *tree;
    proto_item *ti;
    guint32 signature=0;
    int offset=0;
    gboolean is_request;
    itl_nexus_t *itl;
    itlq_nexus_t *itlq;

    /* Reject the packet if data is NULL */
    if (data == NULL)
        return 0;
    usb_conv_info = (usb_conv_info_t *)data;

    /* verify that we do have a usb_ms_conv_info */
    usb_ms_conv_info=(usb_ms_conv_info_t *)usb_conv_info->class_data;
    if(!usb_ms_conv_info){
        usb_ms_conv_info=wmem_new(wmem_file_scope(), usb_ms_conv_info_t);
        usb_ms_conv_info->itl=wmem_tree_new(wmem_file_scope());
        usb_ms_conv_info->itlq=wmem_tree_new(wmem_file_scope());
        usb_conv_info->class_data=usb_ms_conv_info;
    }


    is_request=(pinfo->srcport==NO_ENDPOINT);

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "USBMS");

    col_clear(pinfo->cinfo, COL_INFO);


    ti = proto_tree_add_protocol_format(parent_tree, proto_usb_ms, tvb, 0, -1, "USB Mass Storage");
    tree = proto_item_add_subtree(ti, ett_usb_ms);

    signature=tvb_get_letohl(tvb, offset);


    /*
     * SCSI CDB inside CBW
     */
    if(is_request&&(signature==0x43425355)&&(tvb_reported_length(tvb)==31)){
        tvbuff_t *cdb_tvb;
        int cdbrlen, cdblen;
        guint8 lun, flags;
        guint32 datalen;

        /* dCBWSignature */
        proto_tree_add_item(tree, hf_usb_ms_dCBWSignature, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset+=4;

        /* dCBWTag */
        proto_tree_add_item(tree, hf_usb_ms_dCBWTag, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset+=4;

        /* dCBWDataTransferLength */
        proto_tree_add_item(tree, hf_usb_ms_dCBWDataTransferLength, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        datalen=tvb_get_letohl(tvb, offset);
        offset+=4;

        /* dCBWFlags */
        proto_tree_add_item(tree, hf_usb_ms_dCBWFlags, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        flags=tvb_get_guint8(tvb, offset);
        offset+=1;

        /* dCBWLUN */
        proto_tree_add_item(tree, hf_usb_ms_dCBWTarget, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        proto_tree_add_item(tree, hf_usb_ms_dCBWLUN, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        lun=tvb_get_guint8(tvb, offset)&0x0f;
        offset+=1;

        /* make sure we have a ITL structure for this LUN */
        itl=(itl_nexus_t *)wmem_tree_lookup32(usb_ms_conv_info->itl, lun);
        if(!itl){
            itl=wmem_new(wmem_file_scope(), itl_nexus_t);
            itl->cmdset=0xff;
            itl->conversation=NULL;
            wmem_tree_insert32(usb_ms_conv_info->itl, lun, itl);
        }

        /* make sure we have an ITLQ structure for this LUN/transaction */
        itlq=(itlq_nexus_t *)wmem_tree_lookup32(usb_ms_conv_info->itlq, pinfo->num);
        if(!itlq){
            itlq=wmem_new(wmem_file_scope(), itlq_nexus_t);
            itlq->lun=lun;
            itlq->scsi_opcode=0xffff;
            itlq->task_flags=0;
            if(datalen){
                if(flags&0x80){
                    itlq->task_flags|=SCSI_DATA_READ;
                } else {
                    itlq->task_flags|=SCSI_DATA_WRITE;
                }
            }
            itlq->data_length=datalen;
            itlq->bidir_data_length=0;
            itlq->fc_time=pinfo->abs_ts;
            itlq->first_exchange_frame=pinfo->num;
            itlq->last_exchange_frame=0;
            itlq->flags=0;
            itlq->alloc_len=0;
            itlq->extra_data=NULL;
            wmem_tree_insert32(usb_ms_conv_info->itlq, pinfo->num, itlq);
        }

        /* dCBWCBLength */
        proto_tree_add_item(tree, hf_usb_ms_dCBWCBLength, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        cdbrlen=tvb_get_guint8(tvb, offset)&0x1f;
        offset+=1;

        cdblen=cdbrlen;
        if(cdblen>tvb_captured_length_remaining(tvb, offset)){
            cdblen=tvb_captured_length_remaining(tvb, offset);
        }
        if(cdblen){
            cdb_tvb=tvb_new_subset(tvb, offset, cdblen, cdbrlen);
            dissect_scsi_cdb(cdb_tvb, pinfo, parent_tree, SCSI_DEV_UNKNOWN, itlq, itl);
        }
        return tvb_captured_length(tvb);
    }


    /*
     * SCSI RESPONSE inside CSW
     */
    if((!is_request)&&(signature==0x53425355)&&(tvb_reported_length(tvb)==13)){
        guint8 status;

        /* dCSWSignature */
        proto_tree_add_item(tree, hf_usb_ms_dCSWSignature, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset+=4;

        /* dCSWTag */
        proto_tree_add_item(tree, hf_usb_ms_dCBWTag, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset+=4;

        /* dCSWDataResidue */
        proto_tree_add_item(tree, hf_usb_ms_dCSWDataResidue, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset+=4;

        /* dCSWStatus */
        proto_tree_add_item(tree, hf_usb_ms_dCSWStatus, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        status=tvb_get_guint8(tvb, offset);
        /*offset+=1;*/

        itlq=(itlq_nexus_t *)wmem_tree_lookup32_le(usb_ms_conv_info->itlq, pinfo->num);
        if(!itlq){
            return tvb_captured_length(tvb);
        }
        itlq->last_exchange_frame=pinfo->num;

        itl=(itl_nexus_t *)wmem_tree_lookup32(usb_ms_conv_info->itl, itlq->lun);
        if(!itl){
            return tvb_captured_length(tvb);
        }

        if(!status){
            dissect_scsi_rsp(tvb, pinfo, parent_tree, itlq, itl, 0);
        } else {
            /* just send "check condition" */
            dissect_scsi_rsp(tvb, pinfo, parent_tree, itlq, itl, 0x02);
        }
        return tvb_captured_length(tvb);
    }

    /*
     * Ok it was neither CDB not STATUS so just assume it is either data in/out
     */
    itlq=(itlq_nexus_t *)wmem_tree_lookup32_le(usb_ms_conv_info->itlq, pinfo->num);
    if(!itlq){
        return tvb_captured_length(tvb);
    }

    itl=(itl_nexus_t *)wmem_tree_lookup32(usb_ms_conv_info->itl, itlq->lun);
    if(!itl){
        return tvb_captured_length(tvb);
    }

    dissect_scsi_payload(tvb, pinfo, parent_tree, is_request, itlq, itl, 0);
    return tvb_captured_length(tvb);
}