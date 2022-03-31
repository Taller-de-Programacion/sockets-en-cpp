all:
	g++ -std=c++14 -ggdb -O0 -pedantic -Wall socket.cpp resolver.cpp get_page.cpp -o get_page
	g++ -std=c++14 -ggdb -O0 -pedantic -Wall socket.cpp resolver.cpp echo_server.cpp -o echo_server

