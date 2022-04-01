#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "socket.h"
#include "resolver.h"

int socket_t::init_for_connection(const char *hostname, const char *servicename) {
    class resolver_t resolver;
    int s = resolver.init(hostname, servicename, false);
    if (s == -1)
        return -1;

    int skt = -1;
    while (resolver.has_next()) {
        struct addrinfo *addr = resolver.next();

        /* Cerramos el socket si nos quedo abierto de la iteracion
         * anterior
         * */
        if (skt != -1)
            ::close(skt);

        /* Creamos el socket definiendo la familia (deberia ser AF_INET IPv4),
           el tipo de socket (deberia ser SOCK_STREAM TCP) y el protocolo (0) */
        skt = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (skt == -1) {
            continue;
        }

        /* Intentamos conectarnos al servidor cuya direccion
         * fue dada por getaddrinfo()
         * */
        s = connect(skt, addr->ai_addr, addr->ai_addrlen);
        if (s == -1) {
            continue;
        }

        // Conexion exitosa!
        this->skt = skt;
        resolver.deinit();
        return 0;
    }

    // Este perror() va a imprimir el ultimo error generado.
    // Es importanto no llamar nada antes ya que cualquier llamada
    // a la libc puede cambiar el errno y hacernos perder el mensaje
    // El manejo de errores en C es muy sensible!
    perror("socket connection failed");

    // No hay q olvidarse de cerrar el socket en caso de
    // que lo hayamos abierto
    if (skt != -1)
        ::close(skt);
    resolver.deinit();
    return -1;
}

