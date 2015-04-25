CFLAGS += -Wall

TARGET = server client client_getchangeip_test

all:: $(TARGET)

server: server.o utils.o
	$(CC) $(CFLAGS) $^ -o $@ -lnet -lev

client: client.o
	$(CC) $(CFLAGS) $^ -o $@

client_getchangeip_test: client_getchangeip_test.o
	$(CC) $(CFLAGS) $^ -o $@ `pkg-config openssl --libs`

clean::
	-rm -f $(TARGET) *.o
