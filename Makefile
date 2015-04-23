CFLAGS += -Wall

TARGET = server client

all:: $(TARGET)

server: server.o utils.o
	$(CC) $(CFLAGS) $^ -o $@ -lnet -lev

client: client.o
	$(CC) $(CFLAGS) $^ -o $@

clean::
	-rm -f $(TARGET) *.o
