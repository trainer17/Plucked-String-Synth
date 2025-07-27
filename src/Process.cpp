#pragma once
#include "Sinewave.h"
#include "Auxiliares.h"
#include "Parametros.h"


#include <iostream>
#include <cstdlib> 
#include <assert.h>


#include <float.h>

//TODO: Que el tiempo de precalculo no esté hardocodeado y sea una variable, aunque const

//Como es mi estrategia (los valores son a modo de ejemplo):
//
//Me llega un Note on. Precalculo un segundo de sonido para esa voz
//El DAW me pide samples de a 512 para un sr de 48.000 
//Voy a entregarle 512 de los que tengo ya precalculados para la voz 1.
//Si cubro todo, listo
//Si veo que me faltan, precalculo más samples

//Tengo 8 voces como máximo
//Tengo un array grande ("bufs") donde van a ir los samples de todas las voces. Este array se dividide en 8 partes iguales, una para cada voz, de tamaño "maxbufsize" cada una
//cada voz tiene guardada la posicion inicial ("voz.buffer") que le corresponde dentro de ese gran buffer. La final es "voz.buffer + maxbufsize" (pensar "maxbufsize" como "maxVoiceBufSize").
//Cada voz tiene un indice de donde se encuentra en el buffer ("si")
//Si a la voz le llegó un noteon, le marco "on"
//Si le llegó un noteoff, le dejo el "on" y le marco el "apagándose.
//En este momento, se le aumenta mucho el factor de decay ("rho"), porque sino quedaría sonando indefinidamente; y se le regenera desde todo desde el indice 0, con dampening ("rho")
//Cuando termino de leer el buffer y veo que la nota está apagandose, ahí sí la marco con un "off"



//Si tengo que precalcular, voy a llenar el buffer de esa voz desde el indice 0 hasta donde pueda meter ciclos completos de la onda. El ultimo sample valido precalculado es "ultima"
//Para tener una referencia: las notas más graves piden buffers de 600 samples para arriba.
//Notas mas graves que la real de una guitarra son factibles (suenan re bien)
//Para sfx interesantes , buffers de hasta 4800 samples son factibles (y menos tambien!). Por eso hago que cada voz tenga un gran buffer, para soportar notas graves

//Se podría implementar una clase de "buffer circular" para el buffer de cada voz. Cmparar con el indice con la longitud del buffer en cada incremento es mejor que copiar el excedente cada tantos blocksizes. 
//Pero no voy a hacer eso ahora porque me mato


//Agrego en la verison 11: Timing real
//Las voces que se trigerearon en este bloque, tienen un "deltaframes" (blocki) mayor a 0. Se suman al bloque que me está pidiendo el daw a partir de ese sample
//Modificado en la version 15: A partir de ahora, simplemente se *generan* los samples a partir del offset de deltaframes, se deja en 0's lo anterior y se entrega desde 0 lo mismo. Esto es para implementar más fácil los modos monofónicos

//Me preocupa mucho el tema de la eficiencia para notas muy graves igual...

//Todo: Que pasa cuando solicito una nueva voz y tenia una que se estaba apagando. O si solicito mas voces que el maximo

