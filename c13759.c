void ElectronRenderFrameObserver::DidMeaningfulLayout(
    blink::WebMeaningfulLayout layout_type) {
  if (layout_type == blink::WebMeaningfulLayout::kVisuallyNonEmpty) {
    mojo::Remote<mojom::ElectronBrowser> browser_remote;
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        browser_remote.BindNewPipeAndPassReceiver());
    browser_remote->OnFirstNonEmptyLayout();
  }
}