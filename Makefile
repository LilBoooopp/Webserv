NAME = webserv
CC = c++
RM = rm -rf
INC = include
FLAGS = -Werror -Wextra -Wall -std=c++98 -g -I$(INC)
MAKE := make --no-print-directory

#--------------------------------------SOURCES---------------------------------#
SRC_DIR = src/
SRC_SRC =  \
	main.cpp
SRC = $(addprefix $(SRC_DIR), $(SRC_SRC))

CGI_DIR = src/cgi/
CGI_SRC = \
	Execute.cpp \
	ParseRequest.cpp \
	Respond.cpp
CGI = $(addprefix $(CGI_DIR), $(CGI_SRC))

CORE_DIR = src/core/
CORE_SRC =  \
	EpollReactor.cpp
CORE = $(addprefix $(CORE_DIR), $(CORE_SRC))

HTTP_DIR = src/http/
HTTP_SRC =  \
	ChunkedDecoder.cpp \
	HttpParser.cpp \
	HttpResponse.cpp
HTTP = $(addprefix $(HTTP_DIR), $(HTTP_SRC))

SERVER_DIR = src/server/
SERVER_SRC =  \
	Listener.cpp \
	Server.cpp \
	StaticHandler.cpp
SERVER = $(addprefix $(SERVER_DIR), $(SERVER_SRC))

UTILS_DIR = src/utils/
UTILS_SRC =  \
	Logger.cpp \
	Chrono.cpp
UTILS = $(addprefix $(UTILS_DIR), $(UTILS_SRC))

CONFIG_DIR = src/config/
CONFIG_SRC = \
	Config.cpp \
	Config_Debug.cpp \
	Config_Error.cpp \
	Config_Helpers.cpp \
	Config_Validation.cpp
CONFIG = $(addprefix $(CONFIG_DIR), $(CONFIG_SRC))

ALL_SRC = $(SRC) $(CGI) $(CORE) $(HTTP) $(SERVER) $(UTILS) $(CONFIG)
vpath %.cpp src src/core src/http src/server src/utils src/cgi src/config
#--------------------------------------OBJECTS----------------------------------#
OBJ_DIR = Objects/
# Generate object files with same names but in Objects directory
OBJECTS = $(patsubst %.cpp,$(OBJ_DIR)%.o,$(notdir $(ALL_SRC)))

# This is a crucial part - create variables for each source directory to make vpath work properly
VPATH = $(SRC_DIR)

#--------------------------------------ANIMATION CONFIG------------------------#
ANIMATION_FRAMES = ⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏
ANIMATION_COLOR = '\033[1;36m'
TOTAL_FILES := $(words $(OBJECTS))

#--------------------------------------COLORS-----------------------------------#
NONE='\033[0m'
GREEN='\033[32m'
YELLOW='\033[33m'
GRAY='\033[2;37m'
CURSIVE='\033[3m'
BLUE='\033[34m'
MAGENTA='\033[35m'
CYAN='\033[36m'
WHITE='\033[37m'
BOLD='\033[1m'

#--------------------------------------RULES-----------------------------------#
all: reset_counter $(OBJ_DIR) $(NAME)

reset_counter:
	@rm -f .counter
	@echo "0" > .counter

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@echo $(BOLD)$(MAGENTA)"Objects directory created"$(NONE)


$(NAME): $(OBJECTS)
	@$(CC) $(FLAGS) $(OBJECTS) -o $(NAME)
	@printf "\n"
	@echo "\033[1;32m\n✅ $(NAME) successfully compiled!\n\033[0m"
	@rm .counter

debug: $(OBJECTS)
	@$(CC) $(FLAGS) -fsanitize=address -g $(OBJECTS) -o $(NAME)
	@printf "\n"
	@echo "\033[1;32m\n✅ $(NAME) successfully compiled! <DEBUG>\n\033[0m"

#--------------------------------------COMPILATION RULE------------------------#
$(OBJ_DIR)%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	@compiled=$$(ls -1 $(OBJ_DIR)/*.o 2>/dev/null | wc -l); \
	total=$(TOTAL_FILES); \
	compiled=$$((compiled + 1)); \
	frames="⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏"; \
	frame_index=$$((compiled % 10)); \
	frame=$$(echo $$frames | cut -d ' ' -f $$((frame_index + 1))); \
	barlen=30; \
	percent=$$((100 * compiled / total)); \
	done=$$((barlen * percent / 100)); \
	todo=$$((barlen - done)); \
	bar=$$(printf "█%.0s" $$(seq 1 $$done)); \
	space=$$(printf "░%.0s" $$(seq 1 $$todo)); \
	printf "\r\033[2K\033[1;36m%s \033[1mCompiling\033[0m [%-*s] %3d%% \033[36m%-40.40s\033[0m" \
	"$$frame" "$$barlen" "$$bar$$space" "$$percent" "$<"; \
	$(CC) $(FLAGS) -c $< -o $@

clean:
	@echo $(CURSIVE)$(GRAY) " -> Cleaning object files.." $(NONE)
	@$(RM) $(OBJ_DIR)
	@$(RM) .counter

fclean: clean
	@echo $(CURSIVE)$(GRAY) " -> Removing $(NAME)" $(NONE)
	@$(RM) $(NAME)

re: fclean all

bonus:
	@echo $(CURSIVE)$(GRAY) " - Compiling bonus $(NAME)..." $(NONE)
	@echo $(GREEN)"- Compiled -"$(NONE)

.PHONY: all clean fclean re bonus reset_counter
