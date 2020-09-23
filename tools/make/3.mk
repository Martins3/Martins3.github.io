OBJPATH=obj
define func
tmp=$(OBJPATH)/$(strip $1)
objs+=$$(tmp)
$$(tmp):$2
	gcc $$^ -o $$@
	echo "fuck this !"
endef

gg=

all :m
	echo $(gg)



gg+=a

define innner

endef

define mul_para
m:
	echo $1
	echo $2
	echo $3
endef

# x = $(call mul_para,a,$(1),$(2))
# y = $(call x,b,$(1))
# $(call y,c)

$(eval $(call mul_para,a,b,c))


$(info $(mul))

# define mulno
# mulno:
	# echo wtfwtf
# endef
#
# $(info $(call func,foo,foo.c))
# $(eval $(call func,foo,foo.c))
#
#
#
# $(info $(call $(eval $(call mul,1)),mmmmm))
# $(info $(call mul,1))
#
# $(eval $(mulno))

# $(info $(call $(eval $(call mul,1)),mmmmm))
