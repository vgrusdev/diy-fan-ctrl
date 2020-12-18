# build helloworld executable when user executes "make"
fan-ctrl:main.o tools.o pcf8575.o fan_main_loop.o pcf-test.o sql-select.o
	$(CC) $(LDFLAGS) -lsqlite3 main.o pcf8575.o tools.o fan_main_loop.o -o fan-ctrl
	$(CC) $(LDFLAGS) pcf-test.o pcf8575.o -o pcf-test
	$(CC) $(LDFLAGS) -lsqlite3 sql-select.o -o sql-select-fan
main.o: main.c pcf8575.h tools.h 
	$(CC) $(CFLAGS) -c main.c
tools.o: tools.c tools.h
	$(CC) $(CFLAGS) -c tools.c
pcf8575.o: pcf8575.c pcf8575.h
	$(CC) $(CFLAGS) -c pcf8575.c
fan_main_loop.o: fan_main_loop.c pcf8575.h tools.h 
	$(CC) $(CFLAGS) -c fan_main_loop.c
pcf-test.o: pcf-test.c pcf8575.h
	$(CC) $(CFLAGS) -c pcf-test.c
sql-select.o: sql-select.c
	$(CC) $(CFLAGS) -c sql-select.c

# remove object files and executable when user executes "make clean"
clean:
	rm main.o tools.o fan_main_loop.o fan-ctrl
	rm pcf8575.o
	rm pcf-test.o pcf-test
	rm sql-select.o sql-select-fan
