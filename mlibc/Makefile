# part 1: variables

MLIBC_PATHS      = $(shell find ./src -mindepth 0 -maxdepth 1 -type d)
MLIBC_SOURCES    = $(shell find ./src -name *.c)
MLIBC_HEADERS    = $(shell find ./src -name *.h)
MLIBC_PROCESSED  = $(addsuffix .i, $(basename ${MLIBC_SOURCES}))
MLIBC_OBJECTS    = $(addsuffix .o, $(basename ${MLIBC_SOURCES}))
MLIBC_TARGETS    = libc.a

LINARO_PATH      = /usr/local/gcc-linaro-5.1-2015.08-x86_64_arm-eabi
LINARO_PREFIX    = arm-eabi

MLIBC_INCLUDES   = ./include

# part 2: build commands
%.o : %.c
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-gcc $(addprefix -I , ${MLIBC_INCLUDES}) -mcpu=cortex-a8 -mabi=aapcs -ffreestanding -std=gnu99 -nostdinc -g -c -O -o ${@} ${<}

libc.a : ${MLIBC_OBJECTS}
	@mkdir -p build
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-ar rcs build/libc.a ${^}

%.i : %.c
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-gcc $(addprefix -I , ${MLIBC_INCLUDES}) -mcpu=cortex-a8 -mabi=aapcs -ffreestanding -std=gnu99 -nostdinc -E -g -c -O -o ${@} ${<}

# part 3: targets

.PRECIOUS   : ${MLIBC_OBJECTS} ${MLIBC_TARGETS}

build       : ${MLIBC_TARGETS}

process     : ${MLIBC_PROCESSED}

clean :
	@rm -f core ${MLIBC_OBJECTS} ${MLIBC_PROCESSED} build/libc.a

print-%  : ; @echo $* = $($*)
