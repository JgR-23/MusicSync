#ifndef REPORTES_H
#define REPORTES_H

#include "protocolo.h"
#include "sesion.h"

/* m->arg = REPORTE_DIARIO / REPORTE_SEMANAL / REPORTE_MENSUAL.
 * Devuelve el reporte como texto en m->datos y lo guarda en
 * datos/reporte_diario.txt, reporte_semanal.txt o reporte_mensual.txt */
void srv_reporte(Mensaje *m, const Sesion *s);

#endif