/*						AUDIO   -  PPAL					*/
//This is the heart of our "synthesizer". La generacion de samples!!
//blocksize es como maximo lo especificado en las opciones. En cada llamada puede ser diferente!
void KPS::processReplacing(float** inputs, float** outputs, VstInt32 blockSize)
{

	dawRequest = blockSize;

	if (mute) return;

	float* out1;
	onVoices = 0;
	for (int voice = 0; voice < maxVoices; voice++)
	{
		Voz& voz = Voces[voice];
		if (!voz.on) continue;
		onVoices++;

		blockSize = dawRequest;
		out1 = outputs[0]; //mono a partir de la versión 10

		//1- Veo si la voz necesita calcular samples
		//Si me pide más de lo que ya tengo precalculado, entrego lo que tengo, calculo más, y los entrego
		if (voz.si + dawRequest >= voz.ultima) {

			//Entrego lo que tengo
			while (voz.si < voz.ultima) *(out1++) += voz.buffer[voz.si++];

			//Precalculo más
			if (!voz.apagandose) precalcularVoz(voz);
			else 				seguirApagando(voz);

			//Entrego el resto
			if (voz.on) while (out1 < outputs[0] + dawRequest) *(out1++) += voz.buffer[voz.si++];
			blockSize = 0; //Me aseguro que esta voz ya no entrega samples
		}

		//2-Entrego los samples de esta voz

		if (voz.blocki > 0) //tiene offset dentro de este bloque:
		{
			blockSize -= voz.blocki;
			out1 += voz.blocki;
			voz.blocki = 0; //saco el offset para los siguientes bloques
		}

		//Los sumo al out
		while (--blockSize >= 0)
			(*out1++) += voz.buffer[voz.si++];

		voz.xn_1 = voz.buffer[voz.si-1]; //Indico el último sample para el allpass filter

		//------------------------------------------------------------------------------------
		//----------------------------   2da  ------------------------------------------------
		//---------------------------------------------------------------------------------------
		
		//Repito lo mismo para la 2da cuerda
		if (BICORDIO)
		{
			Voz& voz2 = Voces2da[voice];
			out1 = outputs[0];
			blockSize = dawRequest;

			//1.2 - si necesita precalcular samples::
			if (voz2.si + dawRequest >= voz2.ultima) {

				//if (!voz2.on) continue; todo: ver si comentar esto trae problemas?
				while (voz2.si < voz2.ultima) *(out1++) += voz2.buffer[voz2.si++];
				if (!voz2.apagandose) precalcularVoz(voz2);
				else 				  seguirApagando(voz2);
				if (voz2.on) while (out1 < outputs[0] + dawRequest) *(out1++) += voz2.buffer[voz2.si++];
				blockSize = 0; 
			}

			// Si hay offset, lo atajo
			if (voz2.blocki > 0)
			{
				//Puede que el offset sea tan grande que no arranque a sonar en este bloque.
				//Por ej: blocksize de 500, voz1 arranca a sonar en el sample 400 de este bloque. voz2 deberia sonar en el sample 1200 de "este bloque (800 samples de offset relativos a la voz 1)

				//Pero también puede ser que sí arranque a sonar dentro de este bloque
				//Por ej: blocksize de 500, voz1 arranca a sonar en el sample 400 de este bloque. voz2 deberia sonar en el sample 450 de "este bloque (50 samples de offset relativos a la voz 1)

				blockSize -= voz2.blocki; // -700 o 50

				// Quedan samples (50), preparo el offset (lo mismo que hice arriba)
				if (blockSize > 0) {
					out1 += voz2.blocki;
					voz2.blocki = 0;
				}
				// Sino, no suena en este bloque. Preparo el siguiente offset
				else
					voz2.blocki = -blockSize;  //ES SUMA en este caso. porque blocksize es negativo
			}

			//Si no hay offset, sumo directo al out
			while (--blockSize >= 0)
				(*out1++) += voz2.buffer[voz2.si++];
		}


		//------------------------------------------------------------------------------------
		//----------------------------   3ra ------------------------------------------------
		//---------------------------------------------------------------------------------------


		//Repito lo mismo para la tercera cuerda

		if (TRICORDIO) {
			Voz& voz3 = Voces3ra[voice];
			out1 = outputs[0];
			blockSize = dawRequest;

			//1.3 - Veo si necesita precalcular samples
			if (voz3.si + dawRequest >= voz3.ultima) {
				while (voz3.si < voz3.ultima) *(out1++) += voz3.buffer[voz3.si++];
				if (!voz3.apagandose) precalcularVoz(voz3);
				else 				  seguirApagando(voz3);
				if (voz3.on) while (out1 < outputs[0] + dawRequest) *(out1++) += voz3.buffer[voz3.si++];
				blockSize = 0;
			}

			//2.3: sino, entrego directo
			out1 = outputs[0];

			if (voz3.blocki > 0)
			{

				blockSize -= voz3.blocki;

				if (blockSize > 0) {
					out1 += voz3.blocki;
					voz3.blocki = 0;
				}

				else
					voz3.blocki = -blockSize;
			}
			while (--blockSize >= 0)
				(*out1++) += voz3.buffer[voz3.si++];
		}

	} // fin del for de voz

	if (editor) ((AEffGUIEditor*)editor)->setParameter(kOnVoices, onVoices); //informo que hay una voz menos sonando
}


/*                   MIDI -  PPAL                 */

