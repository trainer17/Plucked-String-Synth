
#include "Sinewave.h"
#include "Auxiliares.h"

//Abreviacion de los chequeos que tengo que hacer cada vez de b y S
//Aplica la ecuacion  *actual = 1/2 (*anterior + *(anterior-1) ) ; incrementando el "comparador" de a 1 hasta que sea == al maximo (se compara con <)
//No evalua cuando comparador == hasta, pero si al salir, termina con el comparador == hasta. OJO con eso
void Voz::promediar(float*& anterior, float*& actual, float* hasta, float* comparador, float* finetunearDesde, float* fineTunearHasta) {
	if (comparador == NULL) comparador = actual;

	if (S != 0.5 || rho != 1.) {
		if (b < 1.)  promediarVecinosConBSrho(anterior, actual, hasta, comparador);
		else  promediarVecinosConSrho(anterior, actual, hasta, comparador);
	}
	else if (b < 1)  promediarVecinosConB(anterior, actual, hasta, comparador);

	//La por defecto
	else  promediarVecinos(anterior, actual, hasta, comparador); //La mas basica y usual
	
	//FineTune all-pass
	if (finetunearDesde) fineTunear(finetunearDesde, fineTunearHasta);
	return;
}



//Todo: Volver a hacer que todo esto sea funcion de clase. No hay razon de que sea de cada voz mas que para finetunear

//-------------------------------------------------------------------------------
//Vuelo a la luna el caso character >0.5 en esta version, porque ya no recuerdo bien cual era la motivacion ni si tiene sentido implementarlo
//TODO: Hacer que cada voz tenga su S, rho, y que esta funcion sea miembro de cada voz

//Mismo promedio pero ahora uso S y rho
void Voz::promediarVecinosConSrho(float*& anterior, float*& actual, float* maximo, float* comparador)
{
	//if Stretch
	if (S != 0.5)
		//if no dampen
		if (rho != 1)
			while (comparador++ < maximo)
			{
				*(actual++) = (1 - S) * (*anterior) + S * (*(anterior - 1));
				anterior++;
			}
		else
			while (comparador++ < maximo)
			{
				*(actual++) = rho * ((1 - S) * (*anterior) + S * (*(anterior - 1)));
				anterior++;
			}

	//if solo dampen
		else {
			while (comparador++ < maximo)
			{
				*(actual++) = 0.5 * rho * ((*anterior) + *(anterior - 1));
				anterior++;
			}
		}

}

//Mismo promedio pero ahora uso b
void Voz::promediarVecinosConB(float*& anterior, float*& actual, float* maximo, float* comparador)
{
	while (comparador++ < maximo)
	{
		*actual = 0.5 * (*anterior + *(anterior - 1));
		if (randomSamplePositivo() > b) * actual = -(*actual);
		anterior++; actual++;
	}

}



//Mismo promedio pero ahora uso b y S
void Voz::promediarVecinosConBSrho(float*& anterior, float*& actual, float* maximo, float* comparador)
{

	//if Stretch
	if (S != 0.5)
	{
		float Snota = S * exp2f((1. - (float) voiceNote / 127.));
		//If only stretch
		if (rho == 1.)
			while (comparador++ < maximo)
			{
				*(actual++) = (1 - Snota) * (*anterior) + Snota * (*(anterior - 1));
				if (randomSamplePositivo() > b) * actual = -(*actual);
				anterior++;
			}
	//stretch y dampen
		else
			while (comparador++ < maximo)
			{
				*(actual++) = rho * ((1 - Snota) * (*anterior) + Snota * (*(anterior - 1)));
				if (randomSamplePositivo() > b) *actual = -(*actual);
				anterior++;
			}
	}
	//Else - Just dampen
	else {
		while (comparador++ < maximo)
		{
			*(actual++) = 0.5 * rho * (*anterior) + (*(anterior - 1));
			if (randomSamplePositivo() > b) * actual = -(*actual);
			anterior++;
		}
	}
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
void Voz::promediarApagar(float*& anterior, float*& actual, float* hasta, float* comparador, float* fineTunearDesde, float* fineTunearHasta) {

	 //Seteo un dampen y hago exactamente lo mismo que el promedio normal

	 rho *= pow(voiceNote/127., 0.05);  //Quiero que las notas más graves tengan mayor rho que las agudas
	 //Todo: Ver si no es mejor setear este rho solamente en la llamada de ApagarVoz, porque sino se sigue achicando cada vez
	 promediar(anterior, actual, hasta, comparador, fineTunearDesde, fineTunearHasta);

	return;
}


//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//Allpass filter para finetunear, cuando el periodo en samples no es entero
//yn = Cxn +xn-1 - Cyn-1
//Lo más óptimo es poner esto en el loop de la funcion de promediar, para acceder a memoria solo una vez y no dos veces, pero por ahora prefiero salvar la legibilidad
//La verdad no le escucho diferencia
void Voz::fineTunear(float* actual, float* hasta) {

	//Si justo n = 0, no hay anterior o está al final del buffer. Si hay último sample válido (xn_1 != 0., porque estoy precalculando más samples, no es que acabo de trigerear) uso ese
	if (actual == buffer) {
		if(xn_1 == 0.)	*actual *= C ;
		else *actual = C * (*actual) + xn_1 - C * *(buffer + si);
		actual++;
	}
	
	while (actual < hasta) //Tood: volver a optimizar con el ++ acá dentro como tenia antes
	{
		float x_n = *actual;
		//Filtro pasabajos de cada ciclo
		*actual = C *x_n + xn_1 - C * *(actual - 1);
		xn_1 = x_n;
		actual++;
	}
	

}
