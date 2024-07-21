static int update_write_order_info(rdpContext* context, wStream* s, ORDER_INFO* orderInfo,
                                   size_t offset)
{
	size_t position;
	WINPR_UNUSED(context);
	position = Stream_GetPosition(s);
	Stream_SetPosition(s, offset);
	Stream_Write_UINT8(s, orderInfo->controlFlags); /* controlFlags (1 byte) */

	if (orderInfo->controlFlags & ORDER_TYPE_CHANGE)
		Stream_Write_UINT8(s, orderInfo->orderType); /* orderType (1 byte) */

	update_write_field_flags(s, orderInfo->fieldFlags, orderInfo->controlFlags,
	                         PRIMARY_DRAWING_ORDER_FIELD_BYTES[orderInfo->orderType]);
	update_write_bounds(s, orderInfo);
	Stream_SetPosition(s, position);
	return 0;
}