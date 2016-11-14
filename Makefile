GTK_INCLUDE = `pkg-config gtk+-2.0 --cflags`
GTK_LIB = `pkg-config gtk+-2.0 --libs`

FLAGS = -O2 -Wall $(GTK_INCLUDE) 
LIBS = $(GTK_LIB)
CFLAGS += -fPIC
LDFLAGS += -shared

CC = gcc $(FLAGS)

OBJS = gkrellshoot.o

gkrellshoot.so: $(OBJS)
	$(CC) $(OBJS) -o gkrellshoot.so $(LDFLAGS) $(LIBS) 

clean:
	rm -f *.o core *.so* *.bak *~

gkrellshoot.o: gkrellshoot.c

install:
	install -D -p -s -m 644 gkrellshoot.so $(DESTDIR)/usr/lib/gkrellm2/plugins/gkrellshoot.so

uninstall:
	rm -f $(DESTDIR)/usr/lib/gkrellm2/plugins/gkrellshoot.so