// processEvents 'll accept MIDI data and decide what to do with it.
//el codigo es bastante claro con lo que se recibe y como se estructuran los eventos desde el DAW
//Esto se llama en cada sample
VstInt32 KPS::processEvents(VstEvents * ev) {
	int notasRecibidas = 0;
	for (VstInt32 i = ev->numEvents-1; i >= 0; --i) { //Invierto el orden para dar prioridad a los eventos nuevos y descartar los más viejos en caso de que lleguen muchos de golpe
		if ((ev->events[i])->type != kVstMidiType) continue;

		//A partir de aca, ev->events[i] es de tipo midi. 
		//en el atributo midiData se guarda el mensaje 
		VstMidiEvent * event = (VstMidiEvent*)ev->events[i];
		char* midiData = event->midiData;

		VstInt32 status = midiData[0] & 0xf0; //Trato igual a todos los canales

		switch (status) {
			//Solo atiendo Note On y Note Off aca
			//TODO: Si me llegan más de maxVoices notas distintas, me quedo solo con las ultimas (por ejemplo, si alguien se apoya en el piano con el brazo)
		case 0x90: //Note on  1001
		case 0x80: //Note off 1000
		{
			VstInt32 note = midiData[1] & 0x7F; //primeros cuatro bits: 0111 1111
			VstInt32 velocity = midiData[2] & 0x7F;
			VstInt32 channel = midiData[0] & 0x0F; // 0000 1111
			VstInt32 deltaFrames = event->deltaFrames; //sample del bloque actual en el que llegó el noteon/noteoff
			if (status == 0x80 || velocity == 0)
			{
				//Note off
				if (LETRING) apagarNota(note, channel, deltaFrames);
				else if (LEGATO)  ligarApagar(note, velocity, channel, deltaFrames);
				else if (SLIDE)   slideApagar(note, velocity, channel, deltaFrames);
			}
			else
			{	//Note On
				if (++notasRecibidas > maxVoices) continue; //Ignoro si recibo DEMASIADAS notas nuevas en este bloque
				
				if (LETRING) triggerVozLibre(note, velocity, channel, deltaFrames);
				else if (LEGATO) ligarPrender(note, velocity, channel, deltaFrames); //TODO: SI ME LLEGAN DOS NOTAS EN EL MISMO BLOQUE, EXPLOTA MI PROGRAMA EN ESTE MODO. RE PENSAR. Guardar solo 3 notas por ej: From, To.
				else if (SLIDE)  slidePrender(note, velocity, channel, deltaFrames);
			}
			break;
			
		}
		case 0xE0: // Pitch bend
		{
			VstInt32 channel = midiData[0] & 0x0F;
			VstInt32 bendAmt = (midiData[1] << 7) + midiData[2]  - 8192;
			break; 
		}


		case 0xB0: // Controller
			break;
			// etc

		}
	}
	return 1;
}

/*					AUXILIARES	PRINCIPALES      */

//TODO: optimizar acá y no acceder a memoria dos veces
void Voz::promediarVecinos(float*& anterior, float*& actual, float* maximo, float* comparador)
{

	while (comparador++ < maximo)
	{
		//Filtro pasabajos de cada ciclo
		*(actual++) = 0.5 * (*anterior + *(anterior - 1));
		anterior++;
	}
}



//-------------------------------------------------------------------------------

//Busca una voz libre y la hace sonar
void KPS::triggerVozLibre(VstInt32 note, VstInt32 velocity, VstInt32 channel, VstInt32 deltaframes) {

	int voice = 0;

	//Activo la primer voz que esté disponible. Si reiteró la misma nota mientras seguia sonando, la piso
	while (voice < maxVoices && (Voces[voice].on && Voces[voice].voiceNote != note)) voice++;

	//Si no hay ninguna voz disponible y tengo que pisar una de las que ya está sonando, elijo la que tenga menos energía de las que se están apagando
	if (voice >= maxVoices) {
		voice = 0;
		float Emin = 500.;
		int voiceMin = -1;
		int notaMin = 0;
		for (voice = 0; voice < maxVoices; voice++) 
			if (Voces[voice].apagandose && Voces[voice].energia < Emin) 
				{ Emin = Voces[voice].energia; voiceMin = voice; }
			
		

		//Y si ninguna se está apagando e igual tengo que pisar alguna, suelto la más aguda
		if (voiceMin >= 0) voice = voiceMin;
		else for (voice = 0; voice < maxVoices; voice++)
			if (Voces[voice].voiceNote > notaMin) 
			{voiceMin = voice; notaMin = Voces[voice].voiceNote;}

	}

	//TODO: Reemplazar una del mismo canal


	Voz & voz = Voces[voice];

	voz.cargarVoz(note, velocity, channel, deltaframes);
	trigger(voz);



//	if (deltaframes > dawRequest)
	//	printf("ERROR");

	if (BICORDIO) {
		Voz& voz2 = Voces2da[voice];
		voz2 = Voces2da[voice];
		voz2.cargarVoz(note, velocity - 7, channel, deltaframes + samplesdeRetardoCuerda2);
		if(character > 0.2) voz2.character -= 0.2;
		//voz2.p -= 1;
		voz2.freq *= 1.00405144;  // +7 cents

		if (voz2.velocity <= 0) voz2.velocity = 3;

		if (pickPos + 0.1 <= 1) voz2.pickPosition += 0.1;
		else voz2.pickPosition -= 0.1;

		trigger(voz2);
	}

	if (TRICORDIO) {
		Voz& voz3 = Voces3ra[voice];
		voz3.cargarVoz(note, velocity - 17, channel, deltaframes + 1.5*samplesdeRetardoCuerda2); //Agrego un poquito de delay temporal y que suene un poquito más débil
		voz3.character += 0.2; //Suena distinto al resto . Todo, ver si no hay problema que sea mayor a 1?
		voz3.freq *= 0.9959648;  //Slight detune, -7 cents
		if (voz3.velocity <= 0) voz3.velocity = 3;
		if (pickPos + 0.2 <= 1) voz3.pickPosition += 0.2; //Cambio la posicion de la púa
		else voz3.pickPosition -= 0.2;
		trigger(voz3);
	}

}


