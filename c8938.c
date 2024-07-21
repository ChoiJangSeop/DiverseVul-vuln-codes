BuildAdmaDescTable (
  IN SD_MMC_HC_TRB          *Trb,
  IN UINT16                 ControllerVer
  )
{
  EFI_PHYSICAL_ADDRESS      Data;
  UINT64                    DataLen;
  UINT64                    Entries;
  UINT32                    Index;
  UINT64                    Remaining;
  UINT64                    Address;
  UINTN                     TableSize;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_STATUS                Status;
  UINTN                     Bytes;
  UINT32                    AdmaMaxDataPerLine;
  UINT32                    DescSize;
  VOID                      *AdmaDesc;

  AdmaMaxDataPerLine = ADMA_MAX_DATA_PER_LINE_16B;
  DescSize           = sizeof (SD_MMC_HC_ADMA_32_DESC_LINE);
  AdmaDesc           = NULL;

  Data    = Trb->DataPhy;
  DataLen = Trb->DataLen;
  PciIo   = Trb->Private->PciIo;

  //
  // Check for valid ranges in 32bit ADMA Descriptor Table
  //
  if ((Trb->Mode == SdMmcAdma32bMode) &&
      ((Data >= 0x100000000ul) || ((Data + DataLen) > 0x100000000ul))) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check address field alignment
  //
  if (Trb->Mode != SdMmcAdma32bMode) {
    //
    // Address field shall be set on 64-bit boundary (Lower 3-bit is always set to 0)
    //
    if ((Data & (BIT0 | BIT1 | BIT2)) != 0) {
      DEBUG ((DEBUG_INFO, "The buffer [0x%x] to construct ADMA desc is not aligned to 8 bytes boundary!\n", Data));
    }
  } else {
    //
    // Address field shall be set on 32-bit boundary (Lower 2-bit is always set to 0)
    //
    if ((Data & (BIT0 | BIT1)) != 0) {
      DEBUG ((DEBUG_INFO, "The buffer [0x%x] to construct ADMA desc is not aligned to 4 bytes boundary!\n", Data));
    }
  }

  //
  // Configure 64b ADMA.
  //
  if (Trb->Mode == SdMmcAdma64bV3Mode) {
    DescSize = sizeof (SD_MMC_HC_ADMA_64_V3_DESC_LINE);
  }else if (Trb->Mode == SdMmcAdma64bV4Mode) {
    DescSize = sizeof (SD_MMC_HC_ADMA_64_V4_DESC_LINE);
  }
  //
  // Configure 26b data length.
  //
  if (Trb->AdmaLengthMode == SdMmcAdmaLen26b) {
    AdmaMaxDataPerLine = ADMA_MAX_DATA_PER_LINE_26B;
  }

  Entries   = DivU64x32 ((DataLen + AdmaMaxDataPerLine - 1), AdmaMaxDataPerLine);
  TableSize = (UINTN)MultU64x32 (Entries, DescSize);
  Trb->AdmaPages = (UINT32)EFI_SIZE_TO_PAGES (TableSize);
  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (TableSize),
                    (VOID **)&AdmaDesc,
                    0
                    );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  ZeroMem (AdmaDesc, TableSize);
  Bytes  = TableSize;
  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    AdmaDesc,
                    &Bytes,
                    &Trb->AdmaDescPhy,
                    &Trb->AdmaMap
                    );

  if (EFI_ERROR (Status) || (Bytes != TableSize)) {
    //
    // Map error or unable to map the whole RFis buffer into a contiguous region.
    //
    PciIo->FreeBuffer (
             PciIo,
             EFI_SIZE_TO_PAGES (TableSize),
             AdmaDesc
             );
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Trb->Mode == SdMmcAdma32bMode) &&
      (UINT64)(UINTN)Trb->AdmaDescPhy > 0x100000000ul) {
    //
    // The ADMA doesn't support 64bit addressing.
    //
    PciIo->Unmap (
      PciIo,
      Trb->AdmaMap
    );
    PciIo->FreeBuffer (
      PciIo,
      EFI_SIZE_TO_PAGES (TableSize),
      AdmaDesc
    );
    return EFI_DEVICE_ERROR;
  }

  Remaining = DataLen;
  Address   = Data;
  if (Trb->Mode == SdMmcAdma32bMode) {
    Trb->Adma32Desc = AdmaDesc;
  } else if (Trb->Mode == SdMmcAdma64bV3Mode) {
    Trb->Adma64V3Desc = AdmaDesc;
  } else {
    Trb->Adma64V4Desc = AdmaDesc;
  }

  for (Index = 0; Index < Entries; Index++) {
    if (Trb->Mode == SdMmcAdma32bMode) {
      if (Remaining <= AdmaMaxDataPerLine) {
        Trb->Adma32Desc[Index].Valid = 1;
        Trb->Adma32Desc[Index].Act   = 2;
        if (Trb->AdmaLengthMode == SdMmcAdmaLen26b) {
          Trb->Adma32Desc[Index].UpperLength = (UINT16)RShiftU64 (Remaining, 16);
        }
        Trb->Adma32Desc[Index].LowerLength = (UINT16)(Remaining & MAX_UINT16);
        Trb->Adma32Desc[Index].Address = (UINT32)Address;
        break;
      } else {
        Trb->Adma32Desc[Index].Valid = 1;
        Trb->Adma32Desc[Index].Act   = 2;
        if (Trb->AdmaLengthMode == SdMmcAdmaLen26b) {
          Trb->Adma32Desc[Index].UpperLength  = 0;
        }
        Trb->Adma32Desc[Index].LowerLength  = 0;
        Trb->Adma32Desc[Index].Address = (UINT32)Address;
      }
    } else if (Trb->Mode == SdMmcAdma64bV3Mode) {
      if (Remaining <= AdmaMaxDataPerLine) {
        Trb->Adma64V3Desc[Index].Valid = 1;
        Trb->Adma64V3Desc[Index].Act   = 2;
        if (Trb->AdmaLengthMode == SdMmcAdmaLen26b) {
          Trb->Adma64V3Desc[Index].UpperLength  = (UINT16)RShiftU64 (Remaining, 16);
        }
        Trb->Adma64V3Desc[Index].LowerLength  = (UINT16)(Remaining & MAX_UINT16);
        Trb->Adma64V3Desc[Index].LowerAddress = (UINT32)Address;
        Trb->Adma64V3Desc[Index].UpperAddress = (UINT32)RShiftU64 (Address, 32);
        break;
      } else {
        Trb->Adma64V3Desc[Index].Valid = 1;
        Trb->Adma64V3Desc[Index].Act   = 2;
        if (Trb->AdmaLengthMode == SdMmcAdmaLen26b) {
          Trb->Adma64V3Desc[Index].UpperLength  = 0;
        }
        Trb->Adma64V3Desc[Index].LowerLength  = 0;
        Trb->Adma64V3Desc[Index].LowerAddress = (UINT32)Address;
        Trb->Adma64V3Desc[Index].UpperAddress = (UINT32)RShiftU64 (Address, 32);
      }
    } else {
      if (Remaining <= AdmaMaxDataPerLine) {
        Trb->Adma64V4Desc[Index].Valid = 1;
        Trb->Adma64V4Desc[Index].Act   = 2;
        if (Trb->AdmaLengthMode == SdMmcAdmaLen26b) {
          Trb->Adma64V4Desc[Index].UpperLength  = (UINT16)RShiftU64 (Remaining, 16);
        }
        Trb->Adma64V4Desc[Index].LowerLength  = (UINT16)(Remaining & MAX_UINT16);
        Trb->Adma64V4Desc[Index].LowerAddress = (UINT32)Address;
        Trb->Adma64V4Desc[Index].UpperAddress = (UINT32)RShiftU64 (Address, 32);
        break;
      } else {
        Trb->Adma64V4Desc[Index].Valid = 1;
        Trb->Adma64V4Desc[Index].Act   = 2;
        if (Trb->AdmaLengthMode == SdMmcAdmaLen26b) {
          Trb->Adma64V4Desc[Index].UpperLength  = 0;
        }
        Trb->Adma64V4Desc[Index].LowerLength  = 0;
        Trb->Adma64V4Desc[Index].LowerAddress = (UINT32)Address;
        Trb->Adma64V4Desc[Index].UpperAddress = (UINT32)RShiftU64 (Address, 32);
      }
    }

    Remaining -= AdmaMaxDataPerLine;
    Address   += AdmaMaxDataPerLine;
  }

  //
  // Set the last descriptor line as end of descriptor table
  //
  if (Trb->Mode == SdMmcAdma32bMode) {
    Trb->Adma32Desc[Index].End = 1;
  } else if (Trb->Mode == SdMmcAdma64bV3Mode) {
    Trb->Adma64V3Desc[Index].End = 1;
  } else {
    Trb->Adma64V4Desc[Index].End = 1;
  }
  return EFI_SUCCESS;
}