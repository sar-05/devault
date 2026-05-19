#ifndef CHARTS_H
#define CHARTS_H

#ifdef __cplusplus
extern "C" {
#endif

void print_pie_chart(int *arreglo, char **nombres, int longitud);
void print_bar_chart(int *arreglo_enteros, char **nombres, int longitud_del_arreglo);

#ifdef __cplusplus
}
#endif

#endif