//Activa una voz: Genera un buffer de ruido y precalcula 0.5s

void KPS::trigger(Voz& voz) {


	float* buf = voz.buffer;

	int p = voz.p;

	//1-Calculo valor C del all-pass filter para finetunear y compensar el que el delay sea entero
	voz.calcularC(sr);



	//3-CICLO DE RUIDO INICIAL


	//3.1 - CHARACTER: 
	// Define como llenar el primer ciclo de la onda
	//0 -> Genera un primer ciclo de puros 0's
	//1 -> Genera un primer ciclo lleno de ruido
	//0.5 -> Genera un primer ciclo cuya primer mitad es ruido y el resto 0's
	//0.75 -> Genera un primer ciclo cuyos primeros 3/4 son ruido y el resto 0's, etc

	
	p = voz.character * p;
	if (p < 5) p += 2; // que no sea completely 0's
	for (int i = p; i < voz.p; i++) buf[i] = 0.;
	
	//3.2 BUFFER DE RUIDO (CICLO INICIAL)
	float promedio = 0;
	for (int i = 0; i < p; i++)
		promedio += buf[i] = randomSample();
	promedio /= (float)p;
	for (int i = 0; i < p; i++) buf[i] -= promedio; //quitamos DC offset


	//3.3 - FILTRO DE DINAMICAS
	float G = (float)voz.velocity / 127.0;
	float R = calcularR(G, PI * voz.freq * sr);
	float unomenosR = 1 - R;

	buf[0] = unomenosR * buf[0];
	for (int i = 1; i < p; i++)
		buf[i] = unomenosR * buf[i] + R * buf[i - 1];

	//Reestablezco lo que cambié por el character y retardo
	p = voz.p;
	buf = voz.buffer;

	//3.4 - PICK POSITION: Comb filter, con la frecuencia dependiendo de la position relativa de la púa
	// 0: 100% puente
	// 1: 100% clavijero
	//Es una resta con un delay de pickPos*p samples
	//Aca tomo una decision: Si la diferencia es con 80 samples, por ej, 
	//   buf[80] <- x[80] - x[0]
	//   buf[81] <- x[81] - x[1]
    //   buf[82] <- x[82] - x[2]
	//   buf[ultimo] <-- x[ultimo] - x[ultimo-80]

	int pickDelay = round(voz.pickPosition * p); //v11: Cambio a round (antes era "floor")
	if (pickDelay == 0) pickDelay = 1;
	for (int i = p-1; i >=pickDelay; i--) //Lo hago al reves para no pisar lo que despues voy a necesitar
		buf[i] =  buf[i] - buf[i - pickDelay];



	//STRING EQUATION
	float* anterior = buf;
	float* actual = buf + p;
	//Tomo que buf[-1] = 0 y por ende buf[p] = buf[0]
	*(actual++) = *(anterior++);
	voz.xn_1 = 0.; //Para all-pass filter

	voz.promediar(anterior, actual, buf + voz.precalcSize+1, actual, buf, buf + voz.precalcSize);

	voz.ultima = voz.precalcSize; //Indico hasta donde son válidos los samples
	voz.si = 0;				     //Indico desde donde empiezan los samples válidos
	//+1 al precalc por seguridad

	return;
}


