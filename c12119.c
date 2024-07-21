void ElectronRenderFrameObserver::DidMeaningfulLayout(
    blink::WebMeaningfulLayout layout_type) {
  if (layout_type == blink::WebMeaningfulLayout::kVisuallyNonEmpty) {
    mojo::AssociatedRemote<mojom::ElectronBrowser> browser_remote;
    render_frame_->GetRemoteAssociatedInterfaces()->GetInterface(
        &browser_remote);
    browser_remote->OnFirstNonEmptyLayout();
  }
}