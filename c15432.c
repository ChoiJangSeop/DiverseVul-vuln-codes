static void stellaris_enet_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = stellaris_enet_init;
    dc->unrealize = stellaris_enet_unrealize;
    dc->props = stellaris_enet_properties;
}