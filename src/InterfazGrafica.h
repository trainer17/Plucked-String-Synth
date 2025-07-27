#pragma once
#include "aeffguieditor.h"
#include "Parametros.h"

class Interfaz : public AEffGUIEditor , CControlListener
{
public:
    Interfaz (AudioEffect*);

    // from AEffGUIEditor
    bool open(void* ptr);
    void close();
    void setParameter(VstInt32 index, float value);

    // from CControlListener
    void valueChanged(CControl* pControl);

protected:
    CControl* controls[kParam]; 
    CControl* displays[kParam]; 
};


