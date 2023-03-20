PARSER=parser
SEARCH=search
cc=g++

.PHONY:all
all:$(PARSER) $(SEARCH)

$(PARSER):parser.cc
	$(cc) -o $@ $^ -std=c++11 -lboost_system -lboost_filesystem

$(SEARCH):search.cc
	$(cc) -o $@ $^ -std=c++11 -ljsoncpp -lpthread

.PHONY:clean
clean:
	rm -f $(PARSER) $(SEARCH)