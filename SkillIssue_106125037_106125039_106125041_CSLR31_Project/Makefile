# Makefile - run `make` in this directory.
#   make          -> builds ./a.out (console app) and ./server (HTTP API)
#   make test     -> builds and runs the test suite (Test/)
#   make clean    -> removes object files and binaries
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -Iinclude -MMD -MP
LDLIBS   :=
# Windows (MinGW / w64devkit): the HTTP server needs the Winsock libraries.
ifeq ($(OS),Windows_NT)
  CXXFLAGS += -D_WIN32_WINNT=0x0A00
  LDLIBS   += -lws2_32 -lwsock32
else
  LDLIBS   += -pthread
endif
OBJ      := object

CORE_OBJS := $(OBJ)/platform.o $(OBJ)/storage.o

all: a.out server

a.out: $(OBJ)/main.o $(CORE_OBJS)
	$(CXX) $^ -o $@

server: $(OBJ)/server.o $(CORE_OBJS)
	$(CXX) $^ -o $@ $(LDLIBS)

test_runner: $(OBJ)/test_skill_issue.o $(CORE_OBJS)
	$(CXX) $^ -o $@

test: test_runner
	./test_runner

$(OBJ)/%.o: Source/%.cpp | $(OBJ)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/test_skill_issue.o: Test/test_skill_issue.cpp | $(OBJ)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ):
	mkdir -p $(OBJ)

# Header dependencies (.d files written by -MMD): editing any .h file rebuilds
# every .cpp that includes it, so objects never go stale.
-include $(wildcard $(OBJ)/*.d)

clean:
	rm -f $(OBJ)/*.o $(OBJ)/*.d a.out server test_runner a.exe server.exe test_runner.exe

.PHONY: all test clean
