#include "almacen.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define RUTA_USUARIOS      "datos/usuarios.txt"
#define RUTA_CANCIONES     "datos/canciones.txt"
#define RUTA_PLAYLISTS     "datos/playlists.txt"
#define RUTA_PLAYLISTS_TMP "datos/playlists.tmp"

#define MAX_LINEA     512
#define MAX_CAMPOS    8
#define MAX_PL_TOTAL  500
#define LARGO_HASH    17   /* 16 digitos hexadecimales + '\0' */

typedef struct {
    char usuario[IPC_MAX_USUARIO];
    char nombre[IPC_MAX_PLAYLIST];
    int  ids[IPC_MAX_CANCIONES_PLAYLIST];
    int  n;
} PlaylistReg;

static PlaylistReg tabla_pl[MAX_PL_TOTAL];
static int total_pl = 0;

static RegCancion catalogo[IPC_MAX_CATALOGO];
static int total_catalogo = 0;

/* -------------------------------------------------- */
/* Utilidades de texto                                */
/* -------------------------------------------------- */

static void poner_error(char *err, size_t n, const char *mensaje){
    snprintf(err, n, "%s", mensaje);
}

static void quitar_salto(char *s){
    size_t largo = strlen(s);

    while(largo > 0 && (s[largo - 1] == '\n' || s[largo - 1] == '\r')){
        s[--largo] = '\0';
    }
}

static void recortar(char *s){
    size_t largo = strlen(s);
    size_t ini = 0;

    while(largo > 0 && isspace((unsigned char)s[largo - 1])){
        largo--;
    }
    s[largo] = '\0';

    while(s[ini] != '\0' && isspace((unsigned char)s[ini])){
        ini++;
    }
    if(ini > 0){
        memmove(s, s + ini, strlen(s + ini) + 1);
    }
}

/* El separador de los archivos es '|', asi que no puede ir dentro de un dato */
static int texto_valido(const char *s){
    return strchr(s, '|') == NULL && strchr(s, '\n') == NULL && strchr(s, '\r') == NULL;
}

/* Lineas vacias y comentarios (#) se ignoran */
static int es_dato(const char *linea){
    return linea[0] != '\0' && linea[0] != '#';
}

/* Divide la linea en el mismo buffer; conserva los campos vacios */
static int dividir(char *linea, char *campos[], int max){
    int n = 0;
    char *p = linea;

    campos[n++] = p;

    while(*p != '\0' && n < max){
        if(*p == '|'){
            *p = '\0';
            campos[n++] = p + 1;
        }
        p++;
    }

    return n;
}

/* FNV-1a de 64 bits sobre "usuario:clave" (didactico, no criptografico) */
static void hash_clave(const char *usuario, const char *clave, char *salida){
    unsigned long long h = 0xcbf29ce484222325ULL;
    const char *p;

    for(p = usuario; *p != '\0'; p++){
        h ^= (unsigned char)*p;
        h *= 0x100000001b3ULL;
    }

    h ^= (unsigned char)':';
    h *= 0x100000001b3ULL;

    for(p = clave; *p != '\0'; p++){
        h ^= (unsigned char)*p;
        h *= 0x100000001b3ULL;
    }

    snprintf(salida, LARGO_HASH, "%016llx", h);
}

/* -------------------------------------------------- */
/* Usuarios                                           */
/* -------------------------------------------------- */

/* Devuelve 1 si existe; copia el hash guardado si hash_salida != NULL */
static int buscar_usuario(const char *usuario, char *hash_salida){
    FILE *f = fopen(RUTA_USUARIOS, "r");
    char linea[MAX_LINEA];
    char *campos[MAX_CAMPOS];
    int encontrado = 0;

    if(f == NULL){
        return 0;
    }

    while(!encontrado && fgets(linea, sizeof(linea), f) != NULL){
        quitar_salto(linea);

        if(!es_dato(linea)){
            continue;
        }

        if(dividir(linea, campos, MAX_CAMPOS) >= 3 && strcmp(campos[0], usuario) == 0){
            if(hash_salida != NULL){
                snprintf(hash_salida, LARGO_HASH, "%s", campos[2]);
            }
            encontrado = 1;
        }
    }

    fclose(f);
    return encontrado;
}

