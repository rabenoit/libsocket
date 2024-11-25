NAME = libsocket.a

SRC_DIR = src/

SRCS = Socket.cpp

OBJS_DIR = objs/

OBJS = $(addprefix $(OBJS_DIR), $(SRCS:%.cpp=%.o))

CC = c++

CFLAGS = -Wall -Werror -Wextra

COMPRESS = ar -rcs

DEPENDENCIES_DIR = deps/

DEPENDENCIES = $(addprefix $(DEPENDENCIES_DIR), $(SRCS:%.cpp=%.d))

all: $(NAME)

$(OBJS_DIR)%.o: $(SRC_DIR)%.cpp | $(OBJS_DIR) $(DEPENDENCIES_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

$(DEPENDENCIES_DIR):
	mkdir -p $(DEPENDENCIES_DIR)

$(NAME): $(OBJS)
	$(COMPRESS) $(NAME) $(OBJS)

clean:
	rm -rf $(OBJS_DIR) $(DEPENDENCIES_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: clean fclean all

-include $(DEPENDENCIES)