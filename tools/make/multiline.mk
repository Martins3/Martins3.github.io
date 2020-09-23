SRC = src/a.c

define foo

endef

C = can you find me



# $(info $(foo))


all:
	$(call foo)
	@echo the shell dislikes multiline variables
