#pragma once
#include "Sinewave.h"
#include "aeffguieditor.h"
#include <stdio.h>
#include <string.h>
#include "Parametros.h"

/*					PRINCIPAL - Creo una instancia de mi plugin								 */

AudioEffect* createEffectInstance(audioMasterCallback audioMaster) {
	return new KPS(audioMaster);
}



KPS* Voz::padre;
void Voz::inicializar() {

	p = 0; //Inicializo con 0's el delay de cada buffer
	si = 0; //Indico cual es el sample inicial
	on = false;
	apagandose = false;
	voiceNote = 0;
	ultima = 0;
	for (int sampi = 0; sampi < padre->maxbufsize; sampi++) buffer[sampi] = 0.; //Lleno de 0s el buffer

}


KPS::KPS(audioMasterCallback audiomaster) : AudioEffectX(audioMaster, 0, kParam)
//The last two arguments to AudioEffectX are the number of programs, and the number of parameters
{

	//PARTE GENERICA
	setNumInputs(0); //Por ahora
	setNumOutputs(1); //Mono a partir de la version 10
	setUniqueID('ASK2');
	canProcessReplacing(); //Genera sonido nuevo (accede al buffer y lo escribe)
	isSynth();
	programs = new KPSPreset[numPrograms];

	Voces    = new Voz[maxVoices];
	Voces2da = new Voz[maxVoices];
	Voces3ra = new Voz[maxVoices];

	//PARTE ESPECIFICA DE KPS - Inicializo los buffers de cada voz
	
	bufs    = new float[maxVoices * maxbufsize];
	bufs2da = new float[maxVoices * maxbufsize];
	bufs3ra = new float[maxVoices * maxbufsize];

	Voces[0].padre = this;

	for (int k = 0; k < maxVoices; k++)
	{
		Voz& voz = Voces[k];
		voz.buffer = bufs + k * maxbufsize;
		voz.inicializar();


		Voz& voz2 = Voces2da[k];
		voz2.buffer = bufs2da + k * maxbufsize;
		voz2.inicializar();

		Voz& voz3 = Voces3ra[k];
		voz3.buffer = bufs3ra + k * maxbufsize;
		voz3.inicializar();
		}

	//Seteo el factor de apagado de cada nota. Puede finetunearse mejor
	for (int i = 0; i < 127; i++)
	{
		notasPrendidas[i] = false; 
	}


	//GUI:
	extern AEffGUIEditor* createEditor(AudioEffectX*);
	setEditor(createEditor(this));




	return;
}

KPS::~KPS() {

	delete[] bufs;
	delete[] bufs2da;
	delete[] bufs3ra;
	delete[] Voces;
	delete[] Voces2da;
	delete[] Voces3ra;

	return;
}

/*						Funciones que el Host va a llamar							*/

//canDo() is called by the host when it wants to know what our capabilities are.
VstInt32 KPS::canDo(const char* text) {
	//1 = Can Do
	//0 = Dont Know
	//-1 = Can't do
	if (!strcmp(text, "receiveVstEvents")) return 1;
	if (!strcmp(text, "receiveVstMidiEvent")) return 1;
	return -1;

}

VstInt32 KPS::getNumMidiInputChannels() {
	return 1; //Hasta 16, cualquiera esta bien
}

VstInt32 KPS::getNumMidiOutputChannels() {
	return 0;
}


//The function is called whenever the sample rate changes, so if there are constants derived from it you can recalculate them here.
//It should also hopefully be called before processReplacing() is first called.
void KPS::setSampleRate(float samplerate) {
	AudioEffectX::setSampleRate(samplerate);
	sr = (int)samplerate;
	samplesdeRetardoCuerda2 = (int)(sr * 0.04);
	for (int k = 0; k < maxVoices; k++)
	{
		Voces[k].inicializar();
		Voces2da[k].inicializar();
		Voces3ra[k].inicializar();
	}
	return;
}

	//Todo: Ver si el sr es muy alto, no incrementar el tamaño de los buffers
	


/*					PARAMETROS Y PROGRAMAS								*/

//-----------------------------------------------------------------------------------------
//Esto en teoria implementa un switch, pero en la practica no hace absolutamente nada
bool KPS::getParameterProperties(VstInt32 index, VstParameterProperties* p)
{
	//Casi todos los parámetros son floats entre 0 y 1, asi que los dejo tranquis
	return false;
}

