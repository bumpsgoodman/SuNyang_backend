# 컴파일러와 플래그 설정
CC = clang
CFLAGS = -Wall -g -Wno-sometimes-uninitialized

BIN_DIR = ../bin
LIB_DIR = ../lib

INCLUDES = -I../include

# 정적 라이브러리 설정
GenericLibrary = $(LIB_DIR)/Generic.a
GenericSources = Generic/ErrorCode/ErrorCode.c \
                 Generic/Logger/Logger.c \
				 Generic/Manager/ConfigManager.c \
				 Generic/Manager/Parser/INIParser.c \
				 Generic/MemPool/StaticMemPool.c \
                 Generic/Network/Network.c
GenericObjects = $(GenericSources:.c=.o)

# 동적 라이브러리 설정
# ManagerLibrary = $(OUTPUT_DIR)/dynamic_lib/Manager.so
# ManagerSources = Manager/ConfigManager.c \
# 				 Manager/Parser/INIParser.c
# ManagerObjects = $(ManagerSources:.c=.o)

# 바이너리 설정
SuNyangiBinary = $(BIN_DIR)/SuNyangi
SuNyangiSources = Main.c
SuNyangiObjects = $(SuNyangiSources:.c=.o)

# 기본 타겟
all: $(BIN_DIR) $(LIB_DIR) $(GenericLibrary) $(SuNyangiBinary)

# 출력 디렉터리 생성
$(BIN_DIR):
	mkdir -p $(BIN_DIR)
$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# 정적 라이브러리 생성
$(GenericLibrary): $(GenericObjects)
	ar rcs $(GenericLibrary) $(GenericObjects)

# 정적 라이브러리 생성
$(ManagerLibrary): $(ManagerObjects)
	ar rcs $(ManagerLibrary) $(ManagerObjects)

# 동적 라이브러리 생성
#	$(CC) $(CFLAGS) $(INCLUDES) -shared -o $(ManagerLibrary) -fPIC $(ManagerSources) -L$(OUTPUT_DIR)/static_lib -l:Generic.a

# 바이너리 생성
$(SuNyangiBinary): $(GenericLibrary) $(SuNyangiObjects)
	$(CC) $(CFLAGS) $(INCLUDES) $(SuNyangiObjects) -L$(LIB_DIR) -l:Generic.a -o $(SuNyangiBinary)

# 오브젝트 파일 컴파일
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 클린업 타겟
clean:
	rm -f $(GenericObjects) $(GenericLibrary) $(SuNyangiObjects) $(SuNyangiBinary)

.PHONY: all clean
