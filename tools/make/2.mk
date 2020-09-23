one = export blah="I was set!"; echo $$blah

define two
export blah=set
echo $$blah
endef

# One and two are different.

.PHONY: all
all: 
	@echo "This prints 'I was set'"
	@$(one)
	@echo "This does not print 'I was set' because each command runs in a seperate shell"
	@$(two)