//-----------------------------------------------------------------------------------------
//Setear parametros desde el host
//El host, por defecto, siempre me pasa valores entre 0 y 1

extern CParamDisplay* pickDisplay;
extern CHorizontalSlider* hslider_pick;

void KPS::setParameter(VstInt32 index, float value)
{
	switch (index)
	{
	case kb:
		//b es una probabilidad
		//En 0 y 1, suena tónico. En 0, suena una octava más grave
		//Como al parámetro en la interfaz lo llamé "tonicidad, hago una funcion que a los valores cerca de 1 los mande a valores cerca de 0 y de 1
		// y que a valores entre 0 y 1 los mande a valores cerca de 0;

		//Suavizo un poco la transicion en el medio
		//TODO: Cuantizar steps?
		bQueMuestro = value;
		if (value < 0.5) b = pow(value, 7);
		if (value > 0.5) b = pow(value, 1. / 10.);
		break;

	case kS: {
		SQueMuestro = value;
		if (value == 0.) S = 100 * eps;
		else if (value == 1.) S = 1. - 100 * eps;
		else                  S = .5 + pow((value - 0.5), 3) * 4;
		break;
	}

	case kdampening:
	{  //Funciona inverso: slider abajo, rho 1. Slider arriba, rho 0.
		rhoQueMuestro = value;
		rho = 1 - pow(value, 10);
		break;
	}
	case kvibAmt:
		vibAmt = value; //Esto me multiplica al desv卲 que hago, asi que lo mantengo
		break;
	case kvibRate:
		vibRate = 20 * value; //Elijo tener vibratos de hasta 20Hz 
		break;
	case kpickPos:
		pickPos = value;
		break;
	case kType:
		Type = value;
		break;
	case kType2:
		setearType2(value);
		break;
	case kCharacter:
		character = value;
		break;
	case kmute:
		if (value == 1.0) mute = true;
		else /* if(value =0.) */ mute = false;
		break;
	case kReset:
		reiniciarVoces();
		break;
	}

	//Además de setear el valor, le digo que se comunique con la gui:
	if (editor)
		((AEffGUIEditor*)editor)->setParameter(index, value);
}

//-----------------------------------------------------------------------------------------
//Deci cuanto vale ahora el parametro pedido 
float KPS::getParameter(VstInt32 index)
{
	switch (index)
	{
	case kb:
		return bQueMuestro;
	case kS:
		return SQueMuestro; 
	case kdampening:
		return rhoQueMuestro;
	case kvibAmt:
		return vibAmt;
	case kvibRate:
		return vibRate;
	case kpickPos:
		return pickPos;
	case kType:
		return Type;
	case kCharacter:
		return character;
	}
	return 0.f;
}

//-----------------------------------------------------------------------------------------
void KPS::getParameterName(VstInt32 index, char* label)
{

	switch (index)
	{
	case kb:
		vst_strncpy(label, "Percusividad", kVstMaxParamStrLen);
		break;
	case kS:
		vst_strncpy(label, "decay", kVstMaxParamStrLen);
		break;
	case kdampening:
		vst_strncpy(label, "dampening", kVstMaxParamStrLen);
		break;
	case kvibAmt:
		vst_strncpy(label, "vibrato (%)", kVstMaxParamStrLen);
		break;
	case kvibRate:
		vst_strncpy(label, "Vibrato Rate (Hz)", kVstMaxParamStrLen);
		break;
	case kpickPos:
		vst_strncpy(label, "Pick Position (Metalicidad) ", kVstMaxParamStrLen);
		break;
	case kType: {
		vst_strncpy(label, "Tipo (Clavicordio/Charango)", kVstMaxParamStrLen);
		break;
	}
	case kCharacter:
		vst_strncpy(label, "Character", kVstMaxParamStrLen);
		break;
	}
	return;
}

