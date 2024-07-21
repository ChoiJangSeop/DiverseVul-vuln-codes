SdMmcCreateTrb (
  IN SD_MMC_HC_PRIVATE_DATA              *Private,
  IN UINT8                               Slot,
  IN EFI_SD_MMC_PASS_THRU_COMMAND_PACKET *Packet,
  IN EFI_EVENT                           Event
  )
{
  SD_MMC_HC_TRB                 *Trb;
  EFI_STATUS                    Status;
  EFI_TPL                       OldTpl;
  EFI_PCI_IO_PROTOCOL_OPERATION Flag;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINTN                         MapLength;

  Trb = AllocateZeroPool (sizeof (SD_MMC_HC_TRB));
  if (Trb == NULL) {
    return NULL;
  }

  Trb->Signature = SD_MMC_HC_TRB_SIG;
  Trb->Slot      = Slot;
  Trb->BlockSize = 0x200;
  Trb->Packet    = Packet;
  Trb->Event     = Event;
  Trb->Started   = FALSE;
  Trb->Timeout   = Packet->Timeout;
  Trb->Retries   = SD_MMC_TRB_RETRIES;
  Trb->Private   = Private;

  if ((Packet->InTransferLength != 0) && (Packet->InDataBuffer != NULL)) {
    Trb->Data    = Packet->InDataBuffer;
    Trb->DataLen = Packet->InTransferLength;
    Trb->Read    = TRUE;
  } else if ((Packet->OutTransferLength != 0) && (Packet->OutDataBuffer != NULL)) {
    Trb->Data    = Packet->OutDataBuffer;
    Trb->DataLen = Packet->OutTransferLength;
    Trb->Read    = FALSE;
  } else if ((Packet->InTransferLength == 0) && (Packet->OutTransferLength == 0)) {
    Trb->Data    = NULL;
    Trb->DataLen = 0;
  } else {
    goto Error;
  }

  if ((Trb->DataLen != 0) && (Trb->DataLen < Trb->BlockSize)) {
    Trb->BlockSize = (UINT16)Trb->DataLen;
  }

  if (((Private->Slot[Trb->Slot].CardType == EmmcCardType) &&
       (Packet->SdMmcCmdBlk->CommandIndex == EMMC_SEND_TUNING_BLOCK)) ||
      ((Private->Slot[Trb->Slot].CardType == SdCardType) &&
       (Packet->SdMmcCmdBlk->CommandIndex == SD_SEND_TUNING_BLOCK))) {
    Trb->Mode = SdMmcPioMode;
  } else {
    if (Trb->Read) {
      Flag = EfiPciIoOperationBusMasterWrite;
    } else {
      Flag = EfiPciIoOperationBusMasterRead;
    }

    PciIo = Private->PciIo;
    if (Trb->DataLen != 0) {
      MapLength = Trb->DataLen;
      Status = PciIo->Map (
                        PciIo,
                        Flag,
                        Trb->Data,
                        &MapLength,
                        &Trb->DataPhy,
                        &Trb->DataMap
                        );
      if (EFI_ERROR (Status) || (Trb->DataLen != MapLength)) {
        Status = EFI_BAD_BUFFER_SIZE;
        goto Error;
      }
    }

    if (Trb->DataLen == 0) {
      Trb->Mode = SdMmcNoData;
    } else if (Private->Capability[Slot].Adma2 != 0) {
      Trb->Mode = SdMmcAdma32bMode;
      Trb->AdmaLengthMode = SdMmcAdmaLen16b;
      if ((Private->ControllerVersion[Slot] == SD_MMC_HC_CTRL_VER_300) &&
          (Private->Capability[Slot].SysBus64V3 == 1)) {
        Trb->Mode = SdMmcAdma64bV3Mode;
      } else if (((Private->ControllerVersion[Slot] == SD_MMC_HC_CTRL_VER_400) &&
                  (Private->Capability[Slot].SysBus64V3 == 1)) ||
                 ((Private->ControllerVersion[Slot] >= SD_MMC_HC_CTRL_VER_410) &&
                  (Private->Capability[Slot].SysBus64V4 == 1))) {
        Trb->Mode = SdMmcAdma64bV4Mode;
      }
      if (Private->ControllerVersion[Slot] >= SD_MMC_HC_CTRL_VER_410) {
        Trb->AdmaLengthMode = SdMmcAdmaLen26b;
      }
      Status = BuildAdmaDescTable (Trb, Private->ControllerVersion[Slot]);
      if (EFI_ERROR (Status)) {
        PciIo->Unmap (PciIo, Trb->DataMap);
        goto Error;
      }
    } else if (Private->Capability[Slot].Sdma != 0) {
      Trb->Mode = SdMmcSdmaMode;
    } else {
      Trb->Mode = SdMmcPioMode;
    }
  }

  if (Event != NULL) {
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    InsertTailList (&Private->Queue, &Trb->TrbList);
    gBS->RestoreTPL (OldTpl);
  }

  return Trb;

Error:
  SdMmcFreeTrb (Trb);
  return NULL;
}