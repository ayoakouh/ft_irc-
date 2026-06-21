CPP = c++
NAME = a.out

FLAGS = -Wall -Wextra -Werror -std=c++98 -MMD
SRC = main.cpp Server.cpp Client.cpp Channel.cpp Join.cpp Invite.cpp Kick.cpp authentication.cpp Topic.cpp Privmsg.cpp Mode.cpp
OBJ = $(SRC:.cpp=.o)
DEPS = ${OBJ:.o=.d}

$(NAME): $(OBJ)
	$(CPP) $(FLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CPP) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(DEPS)

fclean: clean
	rm -f $(NAME)

re: fclean $(NAME)

-include $(DEPS)
