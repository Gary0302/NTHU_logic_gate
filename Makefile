CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g
SRCDIR = src
BINDIR = bin
TARGET = $(BINDIR)/sim

# Find all source files in SRCDIR
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(SRCDIR)/%.o,$(SRCS))

# Default target
all: init $(TARGET)

# Create needed directories if they don't exist
init:
	@mkdir -p $(BINDIR)

# Link all object files to create the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files to object files
$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compare output with golden file
compare:
	@echo "Auto comparing outputs with golden files..."
	@mkdir -p outputs
	@if [ ! -d "golden" ]; then \
		echo "Error: golden directory does not exist"; \
		exit 1; \
	fi
	@files=$$(ls golden/output*.out 2>/dev/null); \
	if [ -z "$$files" ]; then \
		echo "Warning: No output*.out files found in golden directory"; \
		exit 0; \
	fi
	@errors=0; \
	for f in golden/output*.out; do \
		if [ ! -f "$$f" ]; then continue; fi; \
		name=$$(basename "$$f"); \
		echo -n "Comparing $$name ... "; \
		if [ -f "outputs/$$name" ]; then \
			if diff -q "$$f" "outputs/$$name" > /dev/null; then \
				echo "Success! Match"; \
			else \
				echo "Fail! Differ"; \
				errors=$$((errors+1)); \
			fi \
		else \
			echo "Missing: outputs/$$name"; \
			errors=$$((errors+1)); \
		fi \
	done; \
	if [ $$errors -eq 0 ]; then \
		echo " All matched!"; \
	else \
		echo " Total mismatches or missing files: $$errors"; \
		exit 1; \
	fi


# Clean up generated files
clean:
	rm -rf $(SRCDIR)/*.o $(TARGET)

# Rebuild the project
rebuild: clean all


output:
	@for f in inputs/input*.in; do \
		name=$$(basename $$f .in); \
		num=$$(echo $$name | sed 's/[^0-9]*//g'); \
		echo "==> Running input$$num with pattern$$num"; \
		$(TARGET) ./inputs/input$$num.in ./patterns/pattern$$num.txt ./outputs/output$$num.out; \
	done
	@echo "All simulations completed to outputs/output*.out"

.PHONY: all init clean rebuild compare output