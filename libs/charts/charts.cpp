#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include "charts.h"

using namespace std;

#define RESET "\033[0m"
#define ROJO "\033[31m"
#define VERDE "\033[32m"
#define AMARILLO "\033[33m"
#define AZUL "\033[34m"
#define MAGENTA "\033[35m"
#define CIAN "\033[36m"

void print_pie_chart(int *arreglo, char **nombres, int longitud)
{
	double total = std::accumulate(arreglo, arreglo + longitud, 0.0);

	if (total <= 0) {
		cout << "\n[!] Error: No hay datos para graficar." << endl;
		return;
	}

	const char *colores[] = {ROJO, VERDE, AMARILLO, AZUL, MAGENTA, CIAN};

	int radio = 10;
	double relacion_aspecto = 2.0;

	cout << "\n--- GRAFICA DE PASTEL ---\n\n";

	for (int y = -radio; y <= radio; y++) {
		for (int x = (int)(-radio * relacion_aspecto);
		     x <= (int)(radio * relacion_aspecto);
		     x++) {

			double dx = x / relacion_aspecto;
			double dy = y;
			double distancia = sqrt(dx * dx + dy * dy);

			if (distancia <= radio) {
				double angulo = atan2(dy, dx);
				if (angulo < 0)
					angulo += 2 * M_PI;

				double angulo_acumulado = 0;
				int etiqueta_actual = 0;

				for (int i = 0; i < longitud; i++) {
					double proporcion =
					    (arreglo[i] / total) * 2 * M_PI;
					angulo_acumulado += proporcion;
					if (angulo <= angulo_acumulado) {
						etiqueta_actual = i;
						break;
					}
				}
				cout << colores[etiqueta_actual % 6] << "*"
				     << RESET;
			} else {
				cout << " ";
			}
		}
		cout << endl;
	}

	cout << "\nLEYENDA DE ETIQUETAS:\n";
	for (int i = 0; i < longitud; i++) {
		float porc = (arreglo[i] / total) * 100.0;
		cout << colores[i % 6] << "(*) " << RESET << left << setw(15)
		     << nombres[i] << " -> " << fixed << setprecision(1) << porc
		     << "%" << endl;
	}
}
