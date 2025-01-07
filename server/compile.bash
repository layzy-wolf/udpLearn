gcc -c libserver/server.c -o libserver/server.o
ar rcs libserver/libserver.a libserver/server.o
gcc main.c -L./libserver -lserver -o main 