static int existe_correo(const char *correo){
    FILE *f = fopen(RUTA_USUARIOS, "r");
    char linea[MAX_LINEA];
    char *campos[MAX_CAMPOS];
    int encontrado = 0;

    if(f == NULL){
        return 0;
    }

    while(!encontrado && fgets(linea, sizeof(linea), f) != NULL){
        quitar_salto(linea);

        if(!es_dato(linea)){
            continue;
        }

        if(dividir(linea, campos, MAX_CAMPOS) >= 3 && strcasecmp(campos[1], correo) == 0){
            encontrado = 1;
        }
    }

    fclose(f);
    return encontrado;
}

int almacen_registrar_usuario(const char *usuario, const char *correo, const char *clave, char *err, size_t n){
    char u[IPC_MAX_USUARIO];
    char c[IPC_MAX_CORREO];
    char hash[LARGO_HASH];
    FILE *f;

    snprintf(u, sizeof(u), "%s", usuario);
    snprintf(c, sizeof(c), "%s", correo);
    recortar(u);
    recortar(c);

    if(u[0] == '\0' || c[0] == '\0' || clave[0] == '\0'){
        poner_error(err, n, "Faltan datos del registro");
        return -1;
    }

    if(!texto_valido(u) || !texto_valido(c)){
        poner_error(err, n, "No se permite el caracter | en los datos");
        return -1;
    }

    if(buscar_usuario(u, NULL)){
        poner_error(err, n, "Ese usuario ya existe");
        return -1;
    }

    if(existe_correo(c)){
        poner_error(err, n, "Ese correo ya esta registrado");
        return -1;
    }

    hash_clave(u, clave, hash);

    f = fopen(RUTA_USUARIOS, "a+");
    if(f == NULL){
        poner_error(err, n, "No se pudo guardar el usuario");
        return -1;
    }

    /* Si el archivo no termina en salto de linea, lo agrega para no pegar registros */
    if(fseek(f, 0, SEEK_END) == 0 && ftell(f) > 0){
        if(fseek(f, -1, SEEK_END) == 0 && fgetc(f) != '\n'){
            fseek(f, 0, SEEK_END);
            fputc('\n', f);
        }
    }

    fseek(f, 0, SEEK_END);
    fprintf(f, "%s|%s|%s\n", u, c, hash);

    if(fclose(f) != 0){
        poner_error(err, n, "No se pudo guardar el usuario");
        return -1;
    }

    return 0;
}

int almacen_validar_usuario(const char *usuario, const char *clave, char *err, size_t n){
    char esperado[LARGO_HASH];
    char hash[LARGO_HASH];

    if(usuario[0] == '\0' || clave[0] == '\0'){
        poner_error(err, n, "Faltan usuario o contrasena");
        return -1;
    }

    if(!buscar_usuario(usuario, esperado)){
        poner_error(err, n, "Usuario o contrasena incorrectos");
        return -1;
    }

    hash_clave(usuario, clave, hash);

    if(strcmp(hash, esperado) != 0){
        poner_error(err, n, "Usuario o contrasena incorrectos");
        return -1;
    }

    return 0;
}

/* -------------------------------------------------- */
/* Catalogo de canciones                              */
/* -------------------------------------------------- */

/* Lee datos/canciones.txt en "catalogo". Devuelve la cantidad o -1 */
static int cargar_catalogo(void){
    FILE *f = fopen(RUTA_CANCIONES, "r");
    char linea[MAX_LINEA];
    char *campos[MAX_CAMPOS];

    total_catalogo = 0;

    if(f == NULL){
        return -1;
    }

    while(total_catalogo < IPC_MAX_CATALOGO && fgets(linea, sizeof(linea), f) != NULL){
        RegCancion *c;

        quitar_salto(linea);

        if(!es_dato(linea)){
            continue;
        }

        if(dividir(linea, campos, MAX_CAMPOS) < 7){
            continue;
        }

        c = &catalogo[total_catalogo];
        memset(c, 0, sizeof(*c));

        c->id = atoi(campos[0]);
        snprintf(c->titulo,   sizeof(c->titulo),   "%s", campos[1]);
        snprintf(c->artista,  sizeof(c->artista),  "%s", campos[2]);
        snprintf(c->album,    sizeof(c->album),    "%s", campos[3]);
        snprintf(c->genero,   sizeof(c->genero),   "%s", campos[4]);
        snprintf(c->duracion, sizeof(c->duracion), "%s", campos[5]);
        c->precio = atof(campos[6]);

        total_catalogo++;
    }

    fclose(f);
    return total_catalogo;
}

