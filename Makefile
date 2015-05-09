CFLAGS += -Wall

LIBNET_TEST = forge_ip_client forge_ip_server

TARGET = server $(LIBNET_TEST)

all:: $(TARGET)

server: server.o utils.o cripple_log.o
	$(CC) $(CFLAGS) $^ -o $@ -lnet -lev

forge_ip_client: forge_ip_client.o utils.o
	$(CC) $(CFLAGS) $^ -o $@ -lnet

forge_ip_server: forge_ip_server.o utils.o
	$(CC) $(CFLAGS) $^ -o $@ -lev

clean::
	-rm -f $(TARGET) *.o
