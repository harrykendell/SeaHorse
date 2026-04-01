STD = -std=c++20
# CXX = g++

W_FLAGS = -Wall -Wextra
W_FLAGS += -Wno-pragmas

USE_MKL = -lmkl_intel_ilp64 -lmkl_intel_thread -lmkl_core -liomp5 -lpthread -lm -ldl -m64 -DEIGEN_USE_MKL_ALL -DMKL_ILP64

FLAGS = -ffp-contract=fast -mavx2 -mfma -march=native

INCLUDE_PATHS = -I./
INCLUDE_PATHS += -I../
INCLUDE_PATHS += -I/usr/include/mkl

LIBRAYLIB = -L./libs/raylib/src -lraylib

ifeq ($(shell uname), Darwin)
GUI = -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL -lglfw
INCLUDE_PATHS += -I/opt/intel/oneapi/mkl/latest/include/
USE_MKL += -L/usr/local/lib -Wl,-rpath,/usr/local/lib -Wno-unused-command-line-argument
FLAGS += -O3
CXX = /usr/local/bin/g++-12
else
GUI = -lGL -lX11
FLAGS += -Ofast -fopenmp -lmkl_sequential
endif

RED=\033[1;91m
GREEN=\033[1;92m
ORANGE=\033[1;93m
WHITE=\033[1;0m

SEAHORSE_SRCS := $(shell find src -name "*.cpp")
SEAHORSE_OBJ_DIR := build/obj
SEAHORSE_OBJS := $(patsubst src/%.cpp,$(SEAHORSE_OBJ_DIR)/%.o,$(SEAHORSE_SRCS))
SEAHORSE_LIB := build/libseahorse.a

$(shell mkdir -p bin build $(SEAHORSE_OBJ_DIR))

$(SEAHORSE_OBJS): | $(dir $(SEAHORSE_OBJS))

$(sort $(dir $(SEAHORSE_OBJS))):
	@mkdir -p $@

$(SEAHORSE_OBJ_DIR)/%.o: src/%.cpp Makefile
	@printf "${GREEN}[BUILDING]${WHITE} $@\n"
	@mkdir -p $(dir $@)
	@$(CXX) -c $(STD) $(INCLUDE_PATHS) $(W_FLAGS) $(FLAGS) $(USE_MKL) $< -o $@

$(SEAHORSE_LIB): $(SEAHORSE_OBJS) Makefile
	@mkdir -p $(dir $@)
	@ar rcs $@ $(SEAHORSE_OBJS)
	@printf "${GREEN}[BUILDING] Completed legacy seahorse library${WHITE}\n"

project_files = $(notdir $(basename $(wildcard projects/*.cpp)))
all: $(project_files)

$(project_files): %: projects/%.cpp $(SEAHORSE_LIB)
	@mkdir -p bin
	@printf "${GREEN}[BUILDING]${WHITE} projects/$@\n"
	@$(CXX) $< $(STD) $(INCLUDE_PATHS) $(W_FLAGS) $(FLAGS) $(USE_MKL) $(SEAHORSE_LIB) -o ./bin/$(notdir $@)
	@printf "${GREEN}[BUILDING] Completed $@${WHITE}\n"

.PHONY: rebuild_lib
rebuild_lib:
	@rm -f $(SEAHORSE_OBJS) $(SEAHORSE_LIB)
	@$(MAKE) $(SEAHORSE_LIB)

gui: bin/gui
bin/gui: gui/gui.cpp $(SEAHORSE_LIB) Makefile ./libs/raylib/src/libraylib.a
	@mkdir -p bin
	@printf "${GREEN}[BUILDING]${WHITE} Graphical Version\n"
	@$(CXX) $< $(STD) $(INCLUDE_PATHS) $(W_FLAGS) $(FLAGS) $(USE_MKL) $(SEAHORSE_LIB) $(GUI) $(LIBRAYLIB) -o ./bin/gui
	@printf "${GREEN}[BUILDING] Completed gui${WHITE}\n"
	$(RUN) ./bin/gui $(ARGS)

./libs/raylib/src/libraylib.a:
	@printf "${GREEN}[BUILDING]${WHITE} Lib Raylib\n"
	@(cd ./libs/raylib/src && $(MAKE) PLATFORM=PLATFORM_DESKTOP RAYLIB_MODULE_RAYGUI=TRUE CUSTOM_CFLAGS="-Wno-unused-function -Wno-unused-but-set-variable -Wno-null-character")

.PHONY: clean
clean:
	@printf "${GREEN}[CLEANING]${WHITE} Cleaning legacy build outputs\n"
	@- rm -rf build bin
