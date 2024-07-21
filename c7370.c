PeiDoHubConfig (
  IN EFI_PEI_SERVICES    **PeiServices,
  IN PEI_USB_DEVICE      *PeiUsbDevice
  )
{
  EFI_USB_HUB_DESCRIPTOR  HubDescriptor;
  EFI_STATUS              Status;
  EFI_USB_HUB_STATUS      HubStatus;
  UINTN                   Index;
  PEI_USB_IO_PPI          *UsbIoPpi;

  ZeroMem (&HubDescriptor, sizeof (HubDescriptor));
  UsbIoPpi = &PeiUsbDevice->UsbIoPpi;

  //
  // Get the hub descriptor 
  //
  Status = PeiUsbHubReadDesc (
            PeiServices,
            PeiUsbDevice,
            UsbIoPpi,
            &HubDescriptor
            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  PeiUsbDevice->DownStreamPortNo = HubDescriptor.NbrPorts;

  if (PeiUsbDevice->DeviceSpeed == EFI_USB_SPEED_SUPER) {
    DEBUG ((EFI_D_INFO, "PeiDoHubConfig: Set Hub Depth as 0x%x\n", PeiUsbDevice->Tier));
    PeiUsbHubCtrlSetHubDepth (
      PeiServices,
      PeiUsbDevice,
      UsbIoPpi
      );
  } else {
    //
    //  Power all the hub ports
    //
    for (Index = 0; Index < PeiUsbDevice->DownStreamPortNo; Index++) {
      Status = PeiHubSetPortFeature (
                PeiServices,
                UsbIoPpi,
                (UINT8) (Index + 1),
                EfiUsbPortPower
                );
      if (EFI_ERROR (Status)) {
        DEBUG (( EFI_D_ERROR, "PeiDoHubConfig: PeiHubSetPortFeature EfiUsbPortPower failed %x\n", Index));
        continue;
      }
    }

    DEBUG (( EFI_D_INFO, "PeiDoHubConfig: HubDescriptor.PwrOn2PwrGood: 0x%x\n", HubDescriptor.PwrOn2PwrGood));
    if (HubDescriptor.PwrOn2PwrGood > 0) {
      MicroSecondDelay (HubDescriptor.PwrOn2PwrGood * USB_SET_PORT_POWER_STALL);
    }

    //
    // Clear Hub Status Change
    //
    Status = PeiHubGetHubStatus (
              PeiServices,
              UsbIoPpi,
              (UINT32 *) &HubStatus
              );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    } else {
      //
      // Hub power supply change happens
      //
      if ((HubStatus.HubChangeStatus & HUB_CHANGE_LOCAL_POWER) != 0) {
        PeiHubClearHubFeature (
          PeiServices,
          UsbIoPpi,
          C_HUB_LOCAL_POWER
          );
      }
      //
      // Hub change overcurrent happens
      //
      if ((HubStatus.HubChangeStatus & HUB_CHANGE_OVERCURRENT) != 0) {
        PeiHubClearHubFeature (
          PeiServices,
          UsbIoPpi,
          C_HUB_OVER_CURRENT
          );
      }
    }
  }

  return EFI_SUCCESS;
}