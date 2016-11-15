GTK_INCLUDE = `pkg-config gtk+-2.0 --cflags`
GTK_LIB = `pkg-config gtk+-2.0 --libs`

FLAGS = -O2 -Wall -fPIC $(GTK_INCLUDE) 
LIBS = $(GTK_LIB) 
LFLAGS = -shared

CC = gcc $(CFLAGS) $(FLAGS)

OBJS = gkrellshoot.o

gkrellshoot.so: $(OBJS)
	$(CC) $(OBJS) -o gkrellshoot.so $(LFLAGS) $(LIBS) 

clean:
	rm -f *.o core *.so* *.bak *~

gkrellshoot.o: gkrellshoot.c

install:
	if [ -d /usr/local/lib/gkrellm2/plugins/ ] ; then \
		install -c -s -m 644 gkrellshoot.so /usr/local/lib/gkrellm2/plugins/ ; \
	elif [ -d /usr/lib/gkrellm2/plugins/ ] ; then \
        	install -c -s -m 644 gkrellshoot.so /usr/lib/gkrellm2/plugins/ ; \
	else \
		install -D -c -s -m 644 gkrellshoot.so /usr/lib/gkrellm2/plugins/gkrellshoot.so ; \
	fi
userinstall:
	if [ -d $(HOME)/.gkrellm2/plugins/ ] ; then \
		install -c -s -m 644 gkrellshoot.so $(HOME)/.gkrellm2/plugins/ ; \
	else \
		install -D -c -s -m 644 gkrellshoot.so $(HOME)/.gkrellm2/plugins/gkrellshoot.so ; \
	fi

uninstall:
	rm -f /usr/local/lib/gkrellm2/plugins/gkrellshoot.so
	rm -f /usr/lib/gkrellm2/plugins/gkrellshoot.so
	rm -f $(HOME)/.gkrellm2/plugins/gkrellshoot.so

