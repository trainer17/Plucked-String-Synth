#ifndef PARAM
#define PARAM

#define PI 3.1415926535897
#define TWOPI 2*PI
#define eps 0.0001

//TODO: TRES CUERDAS!
#define MONOCORDIO Type == 0. 
#define BICORDIO Type > 0.    // == 0.5
#define TRICORDIO Type > 0.5  // == 1.

//La idea detrás de definir así es que pueda hacer if(BICORDIO) activar segunda cuerda;   if(TRICORDIO) activar 3ra cuerda, sin preocuparme de mas ifs 
//Vuelo el instrumento "Noiseburst" de la version 2.1 a la luna porque no me acuerdo que corno frances hacía

#define LETRING Type2 ==0.
#define LEGATO  Type2 ==0.5
#define SLIDE   Type2 ==1.


#define PARENTLETRING padre->Type2 ==0.

#define ENERGIA_MINIMA 0.00001
#define INTERVALO_MINIMO 0.01 //Todo buscar una cantidad de cents razonable


//ID DE PARAMETROS PARA CONTROLAR DESDE EL HOST
enum {

	knumPresets = 16,

	kb = 0,      //vslider
	kS,			 //vslider
	kdampening,  //vslider
	kvibAmt,     //knob
	kvibRate,    //knob
	kpickPos,    //hslider
	kType,      //switch todo: TriSwitch
	kType2,		//triswitch todo: vswitch
	kCharacter, //knob
	kmute,		//mute
	kParam,		//Cantidad de parámetros
	kOnVoices,	//No es un parámetro, pero le pongo un display
	kReset
};


#endif