#ifndef RESOLVER_ERROR_H
#define RESOLVER_ERROR_H

#include <exception>

/*
 * Clase que encapsula un "gai" error. Vease getaddrinfo()
 * */
class ResolverError : public std::exception {
    int gai_errno;

    public:
    ResolverError(int gai_errno);

    const char* what() noexcept;

    bool is_temporal_failure() const noexcept;
};
#endif
