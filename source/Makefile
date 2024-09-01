# Makefile template for a simple C/C++ project

# Define the compiler and its flags
CC=gcc
CFLAGS=-Wall -g
MKDIR := mkdir -p
DIR_GUARD = @$(MKDIR) $(@D)

# Define the source files, object files, and the final executable name
SRCS= light_config.c light_config_parse.c logic_expr_parse.c \
      light_config_parse_sm.c light_config_parse_func.c \
      light_config_output.c


C_OBJS_TMP = $(patsubst %.c,%.o,$(SRCS))
OBJS := $(addprefix out/,$(C_OBJS_TMP))

EXEC=light_config

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
# Automatic rule generation for dependencies
out/%.d: %.c
	$(CC) $(CFLAGS) -MM $< > $@
