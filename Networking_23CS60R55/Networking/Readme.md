
-> uuid package
    sudo apt-get install uuid-dev

-> to run 
    -> Client.c
        gcc Server.c -luuid -o server
        ./server
    -> Server.c
        gcc Server.c -o client
        ./client