AppName = mie
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
LDFLAGS = 
exec = chmod
InstallLocation = /usr/local/bin/



SRCDIR = src
BUILDDIR = build
BINDIR = bin
TestFolder = tests


SRCS = $(wildcard $(SRCDIR)/*.c)
BUILDS = $(patsubst $(SRCSDIR)/%.c, $(BUILDDIR)/%.o, $(SRCS))
Testfile = $(TestFolder)/test.c
OutFile = $(BINDIR)/$(AppName) 




all: $(BINDIR) $(BUILDS)
	@echo "Linking..."
	$(CC) $(BUILDS) -o $(OutFile) $(LDFLAGS)
	@echo "Build Complete"

$(BINDIR):
	mkdir -p $(BINDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	@echo "Compiling $<"
	$(CC) $(CFLAGS)	-c $< -o $@

test: $(all)
	./$(OutFile) $(Testfile)

run: $(all)
	./$(OutFile)


install: $(OutFile)
	$(exec) +x $(OutFile)
	sudo cp $(OutFile) $(InstallLocation)


.PHONY: clean

clean:
	@echo "Cleaning up..."
	rm -r $(BUILDDIR) $(BINDIR)

