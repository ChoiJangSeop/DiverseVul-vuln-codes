UsbHubReadDesc (
  IN  USB_DEVICE              *HubDev,
  OUT EFI_USB_HUB_DESCRIPTOR  *HubDesc
  )
{
  EFI_STATUS              Status;

  if (HubDev->Speed == EFI_USB_SPEED_SUPER) {
    //
    // Get the super speed hub descriptor
    //
    Status = UsbHubCtrlGetSuperSpeedHubDesc (HubDev, HubDesc);
  } else {

    //
    // First get the hub descriptor length
    //
    Status = UsbHubCtrlGetHubDesc (HubDev, HubDesc, 2);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Get the whole hub descriptor
    //
    Status = UsbHubCtrlGetHubDesc (HubDev, HubDesc, HubDesc->Length);
  }

  return Status;
}