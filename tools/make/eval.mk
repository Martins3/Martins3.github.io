###############################################
pointer := pointed_value


define foo 
var := 123
arg := $1
$$($1) := ooooo
SRC += $1
endef 

$(info $(call foo,pointer))
$(eval $(call foo,pointer))

ERR = $(error GG we found an error!)
.PHONY: err all

all:
	@echo -----------------------------
	@echo var: $(var), arg: $(arg)
	@echo pointer: $(pointer), pointed_value: $(pointed_value) SRC : $(SRC)
	@echo done.
	@echo -----------------------------

err: ; $(ERR)

###############################################
# PROGRAMS    = server client
#
# server_OBJS = server.o server_priv.o server_access.o
# server_LIBS = priv protocol
#
# client_OBJS = client.o client_api.o client_mem.o
# client_LIBS = protocol
#
# .PHONY: all
# all: $(PROGRAMS)
#
# define PROGRAM_template =
 # $(1): $$($(1)_OBJS) $$($(1)_LIBS:%=-l%)
 # ALL_OBJS   += $$($(1)_OBJS)
# endef
#
# $(foreach prog,$(PROGRAMS),$(eval $(call PROGRAM_template,$(prog))))
#
# $(PROGRAMS):
	# $(LINK.o) $^ $(LDLIBS) -o $@
#
# clean:
	# rm -f $(ALL_OBJS) $(PROGRAMS)
