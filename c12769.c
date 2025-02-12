SWFShape_setLeftFillStyle(SWFShape shape, SWFFillStyle fill)
{
	ShapeRecord record;
	int idx;

	if ( shape->isEnded || shape->isMorph )
		return;
	
	if(fill == NOFILL)
	{
		record = addStyleRecord(shape);
		record.record.stateChange->leftFill = 0;
		record.record.stateChange->flags |= SWF_SHAPE_FILLSTYLE0FLAG;
		return;
	}

	idx = getFillIdx(shape, fill);
	if(idx == 0) // fill not present in array
	{
		SWFFillStyle_addDependency(fill, (SWFCharacter)shape);
		if(addFillStyle(shape, fill) < 0)
			return;		
		idx = getFillIdx(shape, fill);
	}
				
	record = addStyleRecord(shape);
	record.record.stateChange->leftFill = idx;
	record.record.stateChange->flags |= SWF_SHAPE_FILLSTYLE0FLAG;
}