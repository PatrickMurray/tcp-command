NAME = tcp
TARBALL = pmurray1-hw2.tar
SRC = tcp.c
OBJ = $(SRC:.c=.o)
RM = rm -f

all: $(OBJ)
	gcc -Wall $(OBJ) -o $(NAME)

clean:
	-$(RM) *~
	-$(RM) \#*
	-$(RM) *.o
	-$(RM) *.core

fclean:	clean
	-$(RM) $(NAME)

re: fclean all

submit: fclean
	tar cvzf $(TARBALL) README Makefile tcp.c
