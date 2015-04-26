CFLAGS += -Wall

LIBNET_TEST = forge_ip_client forge_ip_server

TARGET = server client client_getchangeip_test $(LIBNET_TEST)

all:: $(TARGET)

server: server.o utils.o
	$(CC) $(CFLAGS) $^ -o $@ -lnet -lev

client: client.o
	$(CC) $(CFLAGS) $^ -o $@

client_getchangeip_test: client_getchangeip_test.o
	$(CC) $(CFLAGS) $^ -o $@ `pkg-config openssl --libs`

forge_ip_client: forge_ip_client.o
	$(CC) $(CFLAGS) $^ -o $@ -lnet

forge_ip_server: forge_ip_server.o utils.o
	$(CC) $(CFLAGS) $^ -o $@ -lev

clean::
	-rm -f $(TARGET) *.o
