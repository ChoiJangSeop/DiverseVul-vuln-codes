UsbHubCtrlGetSuperSpeedHubDesc (
  IN  USB_DEVICE          *HubDev,
  OUT VOID                *Buf
  )
{
  EFI_STATUS              Status;
  
  Status = EFI_INVALID_PARAMETER;
  
  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbDataIn,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_HUB,
             USB_HUB_REQ_GET_DESC,
             (UINT16) (USB_DESC_TYPE_HUB_SUPER_SPEED << 8),
             0,
             Buf,
             32
             );

  return Status;
}