
#Makefile for 'en' lang.

BUILDER=msgfmt
OBJS=zeus.po
BIN=zeus.mo
DIR=/home/undernode/projects/zeus/lang/en/LC_MESSAGES
INPUT=$(DIR)/$(OBJS)
OUTPUT=$(DIR)/$(BIN)

.PHONY: all clean


all: $(BIN)

$(BIN):
		@echo $(DIR)
		@echo "Building $@ from $(OBJS), please wait..."
		@$(BUILDER) $(INPUT) -o $(OUTPUT)

clean:
		cd $(DIR); \
		rm *.mo; \
		cd ..;