//Toma una voz ya activa que entregó todos sus samples y le genero la siguiente tanda
void KPS::precalcularVoz(Voz& voz) {

	float* buf = voz.buffer;
	int p = voz.p;

	//Llegado acá, siempre se toma que voz.ultima = voz.si (por haber entregado todo),
	// o por estar en modo Legato

	/*        CASO 1 - ME QUEDA ESPACIO EN EL BUFFER. Sigo llenando  */

	if (voz.si + voz.precalcSize < maxbufsize) {

		float* anterior = buf + voz.si - p;
		float* actual = buf + voz.si;
		voz.promediar(anterior, actual, actual + voz.precalcSize, actual, actual, actual+voz.precalcSize);

		voz.ultima += voz.precalcSize;
		//No actualizo s[i], que siga leyendo
		return;
	}

	//	/*        CASO 2 - NO ME QUEDA ESPACIO EN EL BUFFFER. Vuelvo al inicio */

	//Pensar que el buffer llegó hasta buf[voz.si-1], y que ahora, en vez de seguir en buf[voz.si], sigue en buf[0]

	//Precalculo un ciclo en base al anterior
	float* anterior = buf + voz.si - p;
	float* actual = buf;
	voz.promediar(anterior, actual, anterior+p , anterior);

	//Precalculo el resto de los ciclos
	//El primer sample lo precalculo a mano: (ignoro S, b, rho)
	*(actual++) =  0.5 * (*buf + *(anterior - 1));
	anterior = buf + 1;
	voz.promediar(anterior, actual, buf + voz.precalcSize+1, actual, buf, buf + voz.precalcSize);

	voz.si = 0;
	voz.ultima = voz.precalcSize;

	//TODO: Revisar porque me parece que se está detuneando de a un sample cada vez
	return;
}


void KPS::apagarNota(VstInt32 nota, VstInt32 channel, VstInt32 deltaframes) {

	//1- Identifico que voz se apagó
	int kvoz = 0;
	while(kvoz<maxVoices && ( Voces[kvoz].voiceNote != nota || Voces[kvoz].channel != channel || !Voces[kvoz].on || Voces[kvoz].apagandose))
		kvoz++;	
	
	if (kvoz < maxVoices) {

		apagarVoz(Voces[kvoz]);
		if (BICORDIO) apagarVoz(Voces2da[kvoz]);
		if (TRICORDIO) apagarVoz(Voces3ra[kvoz]);
	}

}



//Se llama al recibir un note off
//Deja que el sonido muera de forma más natural que cortandolo de forma abrupta
#include <assert.h>     /* assert */
void KPS::apagarVoz(Voz& voz) {

	voz.apagandose = true;
	
	//1- Copio un ciclo con dampenings al inicio

	float* actual = voz.buffer; // (empiezo a escribir desde la posicion 0)
	float* anterior = voz.buffer + voz.si; //hasta donde llegué antes

	//Me fijo si me queda un ciclo entero sin entregar para empezar a tomar desde ahí
	//Si no, me tomo el ciclo anterior
	if (voz.si + voz.p >= voz.ultima) anterior -= voz.p;
	//Esto que sigue no deberia pasar, pero 
	if (anterior <= actual)
		anterior = actual + 1;

	//2- Saco un ciclo
	voz.promediarApagar(anterior, actual, voz.buffer + voz.p);

	//3-Saco el resto de los ciclos
	//3.1 EL primer caso va a mano
	*(actual++) = 0.5 * (*voz.buffer + *(anterior - 1));
	anterior = voz.buffer + 1;
	//3.2 el resto
	voz.promediarApagar(anterior, actual, voz.buffer + voz.precalcSize+1, actual, voz.buffer, voz.buffer+voz.precalcSize);

	//Indico en donde estaba mi si
	voz.ultima = voz.precalcSize;
	voz.si = 0;

}


