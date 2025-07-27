#pragma once
#include "InterfazGrafica.h"
#include "Interfaz_Extras.cpp"
#include "Auxiliares.h"  // Para la funcion "groovyConvert", que pasa de float a string para imprimir el valor del param en el display. Estrictamente no hace falta, porque se hace ppor defecto


//----------------------------------------------------------------------------------
//                              CONSTRUCTORES
//----------------------------------------------------------------------------------

AEffGUIEditor* createEditor(AudioEffectX* effect)
{
    return new Interfaz(effect);
}

Interfaz::Interfaz(AudioEffect* ptr)
    : AEffGUIEditor(ptr)
{
}


/*              AUXILIARES                  */

CParamDisplay* crearDisplay(CFrame* frame, CRect size) {
	
	CParamDisplay* display = new CParamDisplay(size, 0, kCenterText);  //Definido en InterfazGrafica.h
	display->setFont(kNormalFontSmall);
	display->setFontColor(kBlackCColor);
	display->setBackColor(kWhiteCColor);
	display->setFrameColor(kGreyCColor);
	display->setStringConvert(&MyGroovyConvert); //no hace falta, se hace por defecto
	frame->addView(display);
	return display;
}


//----------------------------------------------------------------------------------
//                              VENTANA PPAL
//----------------------------------------------------------------------------------
bool Interfaz::open(void* ptr)
{
	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// Cosas que voy a reutilizar bastante
	//Displays
	int xi, yi, xf, yf;
	int xi_display, yi_display;
	int width_display, height_display;
	//Rectangulo (Donde van los objetos. Se define por posicion de la esquina izq y derecha)
	CRect rect;

	//--init background frame-----------------------------------------------
	//-- first we create the frame with a size of 300, 300 and set the background to some color
	// We use a local CFrame object so that calls to setParameter won't call into objects which may not exist yet. 
	// If all GUI objects are created we assign our class member to this one. See bottom of this method.
	CRect frameSize(0, 0, 485, 310);
	CFrame* newFrame = new CFrame(frameSize, ptr, this);
	newFrame->setBackgroundColor(kGreyCColor);
	newFrame->setSize(485, 310); //Por las dudas

	// Load Background Image
	// Importante: Todas las imagenes que use deben estar definidas en "sine.rc". Sino, no van a cargarse
	CBitmap* vst_background= new CBitmap("wood.png");

	
	CView* myView = new CView(CRect(0, 0, vst_background->getWidth(), vst_background->getHeight()));
	myView->setBackground(vst_background);
	newFrame->addView(myView);
	vst_background->forget();


	//------------------------	
	// KNOBS
	//------------------------	
	//-- creating a knob and adding it to the frame
	
	//-- load some bitmaps we need
	CBitmap* knob_background = new CBitmap("knobRoSin3.png");
	CBitmap* knob_handle = new CBitmap("handleSmall2.png");

	//Rectangulo del tamaño del knob
	rect(0, 0, knob_background->getWidth(), knob_background->getHeight());
	xi = newFrame->getWidth() - knob_background->getWidth() - 5;  //Knobs arriba a la derecha
	yi = 50;
	

	//-- creating another knob, we are offsetting the rect, so that the knob is next to the previous knob
	// PARAMETRO: vibRate
	rect.offset(xi, yi);
	CKnob* knob2 = new CKnob(rect, this, kvibRate, knob_background, knob_handle, CPoint(0, 0));
	newFrame->addView(knob2);
	knob2->setValue(effect->getParameter(kvibRate));


	// PARAMETRO: vibAmt
	rect.offset(0, knob_background->getHeight()+10); //Abajo del otro
	CKnob* knob3 = new CKnob(rect, this, kvibAmt, knob_background, knob_handle, CPoint(0, 0));
	newFrame->addView(knob3);
	knob3->setValue(effect->getParameter(kvibAmt));



	// PARAMETRO: Character
	rect.offset(0, knob_background->getHeight() + 10); //Abajo del otro
	CKnob* knob4 = new CKnob(rect, this, kCharacter, knob_background, knob_handle, CPoint(0, 0));
	newFrame->addView(knob4);
	knob4->setValue(effect->getParameter(kCharacter));


	//-- forget the bitmaps
	knob_background->forget();
	knob_handle->forget();


	//------------------------	
	// VSLIDERS
	//------------------------	

	CBitmap* vslider_body = new CBitmap("vsliderSlot.png");
	CBitmap* vslider_handle = new CBitmap("vsliderHandle.png");

	//--coordenadas------------------------------------------------
	xi = 190;  
	yi = 20;
	yf = yi + vslider_body->getHeight() - vslider_handle->getHeight()  - 1; //Posicion final del HANDLE del slider (sobre el canvas del vst)
	int dx = 40;
	xf = xi + vslider_body->getWidth();

	rect(xi, yi, xf, yi + vslider_body->getHeight()); //Caja para todo el conjunto
	
	width_display = 30;
	height_display = 20;

	// 1- PARAMETRO:  b
	CVerticalSlider* bSlider = new CVerticalSlider(rect, this, kb, yi, yf, vslider_handle, vslider_body, CPoint(0, 1));
	bSlider->setOffsetHandle(CPoint(0, 0));
	bSlider->setValue(effect->getParameter(kb)); //valor inicial
	newFrame->addView(bSlider);


	// DISPLAY DEL VALOR
	// Justo en el centro y debajo del slider
	xi_display = xi- vslider_body->getWidth()-4;
	yi_display = yi + vslider_body->getHeight()+5;

	rect(xi_display, yi_display, xi_display + width_display, yi_display + height_display);
	CParamDisplay* bDisplay = crearDisplay(newFrame, rect);
	bDisplay->setValue(effect->getParameter(kb)); //linkear display al valor inicial del parámetro


	// 2- PARAMETRO: decay (S)
	//rect.offset(dx, 0); respecto del otro slider
	xi += dx;
	xi_display += dx;
	xf += dx;

	rect(xi, yi,xf, yi + vslider_body->getHeight()); 
	CVerticalSlider* decaySlider = new CVerticalSlider(rect, this, kS, yi, yf, vslider_handle, vslider_body, CPoint(0, 1));
	decaySlider->setOffsetHandle(CPoint(0, 0));
	decaySlider->setValue(effect->getParameter(kS));
	newFrame->addView(decaySlider);

	rect(xi_display, yi_display, xi_display + width_display, yi_display + height_display);
	CParamDisplay* decayDisplay = crearDisplay(newFrame, rect);
	decayDisplay->setValue(effect->getParameter(kS));

	// 3- PARAMETRO: dampening
	//rect.offset(20, 0); respecto del otro slider
	xi += dx;
	xi_display += dx;
	xf += dx;

	rect(xi, yi, xf, yi + vslider_body->getHeight());
	CVerticalSlider* dampeningSlider = new CVerticalSlider(rect, this, kdampening, yi, yf, vslider_handle, vslider_body, CPoint(0, 1));
	dampeningSlider->setOffsetHandle(CPoint(0, 0));
	dampeningSlider->setValue(effect->getParameter(kdampening));
	newFrame->addView(dampeningSlider);

	rect(xi_display, yi_display, xi_display + width_display, yi_display + height_display);
	CParamDisplay* dampeningDisplay = crearDisplay(newFrame, rect);
	dampeningDisplay->setValue(effect->getParameter(kdampening));



	//Listo vsliders
	vslider_handle->forget();
	vslider_body->forget();

	//------------------------	
	// HSLIDERS
	//------------------------	

	//Este codigo, y el de los displays, tomado de acá:
	//https://www.kvraudio.com/forum/viewtopic.php?t=279247

	// PARAMETRO: pickpos
	CBitmap* hslider_handle = new CBitmap("picksmall.png");
	CBitmap* hslider_body= new CBitmap("stringLong3.png");
	//CHorizontalSlider(const CRect & size, CControlListener * listener, long tag, long iMinPos, long iMaxPos, CBitmap * handle, CBitmap * background, CPoint & offset, const long style = kRight)

	//--coordenadas------------------------------------------------
	xi = 0;  //Posicion inicial del slider en x (sobre la ventana del vst)
	yi = 230; //Posicion inicial del slider en y (sobre la ventana del vst)
	//int xf = xi + hslider_body->getWidth() - hslider_handle->getWidth() - 1;  Clásico, para que se quede dentro del slider. Pero acá quiero que llegue la punta de la púa hasta el finaal
	xf = xi + hslider_body->getWidth() -hslider_handle->getWidth()/2 - 15; //Posicion final del HANDLE del slider (sobre el canvas del vst)


	rect(xi, yi, xi + hslider_body->getWidth(), yi + hslider_body->getHeight());
	CHorizontalSlider* hslider_pick = new CHorizontalSlider(rect, this,kpickPos, 0, xf, hslider_handle, hslider_body, CPoint(0, 1)); 
	hslider_pick->setOffsetHandle(CPoint(0,0));
	hslider_pick->setValue(effect->getParameter(kpickPos));
	newFrame->addView(hslider_pick);
	hslider_handle->forget();
	hslider_body->forget();
	
	// DISPLAY DEL VALOR
	xi_display = newFrame->getWidth()/2-15; //Pongo el display justo en el centro y debajo del slider
	yi_display = yi + hslider_body->getHeight()/2+10;
	width_display = 30;
	height_display = 20;

	rect(xi_display, yi_display,	xi_display + width_display, yi_display + height_display);
	CParamDisplay* pickDisplay = crearDisplay(newFrame, rect);
	pickDisplay->setValue(effect->getParameter(kpickPos)); //linkear display al valor inicial del parámetro

														   
	//------------------------	
	// SWITCHES
	//------------------------	
	//Ejemplo y explicacion de switch y boton:https://www.kvraudio.com/forum/viewtopic.php?t=314152


	// PARAMETRO: type  (cantidad de cuerdas)
	CBitmap* switch_img = new CBitmap("triSwitch.png");
	rect(0, 0, switch_img->getWidth(), switch_img->getHeight()/3);
	rect.offset(125, 210);		//Posicion
//	CHorizontalSwitch* switch1 = new CHorizontalSwitch(rect, this, kType, switch_img, CPoint(0, 0));  No cambia la imagen así
	CHorizontalSwitch* switch1 = new CHorizontalSwitch(rect, this, kType, 3, switch_img->getHeight()/3, 3,switch_img,  CPoint(0, 0));
	switch1->setValue(effect->getParameter(kType));
	newFrame->addView(switch1);
	switch_img->forget();

	//DISPLAY DEL TIPO AL LADO DEL SWITCH
	width_display = 110; //Largo porque es texto
	height_display = 20;
	xi_display = rect.getTopLeft().x - 5 - width_display; //Pongo el display justo a la izquierda del switch
	yi_display = rect.getTopRight().y - 4;


	rect(xi_display, yi_display, xi_display + width_display, yi_display + height_display);
	CParamDisplay* typeDisplay = crearDisplay(newFrame, rect);
	typeDisplay->setValue(effect->getParameter(kType)); //linkear display al valor inicial del parámetro
	typeDisplay->setStringConvert(&mostrarTipo); //no hace falta, se hace por defecto


	// PARAMETRO: type2 (Ligar o let ring)
	CBitmap* triSwitch_img = new CBitmap("triSwitch.png");
	rect(0, 0, triSwitch_img->getWidth(), triSwitch_img->getHeight() / 3);
	rect.offset(310, 210);  //Abajo de los knobs
	CHorizontalSwitch* triswitch = new CHorizontalSwitch(rect, this, kType2, 3, triSwitch_img->getHeight()/3, 3, triSwitch_img, CPoint(0, 0));
	triswitch->setValue(effect->getParameter(kType2));
	newFrame->addView(triswitch);
	triSwitch_img->forget();

	//DISPLAY
	xi_display = rect.getTopRight().x + 5; //Pongo el display justo a la derecha del switch
	yi_display = rect.getTopRight().y - 4;
	width_display = 130; //Largo porque es texto
	height_display = 20;

	rect(xi_display, yi_display, xi_display + width_display, yi_display + height_display);
	CParamDisplay* type2Display = crearDisplay(newFrame, rect);
	type2Display->setValue(effect->getParameter(kType2)); //linkear display al valor inicial del parámetro
	type2Display->setStringConvert(&mostrarTipo2); //no hace falta, se hace por defecto



	//------------------------	
	// BOTON
	//------------------------	


	//MUTE
	CBitmap* button_img = new CBitmap("muteSmall.png");
	COnOffButton* mute_button;
	xi = newFrame->getWidth() - button_img->getWidth();   //Arriba a la derecha
	yi = 0;
	rect(xi, yi, (xi + button_img->getWidth()), (yi + button_img->getHeight() / 2));
	mute_button = new COnOffButton(rect, this, kmute, button_img);
	mute_button->setValue(effect->getParameter(kmute));
	mute_button->setTransparency(0);
	newFrame->addView(mute_button);
	button_img->forget();

	//RESET
	button_img = new CBitmap("reset.png");
	COnOffButton* reset_button;
	xi = newFrame->getWidth() - button_img->getWidth() - 30;   //Abajo a la derecha
	yi = newFrame->getHeight() - button_img->getWidth();
	rect(xi, yi, (xi + button_img->getWidth()), (yi + button_img->getHeight()/2));
	reset_button = new COnOffButton(rect, this, kReset, button_img);
	reset_button->setValue(effect->getParameter(kReset));
	reset_button->setTransparency(0);
	newFrame->addView(reset_button);
	button_img->forget();

	//Todo: agregar nombre de cada parámetro en la barra gris de abajo al hacer hover!!

	//------------------------	
	// DISPLAY CONTROL
	//------------------------	

	//DISPLAY
	width_display = 30; 
	height_display = 20;
	xi_display = newFrame->getWidth() - width_display; //Pongo el display abajo a la derecha de todo
	yi_display = newFrame->getHeight() - height_display;

	rect(xi_display, yi_display, xi_display + width_display, yi_display + height_display);
	CParamDisplay* onVoicesDisplay = crearDisplay(newFrame, rect);
	onVoicesDisplay->setValue(0); //linkear display al valor inicial del parámetro
	onVoicesDisplay->setStringConvert(&mostrarVocesPrendidas); //no hace falta, se hace por defecto



	///////////////////////////////
	// ///////////////////////////
	// 
	//-- remember our controls so that we can sync them with the state of the effect
	controls[kb] = bSlider;
	controls[kS] = decaySlider;
	controls[kdampening] = dampeningSlider;
	controls[kvibRate] = knob2;
	controls[kvibAmt] = knob3;
	controls[kType] = switch1;
	controls[kpickPos] = hslider_pick;
	controls[kmute] = mute_button;
	controls[kType2] = triswitch;
	controls[kCharacter] = knob4;
	controls[kReset] = reset_button;

	displays[kb] = bDisplay;
	displays[kS] = decayDisplay;
	displays[kdampening] = dampeningDisplay;
	displays[kType] = typeDisplay;
	displays[kType2] = type2Display;
	displays[kpickPos] = pickDisplay;
	displays[kOnVoices] = onVoicesDisplay;


	//-- set the member frame to the newly created frame
	//-- we do it this way because it is possible that the setParameter method is called 
	//-- in between of constructing the frame and it's controls
	frame = newFrame;

	//-- sync parameters
	for (int i = 0; i < 0; i++)
		setParameter(i, effect->getParameter(i));

	return true;



}

