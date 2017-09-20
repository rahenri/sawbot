BINARY=sawbot

all: $(BINARY)

CXXFLAGS=-Wall --std=c++1y -Werror -ggdb3 -D_LOCAL -march=native -O3
LDLIBS=-lpthread

OBJECTS = field.o utils.o random.o cmd_args.o flags.o minimax.o hash_table.o interruption.o minimax_worker.o
MAIN_OBJECT = main.o



TESTS =

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.cpp = g++ $(DEPFLAGS) $(CXXFLAGS) -c
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

%.o : %.cpp
%.o : %.cpp $(DEPDIR)/%.d
	@echo "Compiling $<..."
	@$(COMPILE.cpp) $(OUTPUT_OPTION) $<
	@$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(OBJECTS)))
-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(MAIN_OBJECT)))
-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(board_test.o)))

interruption.o: interruption.h interruption.cpp

line_reader.o: line_reader.h line_reader.cpp

$(BINARY): $(OBJECTS) $(MAIN_OBJECT)
	@echo "Linking $@..."
	@g++ $^ $(LDLIBS) $(CXXFLAGS) -o $@

board_test: board_test.o $(OBJECTS)
	@echo "Linking $@..."
	@g++ $^ $(LDLIBS) $(CXXFLAGS) -o $@
	@echo "Running $@..."
	@./board_test
	@rm board_test

tests: board_test

clean:
	rm -f *.o $(BINARY)

