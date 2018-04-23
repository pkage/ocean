COMPILER = clang
OBJ_DIR = obj
SRC_DIR = src
LIB_DIR = lib
BIN_DIR = bin

all: clean link

run:
	bin/ocean plugin/dist

pull-web:
	cp ~/Documents/XCODE/ocean-web/ocean-web/ocean.c ${SRC_DIR}/web.c
	cp ~/Documents/XCODE/ocean-web/ocean-web/ocean.h ${SRC_DIR}/web.h

clean:
	mkdir -p ${OBJ_DIR} ${BIN_DIR}
	rm -vf ${OBJ_DIR}/*
	rm -f  ${BIN_DIR}/ocean

build-plugin:
	cd plugin && make

wv.c:
	${COMPILER} \
		-x objective-c 			\
		-c ${SRC_DIR}/wv.c		\
		-I${SRC_DIR}/ 			\
		-I${LIB_DIR}/ 			\
		-DWEBVIEW_COCOA=1
	mv wv.o ${OBJ_DIR}

cfg.c:
	${COMPILER} -c src/cfg.c
	mv cfg.o ${OBJ_DIR}

web.c:
	${COMPILER} -c src/web.c
	mv web.o ${OBJ_DIR}

plugin.c:
	${COMPILER} -c src/plugin.c
	mv plugin.o ${OBJ_DIR}

main.c: wv.c
	${COMPILER} -c ${SRC_DIR}/main.c	
	mv main.o ${OBJ_DIR}
	
# check if Foundation link necessary
link: wv.c main.c cfg.c plugin.c
	${COMPILER} ${OBJ_DIR}/* 	\
		-ldl					\
		-framework Foundation 	\
		-framework Cocoa        \
		-framework WebKit       \
		-o ${BIN_DIR}/ocean

