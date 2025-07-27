#pragma once
/*				RANDOM				*/
#include <random>
#include <stdio.h>
#include "Sinewave.h"
std::default_random_engine rand_generator;
std::uniform_real_distribution<float> distribution(-1, 1); //floats from -1 to 1
std::uniform_real_distribution<float> distributionPositiva(0, 1); //floats from -1 to 1

// se llama asi: float random = distribution(rand_generator);
float randomSample() { return distribution(rand_generator); }
float randomSamplePositivo() { return distributionPositiva(rand_generator); }


float calcularR(float G, float PIFREQSR) {

	float G2 = G * G;
	float termino1 = 1 - G2 * cos(2 * PIFREQSR);
	termino1 /= 1 - G2;

	float termino2 = 2 * G * sin(PIFREQSR) * sqrt(1 - G2 * cos(PIFREQSR) * cos(PIFREQSR));
	termino2 /= 1 - G2;

	if (abs(termino1 + termino2) < 1.)
		return termino1 + termino2;
	else
		return termino1 - termino2;

}

//Opcional - Podria no usarse
//Usada en el display numerico de los parametros
void MyGroovyConvert(float value, char* /*out*/ string) {
	sprintf(string, "% .2f", value);
}

void mostrarTipo(float value, char* /*out*/ string) {
	if (value == 0.)  sprintf(string, "MONOCORDIO");
	else if (value == 0.5)  sprintf(string, "BICORDIO");
	else /* value ==1. */ sprintf(string, "TRICORDIO");
}


void mostrarTipo2(float value, char* /*out*/ string) {
	if (value ==0.)  sprintf(string, "LET RING");
	else if (value == 0.5)  sprintf(string, "HAMMER ON / PULL OF");
	else /* value ==1. */ sprintf(string, "SLIDE");
}

void mostrarVocesPrendidas(float value, char* /*out*/ string) {
	sprintf(string, "%i / 8", (int) value);
}

void Voz::calcularC(float sr) {

	//float Paux = sr / voz.freq - p - 0.5; //Valido solo para cuando uso el promedio de dos puntos y s=1/2. Para otros más complejos cambia el ultimo término
	//Omega = 2pi*freq
	float wT = TWOPI * freq / sr;
	float Ps = -1 / wT * atan(-S * sin(wT) / ((1 - S) + S * cos(wT)));
	float N = floor(sr / freq - Ps - eps);
	float Pc = sr / freq - N - Ps; //Valido solo para cuando uso el promedio de dos puntos. Para otros más complejos cambia el ultimo término
	wT /= 2.;
	C = sin(wT * (1 - Pc)) / sin(wT * (1 + Pc));      //exacto .    
	
	//voz.C = (1 - Pc) / (1 + Pc);						//aproximado para bajas frecuencias
	//C deberia ser menor a 1, para estabilidad. NO está ocurriendome...
	//Todo: Esto se tiene que actualizar junto con S
}


void Voz::cargarVoz(VstInt32 note, VstInt32 vel, VstInt32 chann, VstInt32 deltaframes) {
	freq = 440.0 * pow(2.0, (note - 69) / 12.0); //Hz
	p = padre->sr / freq;
	voiceNote = note;
	velocity = vel;
	blocki = deltaframes;
	channel = chann;
	on = true;
	apagandose = false;
	pickPosition = padre->pickPos;
	character = padre->character;
	b = padre->b;
	S = padre->S;
	rho = padre->rho;
	if (PARENTLETRING) precalcSize = 10 * padre->dawRequest;
	if (precalcSize < p) precalcSize = p;
	if (precalcSize > padre->maxbufsize) precalcSize = padre->maxbufsize;
}
