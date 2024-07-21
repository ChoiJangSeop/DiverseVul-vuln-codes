UsbHubInit (
  IN USB_INTERFACE        *HubIf
  )
{
  EFI_USB_HUB_DESCRIPTOR  HubDesc;
  USB_ENDPOINT_DESC       *EpDesc;
  USB_INTERFACE_SETTING   *Setting;
  EFI_USB_IO_PROTOCOL     *UsbIo;
  USB_DEVICE              *HubDev;
  EFI_STATUS              Status;
  UINT8                   Index;
  UINT8                   NumEndpoints;
  UINT16                  Depth;

  //
  // Locate the interrupt endpoint for port change map
  //
  HubIf->IsHub  = FALSE;
  Setting       = HubIf->IfSetting;
  HubDev        = HubIf->Device;
  EpDesc        = NULL;
  NumEndpoints  = Setting->Desc.NumEndpoints;

  for (Index = 0; Index < NumEndpoints; Index++) {
    ASSERT ((Setting->Endpoints != NULL) && (Setting->Endpoints[Index] != NULL));

    EpDesc = Setting->Endpoints[Index];

    if (USB_BIT_IS_SET (EpDesc->Desc.EndpointAddress, USB_ENDPOINT_DIR_IN) &&
       (USB_ENDPOINT_TYPE (&EpDesc->Desc) == USB_ENDPOINT_INTERRUPT)) {
      break;
    }
  }

  if (Index == NumEndpoints) {
    DEBUG (( EFI_D_ERROR, "UsbHubInit: no interrupt endpoint found for hub %d\n", HubDev->Address));
    return EFI_DEVICE_ERROR;
  }

  Status = UsbHubReadDesc (HubDev, &HubDesc);

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "UsbHubInit: failed to read HUB descriptor %r\n", Status));
    return Status;
  }

  HubIf->NumOfPort = HubDesc.NumPorts;

  DEBUG (( EFI_D_INFO, "UsbHubInit: hub %d has %d ports\n", HubDev->Address,HubIf->NumOfPort));

  //
  // OK, set IsHub to TRUE. Now usb bus can handle this device
  // as a working HUB. If failed eariler, bus driver will not
  // recognize it as a hub. Other parts of the bus should be able
  // to work.
  //
  HubIf->IsHub  = TRUE;
  HubIf->HubApi = &mUsbHubApi;
  HubIf->HubEp  = EpDesc;

  if (HubIf->Device->Speed == EFI_USB_SPEED_SUPER) {
    Depth = (UINT16)(HubIf->Device->Tier - 1);
    DEBUG ((EFI_D_INFO, "UsbHubInit: Set Hub Depth as 0x%x\n", Depth));
    UsbHubCtrlSetHubDepth (HubIf->Device, Depth);
    
    for (Index = 0; Index < HubDesc.NumPorts; Index++) {
      UsbHubCtrlSetPortFeature (HubIf->Device, Index, USB_HUB_PORT_REMOTE_WAKE_MASK);
    }    
  } else {
    //
    // Feed power to all the hub ports. It should be ok
    // for both gang/individual powered hubs.
    //
    for (Index = 0; Index < HubDesc.NumPorts; Index++) {
      UsbHubCtrlSetPortFeature (HubIf->Device, Index, (EFI_USB_PORT_FEATURE) USB_HUB_PORT_POWER);
    }

    //
    // Update for the usb hub has no power on delay requirement
    //
    if (HubDesc.PwrOn2PwrGood > 0) {
      gBS->Stall (HubDesc.PwrOn2PwrGood * USB_SET_PORT_POWER_STALL);
    }
    UsbHubAckHubStatus (HubIf->Device);
  }

  //
  // Create an event to enumerate the hub's port. On
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  UsbHubEnumeration,
                  HubIf,
                  &HubIf->HubNotify
                  );

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "UsbHubInit: failed to create signal for hub %d - %r\n",
                HubDev->Address, Status));

    return Status;
  }

  //
  // Create AsyncInterrupt to query hub port change endpoint
  // periodically. If the hub ports are changed, hub will return
  // changed port map from the interrupt endpoint. The port map
  // must be able to hold (HubIf->NumOfPort + 1) bits (one bit for
  // host change status).
  //
  UsbIo  = &HubIf->UsbIo;
  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EpDesc->Desc.EndpointAddress,
                    TRUE,
                    USB_HUB_POLL_INTERVAL,
                    HubIf->NumOfPort / 8 + 1,
                    UsbOnHubInterrupt,
                    HubIf
                    );

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "UsbHubInit: failed to queue interrupt transfer for hub %d - %r\n",
                HubDev->Address, Status));

    gBS->CloseEvent (HubIf->HubNotify);
    HubIf->HubNotify = NULL;

    return Status;
  }

  DEBUG (( EFI_D_INFO, "UsbHubInit: hub %d initialized\n", HubDev->Address));
  return Status;
}