//Acá tengo en cuenta la energía de un ciclo para decidir si sigo apagando o no
void KPS::seguirApagando(Voz& voz) {

	float* buf = voz.buffer;

	//1- Saco la energía del último ciclo

	float E = 0.;
	for (int i = voz.ultima - voz.p; i < voz.ultima; i++)
		E += pow(buf[i], 2);

	//2- Si la energia está por debajo de este umbral, le digo que deje de recalcular
	if (E / voz.p <= ENERGIA_MINIMA)
	{
		voz.on = false;
		voz.voiceNote = 0;
		voz.apagandose = false;
		voz.energia = 0.;
		return;
	}

	//Sino, le digo que siga sonando:

	//COPIO LITERAL EL CODIGO DE "SEGUIR SONANDO"

	//3-Precalculo un ciclo más


	/*        CASO 1 - ME QUEDA ESPACIO EN EL BUFFER. Sigo llenando  */

	if (voz.si + voz.precalcSize < maxbufsize) {

		float* anterior = buf + voz.si - voz.p;
		float* actual = buf + voz.si;
		voz.promediarApagar(anterior, actual, actual + voz.precalcSize,actual, buf, buf+voz.precalcSize);

		voz.ultima += voz.precalcSize;
	}
	else
	{
		/*        CASO 2 - NO ME QUEDA ESPACIO EN EL BUFFFER. Vuelvo al inicio */

		//Pensar que el buffer llegó hasta buf[voz.si-1], y que ahora, en vez de seguir en buf[voz.si], sigue en buf[0]

		//Precalculo un ciclo en base al anterior
		float* anterior = buf + voz.si - voz.p;
		float* actual = buf;
		//TODO: Estoy usando la version PROMEDIAR NORMAL PARA UN SOLO CICLO. Esto es porque NO QUIERO REDEFINIR "promediarApagar" para incluir el parámetro "comparador" a esta hora de la noche, ni sé si por un ciclo se justifica (en notas graves graves sí..)
		voz.promediar(anterior, actual, anterior + voz.p, anterior);

		//Precalculo el resto de los ciclos
		//El primer sample lo precalculo a mano: (ignoro S, b, rho)
		*(actual++) = 0.5 * (*buf + *(anterior - 1));
		anterior = buf + 1;
		voz.promediarApagar(anterior, actual, buf + voz.precalcSize,actual, buf, buf + voz.precalcSize);

		voz.si = 0;
		voz.ultima = voz.precalcSize;
	}


	
}



/*             MODOS MONO                          */	

//LEGATO  Y GLIDE

//Como son: Solo suena la voz 1
//Cuando está apagada, se trigerea.
// Si llega un note on mientras está  sonando,  pisa / slidea hasta la siguiente nota.
//Si se suelta la voz actual pero ya había una presionada de antes, se va a la más cercana (por ej, en un trino)


//Elijo que cuando está en uno de los modos "mono", la voz que suena es la primera (voz[0])

void KPS::ligarPrender(VstInt32 note, VstInt32 velocity, VstInt32 channel, VstInt32 deltaframes) {

	Voz& voz = Voces[0];
	notasPrendidas[note] = true;
	bool on = voz.on;
	voz.cargarVoz( note, velocity, channel, deltaframes);
	

	//Si está apagada, la trigereo
	if (!on || voz.apagandose) trigger(voz);

	//Si está sonando, solo cambio de nota (piso otro traste)
	else precalcularVoz(voz);

}


void KPS::ligarApagar(VstInt32 note, VstInt32 velocity, VstInt32 channel, VstInt32 deltaframes) {

	Voz& voz = Voces[0];
	notasPrendidas[note] = false;

	//1- Comparo la que se apagó con la nota que está sonando actualmente (la que se apagó podría ser otra)
	// Si es otra, no hago nada
	if (voz.voiceNote != note) return;


	//2-Si se apagó la que estaba sonando ahora, busco si puedo saltar a otra que siga presionada
	int notaSup = voz.voiceNote;
	int notaInf = voz.voiceNote;
	int nota;
	int i = 1;
	while(i<126)
	{ 
		if (notaInf >= 0 && notasPrendidas[notaInf]) 
		{
			nota = notaInf; 
			break;
		}
		if (notaSup <= 127 && notasPrendidas[notaSup])
		{
			nota = notaSup;
			break;
		}
		notaSup++;  notaInf--; i++;
	}
	
	if (i < 126) // si hay otra nota prendida , salta a esa
	{
		voz.cargarVoz(nota, velocity, channel, deltaframes);
		precalcularVoz(voz);
		return;
	}

	//3-Si están todas apagadas, simplemente apago esta voz
	apagarVoz(voz);
}



void KPS::slideApagar(VstInt32 note, VstInt32 velocity, VstInt32 channel, VstInt32 deltaframes) {
	
	Voces[0].cargarVoz(note, velocity, channel, deltaframes);
	precalcularVoz(Voces[0]);

}


void KPS::slidePrender(VstInt32 note, VstInt32 velocity, VstInt32 channel, VstInt32 deltaframes) {

	Voces[0].cargarVoz(note, velocity, channel, deltaframes);
	precalcularVoz(Voces[0]);

}
