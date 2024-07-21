ElectronBrowserHandlerImpl::ElectronBrowserHandlerImpl(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronBrowser> receiver)
    : render_process_id_(frame_host->GetProcess()->GetID()),
      render_frame_id_(frame_host->GetRoutingID()) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  DCHECK(web_contents);
  content::WebContentsObserver::Observe(web_contents);

  receiver_.Bind(std::move(receiver));
  receiver_.set_disconnect_handler(base::BindOnce(
      &ElectronBrowserHandlerImpl::OnConnectionError, GetWeakPtr()));
}