#pragma once
//Fuente: TOdo de de aca https://mitxela.com/projects/vsti_tutorial
//https://github.com/pongasoft/vst24-hello-world/blob/master/A_Beginners_Guide_To_VST2.4.md agrego este en 2022 por si sirve

//Acá un ejemplo con muchisimo que se puede pedir de data y eventos: https://gist.github.com/zombaio/f573c6e43c696c27f469b2a0f32764b6 


#include "public.sdk/source/vst2.x/audioeffectx.h"
#include <math.h>
#include "aeffguieditor.h" //Para el display de onVoices
#include "Parametros.h"    //Para kParam

class KPS;

struct Voz {
	int p = 0;
	int si = 0;
	bool on = false;
	bool apagandose = false;
	int voiceNote = 0;		//Nota Midi
	float* buffer = 0;		//Inicio del buffer de esta voz
	int ultima = 0;			//Ultimo sample valido dentro del array. Relativo al offset (o sea, si´"última" vale 500, solo son validos los samples 0-499 inclusive). Se compara con <, no con <=
	int blocki = 0;			//sample que le corresponde dentro del bloque actual (buffer del host). Por lo general va sincronizado, de 0 al blocksize, pero cuando arranca, por lo general es a mitad del bloque
	float freq = 0.;		//En Hz
	VstInt32 channel = 0;
	VstInt32 velocity = 0;

	float character = 0.5;
	float b = 1.;			//volar todos estos, son del padre ahora
	float S = 0.5;
	float pickPosition = 1.;
	float rho = 1.;
	
	float C =0.; //Filtro allpass
	float xn_1 = 0.; //Filtro allpass
	int precalcSize = 0;		//Cuantos samples precalculo para esta voz en cada llamada. Por defecto, 5 bloques de audio. Hacerlo miembro de KPS, no de voz
	float energia = 0.;         //para ver cuando ya está bien apagar la voz

	void inicializar();
	void cargarVoz(VstInt32 note, VstInt32 vel, VstInt32 chann, VstInt32 deltaframes);


	void promediarVecinos(float*& anterior, float*& actual, float* maximo, float* comparador = NULL); //la PRINCIPAL

	void promediar(float*& anterior, float*& actual, float* hasta, float* comparador = NULL, float* fineTunearDesde = NULL, float* fineTunearHasta = NULL);
	void promediarVecinosConSrho(float*& anterior, float*& actual, float* maximoActual, float* comparador); //S = decay digamos
	void promediarVecinosConB(float*& anterior, float*& actual, float* maximoActual, float* comparador);
	void promediarVecinosConBSrho(float*& anterior, float*& actual, float* maximoActual, float* comparador);
	void promediarApagar(float*& anterior, float*& actual, float* hasta, float* comparador = NULL, float* fineTunearDesde = NULL, float* fineTunearHasta = NULL);
	
	void fineTunear(float* actual, float* hasta); //Solo afina la fundamental! el resto de los armónicos no

	void calcularC(float sr);


	static KPS* padre;
};

//TODO: Volar el "sampi" a otro puntero, que se vaya incrementando de a uno. 



class KPSPreset
{
	friend class KPS;
public:
	KPSPreset();
	~KPSPreset() {}

private:
	/*
	float S;
	float dampen;
	float b;			
	float vibAmt;
	float vibRate;
	float character;
	float pickPos;
	float Type;
	float Type2;
	*/

	float params[kParam];
	char name[24];
};



//VstInt32 = int
class KPS : public AudioEffectX //Extiende a la clase de VST2
{
	friend class Voz;
public:
	KPS(audioMasterCallback audioMaster);
	~KPS();

