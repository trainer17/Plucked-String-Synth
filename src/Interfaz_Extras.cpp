#pragma once
#include "InterfazGrafica.h"
//-----------------------------------------------------------------------------------
//-- This is the custom CKnob
//-- It takes another bitmap as parameter in its constructor which is the highlited handle
//-----------------------------------------------------------------------------------
//No usado

class MyKnob : public CKnob
{
public:
	MyKnob(const CRect& size, CControlListener* listener, long tag, CBitmap* background, CBitmap* handle, CBitmap* highlightHandle);
	~MyKnob();

	CMouseEventResult onMouseDown(CPoint& where, const long& buttons);
	CMouseEventResult onMouseUp(CPoint& where, const long& buttons);
	CMouseEventResult onMouseEntered(CPoint& where, const long& buttons);
	CMouseEventResult onMouseExited(CPoint& where, const long& buttons);
protected:
	CBitmap* handleBitmap;
	CBitmap* highlightHandleBitmap;
};