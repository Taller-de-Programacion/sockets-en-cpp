#include "resolver.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

int resolver_init(struct resolver_t *self, const char* hostname, const char* servicename, bool passive) {
    struct addrinfo hints;
    self->result = self->next = nullptr;

    /*
     * getaddrinfo nos puede retornar multiples direcciones incluso de
     * protocolos/tecnologias que no nos interesan.
     * Para pre-seleccionar que direcciones nos interesan le pasamos
     * un hint, una estructura con algunos campos completados (no todos)
     * que le indicaran que tipo de direcciones queremos.
     * */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       /* IPv4 (or AF_INET6 for IPv6)     */
    hints.ai_socktype = SOCK_STREAM; /* TCP  (or SOCK_DGRAM for UDP)    */
    hints.ai_flags = passive ? AI_PASSIVE : 0;  /* AI_PASSIVE for server; 0 for client */


    /* Obtengo la (o las) direcciones segun el nombre de host y servicio que
     * busco
     *
     * De todas las direcciones posibles, solo me interesan aquellas que sean
     * IPv4 y TCP (segun lo definido en hints)
     *
     * El resultado lo guarda en result que es un puntero al primer nodo
     * de una lista simplemente enlazada.
     * */
    int s = getaddrinfo(hostname, servicename, &hints, &self->result);

    /* Es muy importante chequear los errores.
     * En caso de uno, usar gai_strerror para traducir el codigo de error (s)
     * a un mensaje humanamente entendible.
     * */
    if (s != 0) {
        printf("Error in getaddrinfo: %s\n", gai_strerror(s));
        return 1;  // TODO lanzar una excepcion
    }

    self->next = self->result;
    return 0;
}

bool resolver_has_next(struct resolver_t *self) {
    return self->next != NULL;
}

struct addrinfo* resolver_next(struct resolver_t *self) {
    struct addrinfo *ret = self->next;
    self->next = ret->ai_next;
    return ret;
}

void resolver_deinit(struct resolver_t *self) {
    /*
     * Como la lista reservada por getaddrinfo() es dinamica requiere
     * una desinicializacion acorde.
     *
     * Podes imaginarte que getaddrinfo() hace un malloc() y que
     * el freeaddrinfo() es su correspondiente free()
     * */
    freeaddrinfo(self->result);
}

