AppName = MadIncEditor

SRCDIR = src
BINDIR = bin
mainFile = $(SRCDIR)/MadIncEditor.c
OutFile =$(BINDIR)/$(AppName) 

$(AppName): $(mainFile)
	$(CC) $(mainFile) -o $(OutFile) -Wall -Wextra -pedantic -std=c99


run: $(AppName)
	./$(OutFile) test.txt




# s sav

#this is a test of the capabliity of saveing 






















































# Page Scroll Test
