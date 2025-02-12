static int arcmsr_iop_message_xfer(struct AdapterControlBlock *acb,
		struct scsi_cmnd *cmd)
{
	char *buffer;
	unsigned short use_sg;
	int retvalue = 0, transfer_len = 0;
	unsigned long flags;
	struct CMD_MESSAGE_FIELD *pcmdmessagefld;
	uint32_t controlcode = (uint32_t)cmd->cmnd[5] << 24 |
		(uint32_t)cmd->cmnd[6] << 16 |
		(uint32_t)cmd->cmnd[7] << 8 |
		(uint32_t)cmd->cmnd[8];
	struct scatterlist *sg;

	use_sg = scsi_sg_count(cmd);
	sg = scsi_sglist(cmd);
	buffer = kmap_atomic(sg_page(sg)) + sg->offset;
	if (use_sg > 1) {
		retvalue = ARCMSR_MESSAGE_FAIL;
		goto message_out;
	}
	transfer_len += sg->length;
	if (transfer_len > sizeof(struct CMD_MESSAGE_FIELD)) {
		retvalue = ARCMSR_MESSAGE_FAIL;
		pr_info("%s: ARCMSR_MESSAGE_FAIL!\n", __func__);
		goto message_out;
	}
	pcmdmessagefld = (struct CMD_MESSAGE_FIELD *)buffer;
	switch (controlcode) {
	case ARCMSR_MESSAGE_READ_RQBUFFER: {
		unsigned char *ver_addr;
		uint8_t *ptmpQbuffer;
		uint32_t allxfer_len = 0;
		ver_addr = kmalloc(ARCMSR_API_DATA_BUFLEN, GFP_ATOMIC);
		if (!ver_addr) {
			retvalue = ARCMSR_MESSAGE_FAIL;
			pr_info("%s: memory not enough!\n", __func__);
			goto message_out;
		}
		ptmpQbuffer = ver_addr;
		spin_lock_irqsave(&acb->rqbuffer_lock, flags);
		if (acb->rqbuf_getIndex != acb->rqbuf_putIndex) {
			unsigned int tail = acb->rqbuf_getIndex;
			unsigned int head = acb->rqbuf_putIndex;
			unsigned int cnt_to_end = CIRC_CNT_TO_END(head, tail, ARCMSR_MAX_QBUFFER);

			allxfer_len = CIRC_CNT(head, tail, ARCMSR_MAX_QBUFFER);
			if (allxfer_len > ARCMSR_API_DATA_BUFLEN)
				allxfer_len = ARCMSR_API_DATA_BUFLEN;

			if (allxfer_len <= cnt_to_end)
				memcpy(ptmpQbuffer, acb->rqbuffer + tail, allxfer_len);
			else {
				memcpy(ptmpQbuffer, acb->rqbuffer + tail, cnt_to_end);
				memcpy(ptmpQbuffer + cnt_to_end, acb->rqbuffer, allxfer_len - cnt_to_end);
			}
			acb->rqbuf_getIndex = (acb->rqbuf_getIndex + allxfer_len) % ARCMSR_MAX_QBUFFER;
		}
		memcpy(pcmdmessagefld->messagedatabuffer, ver_addr,
			allxfer_len);
		if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
			struct QBUFFER __iomem *prbuffer;
			acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
			prbuffer = arcmsr_get_iop_rqbuffer(acb);
			if (arcmsr_Read_iop_rqbuffer_data(acb, prbuffer) == 0)
				acb->acb_flags |= ACB_F_IOPDATA_OVERFLOW;
		}
		spin_unlock_irqrestore(&acb->rqbuffer_lock, flags);
		kfree(ver_addr);
		pcmdmessagefld->cmdmessage.Length = allxfer_len;
		if (acb->fw_flag == FW_DEADLOCK)
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
		else
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_OK;
		break;
	}
	case ARCMSR_MESSAGE_WRITE_WQBUFFER: {
		unsigned char *ver_addr;
		int32_t user_len, cnt2end;
		uint8_t *pQbuffer, *ptmpuserbuffer;
		ver_addr = kmalloc(ARCMSR_API_DATA_BUFLEN, GFP_ATOMIC);
		if (!ver_addr) {
			retvalue = ARCMSR_MESSAGE_FAIL;
			goto message_out;
		}
		ptmpuserbuffer = ver_addr;
		user_len = pcmdmessagefld->cmdmessage.Length;
		memcpy(ptmpuserbuffer,
			pcmdmessagefld->messagedatabuffer, user_len);
		spin_lock_irqsave(&acb->wqbuffer_lock, flags);
		if (acb->wqbuf_putIndex != acb->wqbuf_getIndex) {
			struct SENSE_DATA *sensebuffer =
				(struct SENSE_DATA *)cmd->sense_buffer;
			arcmsr_write_ioctldata2iop(acb);
			/* has error report sensedata */
			sensebuffer->ErrorCode = SCSI_SENSE_CURRENT_ERRORS;
			sensebuffer->SenseKey = ILLEGAL_REQUEST;
			sensebuffer->AdditionalSenseLength = 0x0A;
			sensebuffer->AdditionalSenseCode = 0x20;
			sensebuffer->Valid = 1;
			retvalue = ARCMSR_MESSAGE_FAIL;
		} else {
			pQbuffer = &acb->wqbuffer[acb->wqbuf_putIndex];
			cnt2end = ARCMSR_MAX_QBUFFER - acb->wqbuf_putIndex;
			if (user_len > cnt2end) {
				memcpy(pQbuffer, ptmpuserbuffer, cnt2end);
				ptmpuserbuffer += cnt2end;
				user_len -= cnt2end;
				acb->wqbuf_putIndex = 0;
				pQbuffer = acb->wqbuffer;
			}
			memcpy(pQbuffer, ptmpuserbuffer, user_len);
			acb->wqbuf_putIndex += user_len;
			acb->wqbuf_putIndex %= ARCMSR_MAX_QBUFFER;
			if (acb->acb_flags & ACB_F_MESSAGE_WQBUFFER_CLEARED) {
				acb->acb_flags &=
						~ACB_F_MESSAGE_WQBUFFER_CLEARED;
				arcmsr_write_ioctldata2iop(acb);
			}
		}
		spin_unlock_irqrestore(&acb->wqbuffer_lock, flags);
		kfree(ver_addr);
		if (acb->fw_flag == FW_DEADLOCK)
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
		else
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_OK;
		break;
	}
	case ARCMSR_MESSAGE_CLEAR_RQBUFFER: {
		uint8_t *pQbuffer = acb->rqbuffer;

		arcmsr_clear_iop2drv_rqueue_buffer(acb);
		spin_lock_irqsave(&acb->rqbuffer_lock, flags);
		acb->acb_flags |= ACB_F_MESSAGE_RQBUFFER_CLEARED;
		acb->rqbuf_getIndex = 0;
		acb->rqbuf_putIndex = 0;
		memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
		spin_unlock_irqrestore(&acb->rqbuffer_lock, flags);
		if (acb->fw_flag == FW_DEADLOCK)
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
		else
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_OK;
		break;
	}
	case ARCMSR_MESSAGE_CLEAR_WQBUFFER: {
		uint8_t *pQbuffer = acb->wqbuffer;
		spin_lock_irqsave(&acb->wqbuffer_lock, flags);
		acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED |
			ACB_F_MESSAGE_WQBUFFER_READED);
		acb->wqbuf_getIndex = 0;
		acb->wqbuf_putIndex = 0;
		memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
		spin_unlock_irqrestore(&acb->wqbuffer_lock, flags);
		if (acb->fw_flag == FW_DEADLOCK)
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
		else
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_OK;
		break;
	}
	case ARCMSR_MESSAGE_CLEAR_ALLQBUFFER: {
		uint8_t *pQbuffer;
		arcmsr_clear_iop2drv_rqueue_buffer(acb);
		spin_lock_irqsave(&acb->rqbuffer_lock, flags);
		acb->acb_flags |= ACB_F_MESSAGE_RQBUFFER_CLEARED;
		acb->rqbuf_getIndex = 0;
		acb->rqbuf_putIndex = 0;
		pQbuffer = acb->rqbuffer;
		memset(pQbuffer, 0, sizeof(struct QBUFFER));
		spin_unlock_irqrestore(&acb->rqbuffer_lock, flags);
		spin_lock_irqsave(&acb->wqbuffer_lock, flags);
		acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED |
			ACB_F_MESSAGE_WQBUFFER_READED);
		acb->wqbuf_getIndex = 0;
		acb->wqbuf_putIndex = 0;
		pQbuffer = acb->wqbuffer;
		memset(pQbuffer, 0, sizeof(struct QBUFFER));
		spin_unlock_irqrestore(&acb->wqbuffer_lock, flags);
		if (acb->fw_flag == FW_DEADLOCK)
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
		else
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_OK;
		break;
	}
	case ARCMSR_MESSAGE_RETURN_CODE_3F: {
		if (acb->fw_flag == FW_DEADLOCK)
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
		else
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_3F;
		break;
	}
	case ARCMSR_MESSAGE_SAY_HELLO: {
		int8_t *hello_string = "Hello! I am ARCMSR";
		if (acb->fw_flag == FW_DEADLOCK)
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
		else
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_OK;
		memcpy(pcmdmessagefld->messagedatabuffer,
			hello_string, (int16_t)strlen(hello_string));
		break;
	}
	case ARCMSR_MESSAGE_SAY_GOODBYE: {
		if (acb->fw_flag == FW_DEADLOCK)
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
		else
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_OK;
		arcmsr_iop_parking(acb);
		break;
	}
	case ARCMSR_MESSAGE_FLUSH_ADAPTER_CACHE: {
		if (acb->fw_flag == FW_DEADLOCK)
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
		else
			pcmdmessagefld->cmdmessage.ReturnCode =
				ARCMSR_MESSAGE_RETURNCODE_OK;
		arcmsr_flush_adapter_cache(acb);
		break;
	}
	default:
		retvalue = ARCMSR_MESSAGE_FAIL;
		pr_info("%s: unknown controlcode!\n", __func__);
	}
message_out:
	if (use_sg) {
		struct scatterlist *sg = scsi_sglist(cmd);
		kunmap_atomic(buffer - sg->offset);
	}
	return retvalue;
}