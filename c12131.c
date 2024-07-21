void ElectronBrowserHandlerImpl::UpdateDraggableRegions(
    std::vector<mojom::DraggableRegionPtr> regions) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->UpdateDraggableRegions(std::move(regions));
  }
}