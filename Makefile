program1 := master

src := $(shell find . -name "master*.cpp")
ob1  := $(patsubst %.cpp, %.o, $(src))

all: $(program1)

$(program1): $(ob1)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(program1) $(ob1) $(LDLIBS)

program2 := bin_adder

src := $(shell find . -name "bin_adder*.cpp")
ob2  := $(patsubst %.cpp, %.o, $(src))

all: $(program2)

$(program2): $(ob2)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(program2) $(ob2) $(LDLIBS)

clean:
		find . -type f ! -iname "*.cpp" ! -iname "*.h" ! -iname "makefile" ! -iname "README" ! -iname "adder_log" ! -iname "output.log" -delete