//-----------------------------------------------------------------------------------------
//Mostrá en texto cuanto vale ahora el parametro pedido 
void KPS::getParameterDisplay(VstInt32 index, char* text)
{
	switch (index)
	{

	case kvibRate:
		sprintf(text, "%2.2f Hz", vibRate);
		break;
	case kvibAmt:
		sprintf(text, "%i%%", vibAmt* 100);
		break;


	case kpickPos:
		sprintf(text, "%i %% from Bridge", floor(pickPos*100));
		break;

	case kType: {
		if (Type <= 0.5)
			sprintf(text, "Type: Clavicordio");
		if (Type > 0.5)
			sprintf(text, "Type: Charango");
		break;
	}
	}
	return;
}


/*		Presets		*/

//------------------------------------------------------------------------
void KPS::setProgramName(char* name)
{
	strcpy(programs[curProgram].name,
		name);
}

//------------------------------------------------------------------------
void KPS::getProgramName(char* name)
{
	if (!strcmp(programs[curProgram].name, "Init"))
		sprintf(name, "%s %d", programs[curProgram].name, curProgram + 1);
	else
		strcpy(name, programs[curProgram].name);
}

//-----------------------------------------------------------------------------------------
bool KPS::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
{
	if (index < knumPresets)
	{
		strcpy(text, programs[index].name);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
void KPS::setProgram(VstInt32 program)
{
	KPSPreset* ap = &programs[program];

	curProgram = program;
	for (int i = 0; i < kParam; i++) setParameter(i, ap->params[i]);

	//setParameter(kDelay, ap->fDelay);
	//setParameter(kFeedBack, ap->fFeedBack);
	//setParameter(kOut, ap->fOut);
}



KPSPreset::KPSPreset()
{
	// default Program Values

	strcpy(name, "Init");
	float defaults[kParam];
	defaults[kb] = 1.;
	defaults[kS] = 0.5;
	defaults[kdampening] = 0.;
	defaults[kvibAmt] = 0.;
	defaults[kvibRate] = 0.;
	defaults[kpickPos] = 1.;
	defaults[kType] = 0.;
	defaults[kType2] = 0.;
	defaults[kCharacter] = 0.;
	defaults[kmute] = 0.;

	for (int i = 0; i < kParam; i++) params[i] = defaults[i];

	//Todo: Inicializar todas las vocessss en 0
}






//------------------------------------------------------------------------
void KPS::resume()
{
	//memset(buffer, 0, size * sizeof(float));
	//Indicar aca cambio de latencia si hay
	AudioEffectX::resume();
}

VstInt32 KPS::getOutputLatency() {
	//TODO: Calcular bien
	return 512;

}



/*		Funciones más genericas y de informacion/metadatos del plugin			*/

/*									Metadatos Sociales para el host         */

VstInt32 KPS::getVendorVersion()
{
	return 2;
}

bool KPS::getEffectName(char* name)
{
	vst_strncpy(name, "Karplus Strong Synth", kVstMaxEffectNameLen);
	return true;
}

bool KPS::getProductString(char* text)
{
	vst_strncpy(text, "KPS Synth - Roman Sama", kVstMaxProductStrLen);
	return true;
}

bool KPS::getVendorString(char* text)
{
	vst_strncpy(text, "Román Sama", kVstMaxVendorStrLen);
	return true;
}



void KPS::setearType2(float value) {

	Type2 = value;
		//Apago todas las voces de golpe. Podría apagarlas suavemente, pero meh
		for (int i = 0; i < maxVoices; i++)
		{
			Voces[i].on = false;
			Voces[i].p = 0;
			Voces[i].apagandose = false;
			Voces[i].freq = 0.;
			Voces[i].pickPosition = 0.;
			Voces[i].voiceNote = -1;
			Voces[i].ultima = 0;
		}
		for (int i = 0; i < 128; i++) notasPrendidas[i] = false;

		if (!LETRING) Voces[0].precalcSize = 2 * dawRequest;  //asumo que voy a calcularlo todo el tiempo y reciclo
		else Voces[0].precalcSize = 10 * dawRequest;


}

//TODO: hacer maxbufsize static y que no se pase como parametro a inicializarVoz

void KPS::reiniciarVoces() {

	for (int i = 0; i < maxVoices; i++)
	{
		Voces[i].inicializar();
		if(BICORDIO)  Voces2da[i].inicializar();
		if(TRICORDIO) Voces3ra[i].inicializar();
	}
	onVoices = 0;

	if (editor) ((AEffGUIEditor*)editor)->setParameter(kOnVoices, onVoices);

}