CC = mpicc
CFLAGS = -g -Wall -Wextra
LFLAGS =

# File names	
EXEC = mpi
SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJECTS = ${SOURCES:%.c=obj/%.o}

default: $(EXEC)

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJECTS:obj/%.o=dep/%.d)
endif

dep/%.d: %.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -M $< | \
		sed 's,[a-zA-Z0-9_\.]*.o:,$(<:%.c=obj/%.o):,' > $@

obj/%.o: %.c dep/%.d $(HEADERS)
	@mkdir -p $(dir $@)
	mpicc $(CFLAGS) -c $< -o $@

$(EXEC): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

clean:
	rm -rf obj dep mpi