# MakeFile for onemax 'learning empirical project'

# Global flags
CFLAGS_all := -Wall -Wno-unused-variable -Wno-unused-function -std=c++14 -I../../Empirical/ -I./

# Compiler
CXX_web := emcc
CXX_native := g++

# Other flags
OFLAGS_native := -g -pedantic
OFLAGS_web := -DNDEBUG -s TOTAL_MEMORY=67108864 -s ASSERTIONS=2

# Bringing flag options together
CFLAGS_native := $(CFLAGS_all)
CFLAGS_web := $(CFLAGS_all) $(OFLAGS_web) --js-library ../../Empirical/web/library_emp.js --js-library ../../d3-emscripten/library_d3.js -s EXPORTED_FUNCTIONS="['_main', '_empCppCallback']" -s NO_EXIT_RUNTIME=1 -s DEMANGLE_SUPPORT=1 --preload-file StatsConfig.cfg
# If I want to load config settings: --preload-file evo-in-physics-pt1.cfg

JS_TARGETS := EventDrivenGP-Roles-LSVis.js

default: web

web: $(JS_TARGETS)

EventDrivenGP-Roles-LSVis.js: EventDrivenGP-Roles-LSVis.cc
	mkdir -p web
	$(CXX_web) $(CFLAGS_web) EventDrivenGP-Roles-LSVis.cc -o web/EventDrivenGP-Roles-LSVis.js

# emcc -Wall -Wno-unused-variable -Wno-unused-function -std=c++11 --js-library ../../Empirical/emtools/library_emp.js --js-library ../../d3-emscripten/library_d3.js -s EXPORTED_FUNCTIONS="['_main', '_empCppCallback']" -s NO_EXIT_RUNTIME=1 -s DEMANGLE_SUPPORT=1 onemax_web.cc -o web/onemax.js
