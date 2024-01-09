all: Naming Client Storage_copy

Queue: Queue.c
	gcc queue.c

Naming: Naming_server.c
	gcc LRU.c log.c structs.c server.c Tries.c queue.c Naming_server.c -o Naming -lm

Client: Client.c
	gcc structs.c SS_functions.c queue.c Client.c -o Client -lm

Storage: Storage_server.c 
	gcc structs.c server.c SS_functions.c Storage_server.c -o Storage

# Rule to copy Storage object to 5 different directories
Storage_copy: Storage
	cp Storage 5200/
	cp Storage 5201/
	cp Storage 5202/
	cp Storage 5203/
	cp Storage 5204/
	cp Storage 5205/

clean:
	rm -rf 5200/Storage 5201/Storage 5202/Storage 5203/Storage 5204/Storage 5205/Storage
	rm -f Naming Client Storage a.out
