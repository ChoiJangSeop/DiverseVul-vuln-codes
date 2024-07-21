UsbHubCtrlGetHubDesc (
  IN  USB_DEVICE          *HubDev,
  OUT VOID                *Buf,
  IN  UINTN               Len
  )
{
  EFI_STATUS              Status;

  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbDataIn,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_HUB,
             USB_HUB_REQ_GET_DESC,
             (UINT16) (USB_DESC_TYPE_HUB << 8),
             0,
             Buf,
             Len
             );

  return Status;
}