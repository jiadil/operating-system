all: my_userid.c my_storage_use.c my_shells.c
	gcc -g -Wall -o my_userid my_userid.c
	gcc -g -Wall -o my_storage_use my_storage_use.c
	gcc -g -Wall -o my_shells my_shells.c
clean:
	$(RM) my_userid my_storage_use my_shells