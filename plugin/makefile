COMPILER = clang
PLUGIN_OUT = example.so
PLUGIN_SRC = example.c
SDK_FILE = ../src/web.c

all: dist/example.so assets

clean:
	mkdir -p dist/
	rm -rf dist/*

assets: clean
	cp manifest.cfg dist/manifest.cfg
	cp -r static dist/static

dist/example.so: clean
	${COMPILER} -o dist/${PLUGIN_OUT} ${PLUGIN_SRC} ${SDK_FILE} -shared -fPIC
