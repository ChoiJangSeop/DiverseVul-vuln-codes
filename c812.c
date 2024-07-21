GnashImage::GnashImage(size_t width, size_t height, ImageType type,
        ImageLocation location)
    :
    _type(type),
    _location(location),
    _width(width),
    _height(height)
{
    const size_t max = std::numeric_limits<boost::int32_t>::max();
    if (size() > max) {
        throw std::bad_alloc();
    }
    _data.reset(new value_type[size()]);
}