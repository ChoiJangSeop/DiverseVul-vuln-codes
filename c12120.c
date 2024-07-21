void ElectronBrowserHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronBrowser> receiver) {
  new ElectronBrowserHandlerImpl(frame_host, std::move(receiver));
}