static const RegCancion *buscar_cancion(int id){
    int i;

    for(i = 0; i < total_catalogo; i++){
        if(catalogo[i].id == id){
            return &catalogo[i];
        }
    }

    return NULL;
}

int almacen_listar_catalogo(RegCancion *salida, int max, char *err, size_t n){
    int total = cargar_catalogo();

    if(total < 0){
        poner_error(err, n, "No se pudo leer el catalogo");
        return -1;
    }

    if(total > max){
        total = max;
    }

    memcpy(salida, catalogo, (size_t)total * sizeof(RegCancion));
    return total;
}

/* -------------------------------------------------- */
/* Playlists                                          */
/* -------------------------------------------------- */

static void cargar_playlists(void){
    FILE *f = fopen(RUTA_PLAYLISTS, "r");
    char linea[MAX_LINEA];
    char *campos[MAX_CAMPOS];

    total_pl = 0;

    if(f == NULL){
        return;   /* todavia no existe: no hay playlists */
    }

    while(total_pl < MAX_PL_TOTAL && fgets(linea, sizeof(linea), f) != NULL){
        PlaylistReg *p;
        int campos_leidos;

        quitar_salto(linea);

        if(!es_dato(linea)){
            continue;
        }

        campos_leidos = dividir(linea, campos, MAX_CAMPOS);
        if(campos_leidos < 2){
            continue;
        }

        p = &tabla_pl[total_pl];
        memset(p, 0, sizeof(*p));

        snprintf(p->usuario, sizeof(p->usuario), "%s", campos[0]);
        snprintf(p->nombre,  sizeof(p->nombre),  "%s", campos[1]);

        if(campos_leidos >= 3){
            char *tok = strtok(campos[2], ",");

            while(tok != NULL && p->n < IPC_MAX_CANCIONES_PLAYLIST){
                p->ids[p->n++] = atoi(tok);
                tok = strtok(NULL, ",");
            }
        }

        total_pl++;
    }

    fclose(f);
}

/* Escribe en un temporal y lo renombra para no dejar el archivo a medias */
static int guardar_playlists(void){
    FILE *f = fopen(RUTA_PLAYLISTS_TMP, "w");
    int i, j;

    if(f == NULL){
        return -1;
    }

    for(i = 0; i < total_pl; i++){
        fprintf(f, "%s|%s|", tabla_pl[i].usuario, tabla_pl[i].nombre);

        for(j = 0; j < tabla_pl[i].n; j++){
            fprintf(f, "%s%d", j > 0 ? "," : "", tabla_pl[i].ids[j]);
        }

        fprintf(f, "\n");
    }

    if(fclose(f) != 0){
        return -1;
    }

    return rename(RUTA_PLAYLISTS_TMP, RUTA_PLAYLISTS) == 0 ? 0 : -1;
}

static int buscar_playlist(const char *usuario, const char *nombre){
    int i;

    for(i = 0; i < total_pl; i++){
        if(strcmp(tabla_pl[i].usuario, usuario) == 0 && strcmp(tabla_pl[i].nombre, nombre) == 0){
            return i;
        }
    }

    return -1;
}

int almacen_listar_playlists(const char *usuario, RegPlaylist *salida, int max){
    int i, total = 0;

    cargar_playlists();

    for(i = 0; i < total_pl && total < max; i++){
        if(strcmp(tabla_pl[i].usuario, usuario) == 0){
            snprintf(salida[total].nombre, sizeof(salida[total].nombre), "%s", tabla_pl[i].nombre);
            salida[total].cantidad = tabla_pl[i].n;
            total++;
        }
    }

    return total;
}

