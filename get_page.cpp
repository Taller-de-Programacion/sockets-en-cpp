#include <iostream>
#include "socket.h"

/*
 * Este mini ejemplo se conecta via TCP a www.google.com.ar y se descarga una
 * pagina web.
 *
 * Esta codeado "a la C" por lo que veras muchas cosas hechas "a mano".
 * En particular veras como se manejan los errores en C con el uso de goto.
 *
 * El patron es el siguiente:
 *
 *  - reservo recurso A
 *  - si fallo, goto "after-A"
 *      - reservo recurso B
 *      - si fallo, goto "after-B"
 *          - reservo recurso C
 *          - si fallo, goto "after-C"
 *              ....
 *          - destruyo C
 *      - destruyo B                    ["after-C"]
 *  - destruyo A                        ["after-B"]
 *  - fin                               ["after-A"]
 *
 * Este patron de inicializacion y destruccion con goto evita:
 *  - tener que destruir el recurso en mas de un lugar (nos podriamos olvidar
 *    y tener un leak)
 *  - el riesgo de destruir algo que no fue inicializado (que nos daria
 *    un "use uninitialized variable"
 *
 * Ahora es claro que este esquema de goto es *muy manual*.
 *
 * En Golang podras usar los "defer" para mitigar el problema. En C++ y en Rust
 * tendras RAII para resolverlo completamente (RAII = Resource Acquisition is Initialization)
 * */
int main() {
    int ret = -1;
    int s = -1;
    bool was_closed = false;

    const char req[] = "GET / HTTP/1.1\r\nAccept: */*\r\nConnection: close\r\nHost: www.google.com.ar\r\n\r\n";

    /*
     * Inicializamos el socket para que se conecte a google.com
     * contra el servicio http.
     *
     * Si la conexion falla, cerramos el programa.
     * Notese como si la conexion falla ni siquiera debemos/podemos
     * desinicializar el socket ya que la funcion que la inicializa
     * fallo y por lo tanto una desinicializacion podria terminar
     * en liberar recursos que no fueron reservados en primer lugar
     * */
    Socket skt("www.google.com.ar", "http");
    if (s == -1)
        goto connection_failed;

    /*
     * Enviamos el HTTP Request, el pedido de pagina web. Como sabemos
     * que no queremos enviar nada mas podemos usar socket_sendall()
     * para que envie todo.
     * En caso que el send() no pueda enviar todo de un solo golpe,
     * socket_sendall() fue codeada para hacer "el loop" por nosotros.
     *
     * Notese ademas que si falla (esta u otra funcion abajo) cerramos
     * el programa pero sin olvidarnos de liberar el socket ya que
     * aunque el send()/recv() fallen, los recursos *si* fueron reservados
     * por nosotros.
     * */
    s = skt.sendall(req, sizeof(req) - 1, &was_closed);
    if (s != sizeof(req) - 1)
        goto send_req_failed;

    /*
     * Iteramos leyendo/recibiendo de a cachos la pagina web.
     * No podemos usar socket_recvall() ya que no sabemos a priori
     * el tamaño de la respuesta.
     * Notese ademas que con socket_recvsome() no se exige q la respuesta
     * tenga exactamente el size del buffer: sera nuestro trabajo hacer
     * el loop aqui.
     * */
    char buf[512];
    while (not was_closed) {
        int r = skt.recvsome(buf, sizeof(buf) - 1, &was_closed);
        if (was_closed)
            break;

        if (r < 0)
            goto recv_failed;

        /*
         * Recorda que con sockets se envian/reciben *bytes*, no texto.
         * La respuesta de google seran bytes y no necesariamente terminaran
         * en un \0 asi que es necesario poner ese "fin de string" nosotros.
         * */
        buf[r] = 0;
        std::cout << buf;
    }

    ret = 0;

    // Por que instanciamos el Socket en el stack, cuando la funcion main()
    // termine se llamara al destructor de Socket automaticamente
    // lo que significa que no tenemos que acordarnos de liberar
    // las cosas. Este es el poder de RAII (Resource Acquisition is Initialization)
recv_failed:
send_req_failed:
connection_failed:
    return ret;
}
