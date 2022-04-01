#ifndef RESOLVER_H
#define RESOLVER_H

struct addrinfo;

/*
 * Resolverdor de hostnames y service names.
 * Por simplificacion este TDA se enfocara solamente
 * en direcciones IPv4 para TCP.
 * */
struct resolver_t {
    struct addrinfo *result; // privado, no accedas a este atributo
    struct addrinfo *next_; // privado, no accedas a este atributo

    /* Inicializa la estructura y resuelve el dado nombre del host y servicio.
     * Si passive es True, la resolucion se hace pensando que se quiere
     * inicializar un socket pasivo, de otro modo sera para un socket activo.
     *
     * Retorna 0 en caso de exito, -1 en caso de error.
     * */
    int init(const char* hostname, const char* servicename, bool passive);


    /* Retorna si hay o no una direccion siguiente para testear.
     * Si la hay, se debera llamar a resolver_t::next() para obtenerla.
     *
     * Si no la hay se puede asumir que el resolver esta extinguido.
     * */
    bool has_next();

    /* Retorna la siguiente direccion para testear e internamente
     * mueve el iterador a la siguiente direccion.
     *
     * Si no existe una siguiente direccion el resultado es indefinido.
     * */
    struct addrinfo* next();

    /*
     * Libera los recursos.
     * */
    void deinit();

};

#endif