int almacen_crear_playlist(const char *usuario, const char *nombre, char *err, size_t n){
    char limpio[IPC_MAX_PLAYLIST];
    int i, propias = 0;

    if(strlen(nombre) >= sizeof(limpio)){
        poner_error(err, n, "El nombre es demasiado largo");
        return -1;
    }

    snprintf(limpio, sizeof(limpio), "%s", nombre);
    recortar(limpio);

    if(limpio[0] == '\0'){
        poner_error(err, n, "El nombre esta vacio");
        return -1;
    }

    if(!texto_valido(limpio)){
        poner_error(err, n, "El nombre no puede contener |");
        return -1;
    }

    cargar_playlists();

    for(i = 0; i < total_pl; i++){
        if(strcmp(tabla_pl[i].usuario, usuario) == 0){
            propias++;

            if(strcmp(tabla_pl[i].nombre, limpio) == 0){
                poner_error(err, n, "Ese nombre ya existe");
                return -1;
            }
        }
    }

    if(propias >= IPC_MAX_PLAYLISTS){
        poner_error(err, n, "Limite de playlists alcanzado");
        return -1;
    }

    if(total_pl >= MAX_PL_TOTAL){
        poner_error(err, n, "El servidor no admite mas playlists");
        return -1;
    }

    memset(&tabla_pl[total_pl], 0, sizeof(PlaylistReg));
    snprintf(tabla_pl[total_pl].usuario, sizeof(tabla_pl[total_pl].usuario), "%s", usuario);
    snprintf(tabla_pl[total_pl].nombre,  sizeof(tabla_pl[total_pl].nombre),  "%s", limpio);
    total_pl++;

    if(guardar_playlists() < 0){
        poner_error(err, n, "No se pudo guardar la playlist");
        return -1;
    }

    return 0;
}

int almacen_eliminar_playlist(const char *usuario, const char *nombre, char *err, size_t n){
    int idx, i;

    cargar_playlists();
    idx = buscar_playlist(usuario, nombre);

    if(idx < 0){
        poner_error(err, n, "Playlist no encontrada");
        return -1;
    }

    for(i = idx; i < total_pl - 1; i++){
        tabla_pl[i] = tabla_pl[i + 1];
    }
    total_pl--;

    if(guardar_playlists() < 0){
        poner_error(err, n, "No se pudo guardar los cambios");
        return -1;
    }

    return 0;
}

int almacen_listar_canciones_playlist(const char *usuario, const char *nombre, RegCancion *salida, int max, char *err, size_t n){
    int idx, i, total = 0;
    const RegCancion *c;

    cargar_playlists();
    idx = buscar_playlist(usuario, nombre);

    if(idx < 0){
        poner_error(err, n, "Playlist no encontrada");
        return -1;
    }

    if(cargar_catalogo() < 0){
        poner_error(err, n, "No se pudo leer el catalogo");
        return -1;
    }

    for(i = 0; i < tabla_pl[idx].n && total < max; i++){
        c = buscar_cancion(tabla_pl[idx].ids[i]);

        if(c != NULL){
            salida[total++] = *c;
        }
    }

    return total;
}

int almacen_agregar_cancion(const char *usuario, const char *nombre, int id_cancion, char *err, size_t n){
    PlaylistReg *p;
    int idx, i;

    cargar_playlists();
    idx = buscar_playlist(usuario, nombre);

    if(idx < 0){
        poner_error(err, n, "Playlist no encontrada");
        return -1;
    }

    if(cargar_catalogo() < 0){
        poner_error(err, n, "No se pudo leer el catalogo");
        return -1;
    }

    if(buscar_cancion(id_cancion) == NULL){
        poner_error(err, n, "Cancion no encontrada");
        return -1;
    }

    p = &tabla_pl[idx];

    for(i = 0; i < p->n; i++){
        if(p->ids[i] == id_cancion){
            poner_error(err, n, "La cancion ya esta en la playlist");
            return -1;
        }
    }

    if(p->n >= IPC_MAX_CANCIONES_PLAYLIST){
        poner_error(err, n, "La playlist esta llena");
        return -1;
    }

    p->ids[p->n++] = id_cancion;

    if(guardar_playlists() < 0){
        poner_error(err, n, "No se pudo guardar los cambios");
        return -1;
    }

    return 0;
}

int almacen_quitar_cancion(const char *usuario, const char *nombre, int id_cancion, char *err, size_t n){
    PlaylistReg *p;
    int idx, i, pos = -1;

    cargar_playlists();
    idx = buscar_playlist(usuario, nombre);

    if(idx < 0){
        poner_error(err, n, "Playlist no encontrada");
        return -1;
    }

    p = &tabla_pl[idx];

    for(i = 0; i < p->n; i++){
        if(p->ids[i] == id_cancion){
            pos = i;
            break;
        }
    }

    if(pos < 0){
        poner_error(err, n, "La cancion no esta en la playlist");
        return -1;
    }

    for(i = pos; i < p->n - 1; i++){
        p->ids[i] = p->ids[i + 1];
    }
    p->n--;

    if(guardar_playlists() < 0){
        poner_error(err, n, "No se pudo guardar los cambios");
        return -1;
    }

    return 0;
}