	//PROCESAMIENTO PPAL - MIDI Y AUDIO (Process.cpp)
	virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);
	virtual VstInt32 processEvents(VstEvents* events);


	//INTERACCION CON EL HOST (.cpp)
	virtual VstInt32 canDo(const char* text);
	virtual VstInt32 getNumMidiInputChannels();
	virtual VstInt32 getNumMidiOutputChannels();
	virtual void setSampleRate(float sampleRate);
	virtual float getParameter(VstInt32 index);
	virtual bool getParameterProperties(VstInt32 index, VstParameterProperties* p);
	virtual void  getParameterName(VstInt32 index, char* label);
	virtual void getParameterDisplay(VstInt32 index, char* text);
	virtual void setParameter(VstInt32 index, float value);


	virtual VstInt32 getOutputLatency();

	//PRESETS
	virtual void setProgram(VstInt32 program);
	virtual void setProgramName(char* name);
	virtual void getProgramName(char* name);
	virtual bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text);


	virtual VstPlugCategory getPlugCategory() { return kPlugCategSynth; }


	virtual void resume();



	//SOCIALES  (.cpp)
	VstInt32 getVendorVersion();
	bool getEffectName(char* name);
	bool getProductString(char* text);
	bool getVendorString(char* text);



	/* PARAMETROS */
	float b = 1;     //1 - probabilidad de cambiar de signo el ciclo actual. Ni lo uso porque es un embole.
	float vibAmt = 0.;
	float vibRate = 0.;
	float S = 0.5;		 //Stretch factor. Tiene un minimo en 1/2 (en lo que se reduce al caso comun)
	float rho = 1.;		 //Dampening factor general
	float Type = 0.	;	 //Cantidad de cuerdas. Monocordio o Bicordio, según mayor o menor a 0.5
	float Type2 = 0.;	 //Let Ring, Vibrato o Legato
	float character=0.5; //Controla la cantidad de ruido en el ciclo inicial. //0.5 = sin cambios
	float pickPos = 1.;	 //parametro externo
	bool mute = false;	//De prueba para ver si funciona el boton. Totalmente inutil aparte de eso
	

protected:

	//EXTERNAS
	int sr;
	VstInt32 dawRequest;     //VARIABLE EN CADA LLAMADA!


	//CORAZON DE LA IMPLEMENTACION
	const int maxVoices = 8;
	int maxbufsize = 2048 * 32;	    //cantidad máxima de samples del buffer de una voz. Arbitrario. Elijo un multiplo de 2048
	float* bufs;					// El corazon. Acá están los samples que se escuchan.
	float* bufs2da;					// 2da cuerda
	float* bufs3ra;					// 3ra cuerda
	int samplesdeRetardoCuerda2 = 0;  //delay al tocar la segunda cuerda


	int onVoices = 0;				//Cuantas voces estan sonando ahora mismo

	//LIGADAS A PARAMETROS
	float rhoQueMuestro = 1;		     //el que le muestro al host
	float bQueMuestro = 1.;
	float SQueMuestro = 0.5;

	Voz* Voces;
	Voz* Voces2da; //todo: Para resonancias y segunda cuerda
	Voz* Voces3ra; //Para tercera cuerda



	//PROCESAMIENTO PPAL - Auxiliares de Audio
	void trigger(Voz& whichVoice);
	void precalcularVoz(Voz& voicei);
	void apagarNota(int notaMidi, VstInt32 channel, VstInt32 deltaframes);
	void seguirApagando(Voz& voz);
	void apagarVoz(Voz& voz);
	void reiniciarVoces();

	KPSPreset* programs; //Steinberg llama "programs" a los presets
	//int curProgram = 0; Te juro que no s・de donde sale esta variable pero bueno

	void resonar(int);
	void triggerVozLibre(VstInt32 note, VstInt32 velocity, VstInt32 chann, VstInt32 deltaframes = 0); //letRing

	// Tipos Monofónicos
	void ligarPrender(VstInt32 note, VstInt32 velocity, VstInt32 chann, VstInt32 deltaframes); //legato
	void slidePrender(VstInt32 note, VstInt32 velocity, VstInt32 chann, VstInt32 deltaframes); //slide
	void ligarApagar(VstInt32 note, VstInt32 velocity, VstInt32 chann, VstInt32 deltaframes); //legato
	void slideApagar(VstInt32 note, VstInt32 velocity, VstInt32 chann, VstInt32 deltaframes); //slide
	void setearType2(float value);

	VstInt32 notasPrendidas[128]; //Me dice que notas están aún presionadas

};



