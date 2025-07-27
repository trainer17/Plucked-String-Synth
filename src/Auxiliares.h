#pragma once
//La gran explicacion de por que no se hace include a un .cpp
//https://stackoverflow.com/questions/333889/why-have-header-files-and-cpp-files/333964#333964
#include "Sinewave.h"
float randomSample();
float randomSamplePositivo();
float calcularR(float G, float PIFREQSR);
void MyGroovyConvert(float value, char* /*out*/ string);
void mostrarTipo(float value, char* /*out*/ string);
void mostrarTipo2(float value, char* /*out*/ string);
void mostrarVocesPrendidas(float value, char* /*out*/ string);