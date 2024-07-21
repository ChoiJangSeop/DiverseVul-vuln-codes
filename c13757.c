void ElectronBrowserHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<mojom::ElectronBrowser> receiver) {
  new ElectronBrowserHandlerImpl(frame_host, std::move(receiver));
}