int socket_t::init_for_listen(const char *servicename) {
    class resolver_t resolver;
    int s = resolver.init(nullptr, servicename, true);
    if (s == -1)
        return -1;

    int skt = -1;
    while (resolver.has_next()) {
        struct addrinfo *addr = resolver.next();

        /* Cerramos el socket si nos quedo abierto de la iteracion
         * anterior
         * */
        if (skt != -1)
            ::close(skt);

        /* Creamos el socket definiendo la misma familia, tipo y protocolo
         * que tiene la direccion que estamos por probar.
         * Por ejemplo deberia ser la familia AF_INET IPv4,
         * el tipo de socket SOCK_STREAM TCP  y el protocolo 0.
         *
         * Ya que usamos getaddrinfo() no conviene hardcodear esos valores
         * sino usar los mismos que ya estan cargados en la direccion
         * que estamos probando.
         * */
        skt = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (skt == -1) {
            continue;
        }

        /*
         * Como el sistema operativo puede mantener ocupado (en TIME_WAIT)
         * un puerto que fue usado recientemente, no queremos que nuesta
         * aplicacion falle a la hora de hacer un bind() por algo que sabemos
         * que es temporal.
         *
         * Un puerto en TIME_WAIT esta "ocupado" solo temporalmente por
         * el sistema operativo.
         *
         * Entonces queremos decirle que "queremos reutilizar la direccion"
         * aun si esta en TIME_WAIT (de otro modo obtendriamos un error al
         * llamar a bind() con un "Address already in use")
         * */
        int val = 1;
        s = setsockopt(skt, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        if (s == -1) {
            continue;
        }

        // Hacemos le bind: enlazamos el socket a una direccion local
        // para escuchar
        s = bind(skt, addr->ai_addr, addr->ai_addrlen);
        if (s == -1) {
            continue;
        }

        // Ponemos el socket a escuchar. Ese 20 (podria ser otro valor)
        // indica cuantas conexiones a la espera de ser aceptadas se toleraran
        // No tiene nada q ver con cuantas conexiones totales el server tendra
        s = listen(skt, 20);
        if (s == -1) {
            continue;
        }

        // setupeamos el socket! Ahora esta escuchando
        // en una de las direcciones obtenidas por getaddrinfo()
        // y esta listo para aceptar conexiones.
        this->skt = skt;
        resolver.deinit();
        return 0;
    }

    // Este perror() va a imprimir el ultimo error generado.
    // Es importanto no llamar nada antes ya que cualquier llamada
    // a la libc puede cambiar el errno y hacernos perder el mensaje
    // El manejo de errores en C es muy sensible!
    perror("socket setup failed");

    // No hay q olvidarse de cerrar el socket en caso de
    // que lo hayamos abierto
    if (skt != -1)
        ::close(skt);
    resolver.deinit();
    return -1;
}

/*
 * Inicializa el socket pasandole directamente el file descriptor.
 *
 * No queremos que el codigo del usuario este manipulando el file descriptor,
 * queremos q interactue con él *solo* a traves de socket_t.
 *
 * Por ello ponemos este metodo privado (vease socket.h).
 * */
int socket_t::init_with_file_descriptor(struct socket_t *self, int skt) {
    self->skt = skt;
    self->closed = false;

    return 0;
}

int socket_t::recvsome(void *data, unsigned int sz, bool *was_closed) {
    *was_closed = false;
    int s = recv(this->skt, (char*)data, sz, 0);
    if (s == 0) {
        // Puede ser o no un error, dependera del protocolo.
        // Alguno protocolo podria decir "se reciben datos hasta
        // que la conexion se cierra" en cuyo caso el cierre del socket
        // no es un error sino algo esperado.
        *was_closed = true;
        return 0;
    } else if (s < 0) {
        // 99% casi seguro que es un error real
        perror("socket recv failed");
        return s;
    } else {
        return s;
    }
}

int socket_t::sendsome(const void *data, unsigned int sz, bool *was_closed) {
    *was_closed = false;
    int s = send(this->skt, (char*)data, sz, MSG_NOSIGNAL);
    if (s == 0) {
        // Puede o no ser un error (vease el comentario en recvsome())
        *was_closed = true;
        return 0;
    } else if (s < 0) {
        // Este es un caso especial: cuando enviamos algo pero en el medio
        // se detecta un cierre del socket no se sabe bien cuanto se logro
        // enviar (y fue recibido por el peer) y cuanto se perdio.
        //
        // Se dice que la "tuberia esta rota" o en ingles, "broken pipe"
        //
        // En Linux el sistema operativo envia una signal (SIGPIPE) que
        // mata al proceso. El flag MSG_NOSIGNAL evita eso y nos permite
        // checkear y manejar la condicion mas elegantemente
        if (errno == EPIPE) {
            // Puede o no ser un error (vease el comentario en recvsome())
            *was_closed = true;
            return 0;
        }

        // 99% casi seguro que es un error
        perror("socket send failed");
        return s;
    } else {
        return s;
    }
}

int socket_t::recvall(void *data, unsigned int sz, bool *was_closed) {
    unsigned int received = 0;
    *was_closed = false;

    while (received < sz) {
        int s = this->recvsome((char*)data + received, sz - received, was_closed);
        if (s <= 0) {
            // Si el socket fue cerrado (s == 0) o hubo un error (s < 0)
            // el metodo socket_t::recvsome ya deberia haber seteado was_closed
            // y haber notificado el error.
            // Nosotros podemos entonces meramente retornar.
            // (si no llamaramos a socket_t::recvsome() y llamaramos a recv()
            // deberiamos entonces checkear los errores y no solo retornarlos)
            return s;
        }
        else {
            // Ok, recibimos algo pero no necesariamente todo lo que
            // esperamos. La condicion del while checkea eso justamente
            received += s;
        }
    }

    return sz;
}


int socket_t::sendall(const void *data, unsigned int sz, bool *was_closed) {
    unsigned int sent = 0;
    *was_closed = false;

    while (sent < sz) {
        int s = this->sendsome((char*)data + sent, sz - sent, was_closed);
        if (s <= 0) {
            // Si el socket fue cerrado (s == 0) o hubo un error (s < 0)
            // el metodo socket_t::sendall ya deberia haber seteado was_closed
            // y haber notificado el error.
            // Nosotros podemos entonces meramente retornar.
            // (si no llamaramos a socket_t::sendsome() y llamaramos a send()
            // deberiamos entonces checkear los errores y no solo retornarlos)
            return s;
        } else {
            sent += s;
        }
    }

    return sz;
}

int socket_t::accept(struct socket_t *peer) {
    int skt = ::accept(this->skt, nullptr, nullptr);
    if (skt == -1)
        return -1;

    int s = init_with_file_descriptor(peer, skt);
    if (s == -1)
        return -1;

    return 0;
}

int socket_t::shutdown(int how) {
    if (::shutdown(this->skt, how) == -1) {
        perror("socket shutdown failed");
        return -1;
    }

    return 0;
}

int socket_t::close() {
    this->closed = true;
    return ::close(this->skt);
}

void socket_t::deinit() {
    if (not this->closed) {
        ::shutdown(this->skt, 2);
        ::close(this->skt);
    }
}
