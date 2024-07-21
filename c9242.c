static int update_prepare_order_info(rdpContext* context, ORDER_INFO* orderInfo, UINT32 orderType)
{
	int length = 1;
	orderInfo->fieldFlags = 0;
	orderInfo->orderType = orderType;
	orderInfo->controlFlags = ORDER_STANDARD;
	orderInfo->controlFlags |= ORDER_TYPE_CHANGE;
	length += 1;
	length += PRIMARY_DRAWING_ORDER_FIELD_BYTES[orderInfo->orderType];
	length += update_prepare_bounds(context, orderInfo);
	return length;
}