/*PROBLEMA GRANDE GRANDE  - NO PUEDO HACER GLISSANDO CON LA IMPLEMENTACION ACTUAL! */
//Ahora mismo estoy precalculando como dos segundos de sonido por vez (en cada noteon y cuando me quedo sin samples)
// Est・bien, y anda hermoso!!, pero por eso voy a dejar esta version atr疽 por la siguiente, solo para hacer un poco mas facil el glissando y el dampening, y aprovechar las cuetnas que hago (porque si precalculo dos segundos de sonido y despues los descarto cuando hay un note off... no tiene sentido)


/*			TODO	*/
/*
* -Al soltar la nota con tonicidad de botella, salta una 8va (creo que corregido)
* -Al soltar la nota con "b", hace como un "pulloff"
* -Modo Mono +Pullof; Mono+GLiss, Poly (actual)
  - Pitch Bend y vibrato. Tengo que crear un nuevo caso para la funcion "promediar"
  -Las notas graves se apagan muy pronto
  -Displays de los parametros
  - Dampening, R y S individual por voz (mejor no!) . Pero ajustar el rango de los dos
  - ajustar la funcion de G
  - Presets
  -Sensibilidad al velocity!! Ahora mismo se siente que no hay
  -Testear notas muy graves y aumentarles el decay time 
  -Probar en un DAW!!
  -Tunear bien el rho en funcion de las notas, o hasta dar una opci para desactivarlo (total ya se apagan solas cuando solt疽 las notas=
 -Keytracking para S y rho . S probabilistico en vez de multiplicativo
 -Optimizar en assembly los promedios. Se haria en un pedo asi
 -QUe character >0.5 funcione
 -ALgunas notas suenan MUCHO mas fuertes que otras actualmente (por ejemplo, G vs C grave). 
 Ademas, de por si, las agudas suenan menos que las graves
 //Todo: Dejar sonando una voz lo que necesite usando el calculo del tiempo de decay en segundos para la fundamental
 Todo: QUe las voces que llegan a un nuevo canal solo reemplacen a las del mismo canal
 Las cuerdas "la" suenan muuucho mas suave... que onda?
 Todo: Hacer que el character entre 0 y 0.5 sea el normal, y de 0.5 a 1 sea agregar copias a la wavetable original
Todo: Panic Button, silenciar todas las voces
Todo: Que los sliders actualicen las voces on the fly?

 */


//Podria usar un vector para cada voz. Estaria re bien hacer eso incluso
//Pero hago otra cosa
//El metodo que uso es sencillo pero no super evidente. 
 //Tengo 6 voces maximo. Entonces tengo 6 buffers: bufs[0], bufs[1]... bufs[5]
 //El tema es que quiero guardar esto de forma sencilla como una sola matriz (toda una porcion de memoria contigua, pero tampoco quiero romperme la cabeza indexando.

 //Cada buffer son los "ultimos samples" de cada voz
 //Por como esta declarado, bufs[0] [1] [2] [3]... son todos samples de la primer voz.
 //Por eso accedo con bufs[si[vi] + i] para el sample i de la voz vi
 // si[0] = 0
//  si[1] = maxbufsize
//si[2]   = 2*maxbufsize... etc
//El sample 3 de la voz 4 es bufs[4*maxbufsize+3]. Cuando si[4] se reinicia,  va a 4*maxbufsize, no a 0

//Quizas lo mas importante para la usabilidad... hacer mas uniformes las notas graves con las agudas! La intensidad, duracion, ataque es muy distinto :/

//En esta version, aumento la cantidad de voces a 10

//TODO: Pensar si las voces extra en realidad no conviene implementarlas en una única voz directamente. Lo que habría que hacer es esto: Al ciclo de ruido inicial, agregarle otro burst desfasado unos samples y filtrado de otra manera. Mucho más barato que calcular 3 voces para una misma nota
//TODO: Finetune solo si la diferencia de frecuencia es mayor a xx cents/intervalo