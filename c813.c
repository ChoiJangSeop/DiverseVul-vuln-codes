GnashImage::GnashImage(iterator data, size_t width, size_t height,
        ImageType type, ImageLocation location)
    :
    _type(type),
    _location(location),
    _width(width),
    _height(height),
    _data(data)
{
}