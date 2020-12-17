CXX      := icpc
CXXFLAGS := -Wall -O3
LDFLAGS  := 
TARGET   := main.exe
CPPVERSION := -std=c++17
SRC1  :=dualfunctions.cpp
SRC2  :=parse_tree.cpp
Obj1  :=dualfunctions.o
Obj2  :=parse_tree.o
   


all:
	$(CXX) $(CPPVERSION) $(CXXFLAGS) $(LDFLAGS) -c $(SRC1)
	$(CXX) $(CPPVERSION) $(CXXFLAGS) $(LDFLAGS) -c $(SRC2)
	$(CXX) $(CPPVERSION) $(CXXFLAGS) $(LDFLAGS)  $(Obj1) $(Obj2) -o $(TARGET)

	./main.exe
clean:
	rm -rf $(Obj1) $(Obj2) $(TARGET)
	
