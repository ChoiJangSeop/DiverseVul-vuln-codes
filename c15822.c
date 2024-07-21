bool ElectronBrowserClient::BindAssociatedReceiverFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle* handle) {
  if (interface_name == mojom::ElectronAutofillDriver::Name_) {
    AutofillDriverFactory::BindAutofillDriver(
        mojom::ElectronAutofillDriverAssociatedRequest(std::move(*handle)),
        render_frame_host);
    return true;
  }
#if BUILDFLAG(ENABLE_PRINTING)
  if (interface_name == printing::mojom::PrintManagerHost::Name_) {
    mojo::PendingAssociatedReceiver<printing::mojom::PrintManagerHost> receiver(
        std::move(*handle));
    PrintViewManagerElectron::BindPrintManagerHost(std::move(receiver),
                                                   render_frame_host);
    return true;
  }
#endif
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (interface_name == extensions::mojom::LocalFrameHost::Name_) {
    extensions::ExtensionWebContentsObserver::BindLocalFrameHost(
        mojo::PendingAssociatedReceiver<extensions::mojom::LocalFrameHost>(
            std::move(*handle)),
        render_frame_host);
    return true;
  }
#endif

  return false;
}