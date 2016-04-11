# part 1: variables

 PROJECT_PATHS      = $(shell find ./src -mindepth 0 -maxdepth 1 -type d)
 PROJECT_AS_SOURCES = $(shell find ./src -name *.c)
 PROJECT_C_SOURCES  = $(shell find ./src -name *.s)
 PROJECT_HEADERS    = $(shell find ./src -name *.h)
 PROJECT_AS_OBJECTS = $(addsuffix .o, $(basename ${PROJECT_AS_SOURCES}))
 PROJECT_C_OBJECTS  = $(addsuffix .o, $(basename ${PROJECT_C_SOURCES}))
 PROJECT_TARGETS    = image.elf image.bin

 #MLIBC_PATH       = $(shell find ./mlibc -mindepth 0 -maxdepth 1 -type d)
 MLIBC_SOURCES    = $(shell find ./mlibc -name *.c)
 MLIBC_HEADERS    = $(shell find ./mlibc/includes -name *.h)
 MLIBC_OBJECTS    = $(addsuffix .o, $(basename ${MLIBC_SOURCES}))
 MLIBC_TARGETS    = libc.a

 QEMU_PATH        = /usr
 QEMU_GDB         =        127.0.0.1:1234
 QEMU_UART        = stdio
#QEMU_UART       += telnet:127.0.0.1:1235,server
#QEMU_UART       += telnet:127.0.0.1:1236,server

 LINARO_PATH      = /usr/local/gcc-linaro-5.1-2015.08-x86_64_arm-eabi
 LINARO_PREFIX    = arm-eabi

 MLIBC_INCLUDES   = ./mlibc/includes

# part 2a: OS build commands
${PROJECT_AS_OBJECTS} : ${PROJECT_AS_SOURCES}
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-as  $(addprefix -I , ${PROJECT_PATHS} ${MLIBC_INCLUDES}/include) -mcpu=cortex-a8 -g -o ${@} ${<}

${PROJECT_C_OBJECTS}   : ${PROJECT_C_SOURCES}
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-gcc $(addprefix -I , ${PROJECT_PATHS} ${MLIBC_INCLUDES}/include) -mcpu=cortex-a8 -mabi=aapcs -ffreestanding -std=gnu99 -g -c -O -o ${@} ${<}

%.elf : ${PROJECT_AS_OBJECTS} ${PROJECT_C_OBJECTS}
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-ld  $(addprefix -L , ${LINARO_PATH}/arm-eabi/libc/usr/lib    ) -T ${*}.ld -o ${@} ${^} -lc -lgcc -nostdlib

%.bin : %.elf
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-objcopy -O binary ${<} ${@}

# part 2b: mlibc build commands
${MLIBC_OBJECTS} : ${MLIBC_SOURCES}
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-gcc $(addprefix -I , ./mlibc/includes) -mcpu=cortex-a8 -mabi=aapcs -ffreestanding -std=gnu99 -g -c -O -o ${@} ${<}

libc.a : ${MLIBC_OBJECTS}
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-ar rcs libc.a ${<}


# part 3: targets

.PRECIOUS   : ${PROJECT_OBJECTS} ${PROJECT_TARGETS}

build       : ${PROJECT_TARGETS}

build-mlibc : ${MLIBC_TARGETS}

launch-qemu : ${PROJECT_TARGETS}
	@${QEMU_PATH}/bin/qemu-system-arm -M realview-pb-a8 -m 128M -display none -gdb tcp:${QEMU_GDB} $(addprefix -serial , ${QEMU_UART}) -S -kernel $(filter %.bin, ${PROJECT_TARGETS})

launch-gdb  : ${PROJECT_TARGETS}
	@${LINARO_PATH}/bin/${LINARO_PREFIX}-gdb -ex "file $(filter %.elf, ${PROJECT_TARGETS})" -ex "target remote ${QEMU_GDB}"

kill-qemu   :
	@-killall --quiet --user ${USER} qemu-system-arm

kill-gdb    :
	@-killall --quiet --user ${USER} ${LINARO_PREFIX}-gdb

clean       : kill-qemu kill-gdb
	@rm -f core ${PROJECT_OBJECTS} ${PROJECT_TARGETS}

print-%  : ; @echo $* = $($*)
