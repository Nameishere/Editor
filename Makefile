AppName = mie

InstallLocation = /usr/local/bin/

SRCDIR = src
BINDIR = bin
mainFile = $(SRCDIR)/MadIncEditor.c
OutFile =$(BINDIR)/$(AppName) 
exec = chmod

TestFolder = tests
Testfile = $(TestFolder)/test.c


$(AppName): $(mainFile)
	$(CC) $(mainFile) -o $(OutFile) -Wall -Wextra -pedantic -std=c99


test: $(AppName)
	./$(OutFile) $(Testfile)

run: $(AppName)
	./$(OutFile) $(Testfile)


install: $(OutFile)
	$(exec) +x $(OutFile)
	sudo cp $(OutFile) $(InstallLocation)


