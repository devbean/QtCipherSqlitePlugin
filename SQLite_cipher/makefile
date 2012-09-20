all:libsqlite.a
	@echo All Done
libsqlite.a:sqlite3secure.o
	ar -r libsqlite.a sqlite3secure.o
sqlite3secure.o:sqlite3secure.c sqlite3.c sqlite3.h codec.c codec.h rijndael.h rijndael.c codecext.c
	gcc -c -D SQLITE_HAS_CODEC sqlite3secure.c -o sqlite3secure.o
clean:
	del *.o *.a *.obj *.gch