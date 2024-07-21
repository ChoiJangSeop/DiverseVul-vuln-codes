void BindElectronBrowser(
    mojo::PendingAssociatedReceiver<electron::mojom::ElectronBrowser> receiver,
    content::RenderFrameHost* frame_host) {
  ElectronBrowserHandlerImpl::Create(frame_host, std::move(receiver));
}