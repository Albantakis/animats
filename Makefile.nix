default: compile

# Compile everything into an executable called main.o in the build directory
compile:
	c++ temporalSpatialIntegrationLite/*.cpp -o build/main.o

# Remove the built object
clean:
	$(RM) build/main.o

run: compile
	./runall

test: compile
	./testrun
