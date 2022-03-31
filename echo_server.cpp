#include <iostream>
#include "socket.h"

/*
 * Este mini ejemplo escucha en el puerto 3129 TCP y acepta a un unico
 * cliente. Todo lo que el cliente envie el servidor se lo reenviara.
 *
 * Es un echo server!
 *
 * Esta codeado "a la C" por lo que veras muchas cosas hechas "a mano".
 * En particular veras como se manejan los errores en C con el uso de goto.
 *
 * Escribi mucho mas en get_page.cpp, podes mirar ahi los detalles.
 *
 * Este servidor es muy basico: solo acepta a un unico cliente.
 * Es tan basico que incluso no finaliza hasta q ese cliente se conecte
 * (no hay forma de cerrarlo ordenadamente)
 *
 * Cuando veamos threads podremos implementar servidores multi-clientes
 * y podremos cerrar todo en orden.
 *
 * Si queres probar el server, corre en una consola:
 *
 *  nc 127.0.0.1 3129
 *
 **/
int main() {
    int ret = -1;
    int s = -1;
    bool was_closed = false;

    /*
     * Inicializamos nuestro socket "server" o "aceptador"
     * que usaremos para escuchar y aceptar conexiones entrantes.
     *
     * En este mini-ejemplo tendremos un socket srv (server) y
     * otro llamado peer que representara a nuestro cliente.
     * En general cualquier servidor real tendra N+1 sockets,
     * uno para escuchar y aceptar y luego N sockets para sus
     * N clientes.
     *
     * Otro detalle. getaddrinfo() no solo resuelve hostnames (www.google.com)
     * y service names (http) sino que tambien acepta direcciones
     * IP (127.0.0.1) y puertos (3129).
     *
     * En general es una mala idea hardcodear IPs/puertos, aca esta
     * con fines didacticos.
     * */
    struct socket_t peer, srv;
    s = socket_init_for_listen(&srv, "3129");
    if (s == -1)
        goto listening_failed;

    /*
     * Bloqueamos el programa hasta q haya una conexion entrante
     * y sea aceptada. Hablaremos (send/recv) con ese cliente
     * conectado en particular usando un socket distinto, el peer,
     * inicializado aqui.
     * */
    s = socket_accept(&srv, &peer);
    if (s == -1)
        goto accept_failed;

    /*
     * A partir de aqui podriamos volver a usar srv para aceptar
     * nuevos clientes a la vez q hablamos con peer pero
     * en este mini-ejemplo nos quedaremos con algo simple
     * de un solo cliente.
     * */

    char buf[512];
    while (not was_closed) {
        /*
         * Loop principal: lo que el servidor recibe lo reenvia
         * al cliente. Es un echo server despues de todo!
         *
         * Usamos socket_recvsome() por q no sabemos cuanto vamos a
         * recibir exactamente pero usamos socket_sendall() por
         * que sabemos cuanto queremos enviar.
         *
         * Podriamos usar tambien socket_sendsome() para hacer
         * ciertas optimizaciones pero aca nos quedamos con la
         * version simple (y facil de entender).
         * Si queres indagar mas podes ver la implementacion
         * de tiburoncin pero te advierto, es heavy.
         * https://github.com/eldipa/tiburoncin
         *
         * Pregunta: por que usamos sizeof(buf) en este socket_recvsome()
         * pero usamos sizeof(buf)-1 en el socket_recvsome() de get_page.cpp?
         * */
        int sz = socket_recvsome(&peer, buf, sizeof(buf), &was_closed);

        if (was_closed)
            break;

        if (sz < 0)
            goto recv_failed;

        s = socket_sendall(&peer, buf, sz, &was_closed);

        if (was_closed)
            break;

        if (sz < 0)
            goto send_failed;
    }

    ret = 0;

send_failed:
recv_failed:
    socket_shutdown(&peer, 2);
    socket_close(&peer);
    socket_deinit(&peer);

accept_failed:
    socket_shutdown(&srv, 2);
    socket_close(&srv);
    socket_deinit(&srv);

listening_failed:
    return ret;
}