void Interfaz::close()
{
	//-- on close we need to delete the frame object.
	//-- once again we make sure that the member frame variable is set to zero before we delete it
	//-- so that calls to setParameter won't crash.
	CFrame* oldFrame = frame;
	frame = 0;
	delete oldFrame;
}


//-----------------------------------------------------------------------------------
//                                  CONTROL
//-----------------------------------------------------------------------------------
void Interfaz::valueChanged(CControl* pControl)
{
    //-- valueChanged is called whenever the user changes one of the controls in the User Interface (UI)
    effect->setParameterAutomated(pControl->getTag(), pControl->getValue());
	//effect->setParameter(pControl->getTag(), pControl->getValue());
}
//------------------------------------------------------------------------------------
void Interfaz::setParameter(VstInt32 index, float value) //NUNCA ES LLAMADA, aparentemente. La pisa KPS::setValue()
{
	//-- setParameter is called when the host automates one of the effects parameter.
	//-- The UI should reflect this state so we set the value of the control to the new value.
	//-- VSTGUI will automaticly redraw changed controls in the next idle (as this call happens to be in the process thread).
	if (frame && index < kParam)
	{

		controls[index]->setValue(value);
		if (displays[index]) displays[index]->setValue(value); //Los knobs no tienen display
	}

	if (index == kOnVoices) displays[kOnVoices]->setValue(value);
}

// assume you have a CParamDisplay* my_param_display; somewhere
// and after you "new" it, do this:
