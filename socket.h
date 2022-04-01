#ifndef SOCKET_H
#define SOCKET_H

/*
 * Socket.
 * Por simplificacion este TDA se enfocara solamente
 * en sockets IPv4 para TCP.
 * */
class Socket {
    int skt;
    bool closed;

    int init_with_file_descriptor(Socket *self, int skt);

    public:
    /*
     * Construye el socket tanto para conectarse a un servidor
     * (primer constructor) como para inicializarlo para ser usado
     * por un servidor (segundo constructor).
     *
     * Muchas librerias de muchos lenguajes ofrecen una unica formal de inicializar
     * los sockets y luego metodos (post-inicializacion) para establer
     * la conexion o ponerlo en escucha.
     *
     * Otras librerias/lenguajes van por tener una inicializacion para
     * el socket activo y otra para el pasivo.
     *
     * Este codigo es un ejemplo de ello.
     *
     * */
    Socket(const char *hostname, const char *servicename);
    Socket(const char *servicename);

    /*
     * Un constructor sin parametros es el llamado constructor por default.
     * En ciertos casos es necesario pero suelen ser un peligro ya que
     * no suelen poder inicializar correctamente al objeto.
     *
     * En nuestro caso es un HACK para poder implementar Socket::accept
     * pero lo resolveremos con Move Semantics pronto....
     * */
    Socket();

    /* Socket::sendsome() lee hasta sz bytes del buffer y los envia. La funcion
     * puede enviar menos bytes sin embargo.
     *
     * Socket::recvsome() por el otro lado recibe hasta sz bytes y los escribe
     * en el buffer (que debe estar pre-allocado). La funcion puede recibir
     * menos bytes sin embargo.
     *
     * Si el socket detecto que la conexion fue cerrada, la variable
     * was_closed es puesta a True, de otro modo sera False.
     *
     * Retorna 0 si se cerro el socket, menor a 0 si hubo un error
     * o positivo que indicara cuantos bytes realmente se enviaron/recibieron.
     *
     * Lease man send y man recv
     * */
    int sendsome(const void *data, unsigned int sz, bool *was_closed);
    int recvsome(void *data, unsigned int sz, bool *was_closed);

    /*
     * Socket::sendall() envia exactamente sz bytes leidos del buffer, ni mas,
     * ni menos. Socket::recvall() recibe exactamente sz bytes.
     *
     * Si hay un error o el socket se cerro durante el envio/recibo de los bytes
     * no hay forma certera de saber cuantos bytes realmente se enviaron/recibieron.
     *
     * En caso de error se retorna un valor negativo, se retorna 0
     * si el socket se cerro o un valor positivo igual a sz si se envio/recibio
     * todo lo pedido.
     *
     * En caso de que se cierre el socket, was_closed es puesto a True.
     * */
    int sendall(const void *data, unsigned int sz, bool *was_closed);
    int recvall(void *data, unsigned int sz, bool *was_closed);

    /*
     * Acepta una conexion entrante e inicializa con ella el socket peer.
     * Dicho socket peer debe estar *sin* inicializar y si Socket::accept() es
     * exitoso, se debe llamar a Socket::deinit() sobre él.
     *
     * Retorna -1 en caso de error, 0 de otro modo.
     * */
    int accept(struct Socket *peer);

    /*
     * Cierra la conexion ya sea parcial o completamente.
     * Lease man 2 shutdown
     * */
    int shutdown(int how);

    /*
     * Cierra el socket. El cierre no implica un shutdown
     * que debe ser llamado explicitamente.
     * */
    int close();

    /*
     * Desinicializa el socket. Si aun esta conectado,
     * se llamara a Socket::shutdown() y Socket::close()
     * automaticamente.
     * */
    ~Socket();

};

#endif

