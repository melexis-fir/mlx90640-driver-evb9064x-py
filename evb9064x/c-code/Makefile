.PHONY: clean all

ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

CC=gcc
AR=ar

DEP_LIBS:=
SHARED_LIB_NAME:=mlx90640_driver_evb9064x
STATIC_LIB_NAME:=mlx90640_driver_evb9064x_static

STATIC_LIB_FILE:=lib$(STATIC_LIB_NAME).a
SHARED_LIB_FILE:=lib$(SHARED_LIB_NAME).so

LIB_RESULT_DIR:=../libs

ifeq ($(detected_OS),Windows)
MACHINE:=windows
SHARED_LIB_FILE:=$(SHARED_LIB_NAME).dll
else
MACHINE:=$(shell uname -m)
endif

CFLAGS:=-Wall -Wextra -Wstrict-prototypes -Wunused-result -Wno-stringop-truncation -O3 -std=c99 -fmessage-length=0 -fPIC -I../libs -Iinc
CFLAGS+=$(CFLAGS_EXTRA)

all: $(SHARED_LIB_FILE) $(STATIC_LIB_FILE)

clean:
	rm -f main *.o
	rm -f src/*.o
	rm -f *.exe
	rm -f $(SHARED_LIB_FILE) $(STATIC_LIB_FILE)

main.o: main.c
	$(CC) -c -o $@ $(CFLAGS) $^

serial_debug.o: serial_debug.c
	$(CC) -c -o $@ $(CFLAGS) -Isrc $^

src/rs232.o: src/rs232.c
	$(CC) -c -o $@ $(CFLAGS) $^

src/evb9064x.o: src/evb9064x.c
	$(CC) -c -o $@ $(CFLAGS) $^

src/MLX90640_I2C_Driver_evb9064x.o: src/MLX90640_I2C_Driver_evb9064x.c
	$(CC) -c -o $@ $(CFLAGS) $^


main: main.o src/MLX90640_I2C_Driver_evb9064x.o src/rs232.o src/evb9064x.o
	$(CC) -o $@ $^ -lm -Wl,-rpath=$(LIB_RESULT_DIR)/$(MACHINE) -L$(LIB_RESULT_DIR)/$(MACHINE) $(DEP_LIBS)

serial_debug: serial_debug.o src/rs232.o
	$(CC) -o $@ $^


main_shared: main.o
	$(CC) -o $@ $^ -lm -lmlx90640_driver_evb9064x -Wl,-rpath=$(LIB_RESULT_DIR)/$(MACHINE) -L$(LIB_RESULT_DIR)/$(MACHINE) $(DEP_LIBS)



$(SHARED_LIB_FILE): src/MLX90640_I2C_Driver_evb9064x.o src/rs232.o src/evb9064x.o
	$(CC) -shared -o $@ $^ -L$(LIB_RESULT_DIR)/$(MACHINE) $(DEP_LIBS) -Wl,-rpath=$(LIB_RESULT_DIR)/$(MACHINE)
	@mkdir -p $(LIB_RESULT_DIR)/$(MACHINE)
	@cp -fv $@ $(LIB_RESULT_DIR)/$(MACHINE)

$(STATIC_LIB_FILE): src/MLX90640_I2C_Driver_evb9064x.o src/rs232.o src/evb9064x.o
	$(AR) rcu $@ $<
	@mkdir -p $(LIB_RESULT_DIR)/$(MACHINE)
	@cp -fv $@ $(LIB_RESULT_DIR)/$(MACHINE)
