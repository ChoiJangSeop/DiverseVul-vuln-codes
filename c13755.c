void BindElectronBrowser(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<electron::mojom::ElectronBrowser> receiver) {
  ElectronBrowserHandlerImpl::Create(frame_host, std::move(receiver));
}