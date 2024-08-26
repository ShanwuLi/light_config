# Makefile template for a simple C/C++ project

# Define the compiler and its flags
CC=gcc
CFLAGS=-Wall -g
MKDIR := mkdir -p
DIR_GUARD = @$(MKDIR) $(@D)

# Define the source files, object files, and the final executable name
SRCS= main.c light_config_parse.c logic_expr_parse.c \
      light_config_parse_sm.c


C_OBJS_TMP = $(patsubst %.c,%.o,$(SRCS))
OBJS := $(addprefix out/,$(C_OBJS_TMP))

EXEC=main

# Define the default target to build the executable
all: test/$(EXEC)

# Rule to compile the executable
test/$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to compile each .c file into an .o file
out/%.o: %.c
	$(DIR_GUARD)
	$(CC) $(CFLAGS) -c $< -o $@

# Target to clean up the object files and the executable
clean:
	rm -rf out test/main.exe

# Include dependency rules if they exist
-include $(OBJS:.o=.d)

# Automatic rule generation for dependencies
out/%.d: %.c
	$(CC) $(CFLAGS) -MM $< > $@


