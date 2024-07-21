void ElectronRenderFrameObserver::DraggableRegionsChanged() {
  blink::WebVector<blink::WebDraggableRegion> webregions =
      render_frame_->GetWebFrame()->GetDocument().DraggableRegions();
  std::vector<mojom::DraggableRegionPtr> regions;
  for (auto& webregion : webregions) {
    auto region = mojom::DraggableRegion::New();
    render_frame_->ConvertViewportToWindow(&webregion.bounds);
    region->bounds = webregion.bounds;
    region->draggable = webregion.draggable;
    regions.push_back(std::move(region));
  }

  mojo::Remote<mojom::ElectronBrowser> browser_remote;
  render_frame_->GetBrowserInterfaceBroker()->GetInterface(
      browser_remote.BindNewPipeAndPassReceiver());
  browser_remote->UpdateDraggableRegions(std::move(regions));
}