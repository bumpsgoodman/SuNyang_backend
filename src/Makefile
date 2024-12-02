# 컴파일러와 플래그 설정
CC = clang
CFLAGS = -Wall -g -Wno-sometimes-uninitialized -DUNICODE -D_UNICODE

BIN_DIR = ../bin
LIB_DIR = ../lib

INCLUDES = -I../include

# Generic 라이브러리 설정
GenericLibrary = $(LIB_DIR)/Generic.a
GenericSources = Generic/Container/FixedArray.c \
				 Generic/Container/DynamicArray.c \
				 Generic/ErrorCode/ErrorCode.c \
                 Generic/Logger/Logger.c \
                 Generic/Manager/ConfigManager.c \
                 Generic/Manager/Parser/INIParser.c \
                 Generic/MemPool/StaticMemPool.c \
                 Generic/Network/Network.c
GenericObjects = $(GenericSources:.c=.o)

# Blog 라이브러리 설정
BlogLibrary = $(LIB_DIR)/Blog.so
BlogSources = Blog/Blog.c
BlogObjects = $(BlogSources:.c=.o)

# 바이너리 설정
SuNyangiBinary = $(BIN_DIR)/SuNyangi
SuNyangiSources = Main.c HttpRedirector.c
SuNyangiObjects = $(SuNyangiSources:.c=.o)

# 기본 타겟
all: $(BIN_DIR) $(LIB_DIR) $(GenericLibrary) $(BlogLibrary) $(SuNyangiBinary)

# 출력 디렉터리 생성
$(BIN_DIR):
	mkdir -p $(BIN_DIR)
$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# Generic 라이브러리 생성
$(GenericLibrary): $(GenericObjects)
	ar rcs $(GenericLibrary) $(GenericObjects)

# Blog 라이브러리 생성
$(BlogLibrary): $(GenericLibrary) $(BlogObjects)
	$(CC) $(CFLAGS) $(INCLUDES) -shared -o $(BlogLibrary) -fPIC $(BlogSources) -L$(LIB_DIR) -l:Generic.a

# SuNyangi 실행 파일 생성
$(SuNyangiBinary): $(GenericLibrary) $(SuNyangiObjects)
	$(CC) $(CFLAGS) $(INCLUDES) $(SuNyangiObjects) -L$(LIB_DIR) -l:Generic.a -o $(SuNyangiBinary)

# 오브젝트 파일 컴파일
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 클린업 타겟
clean:
	rm -f $(GenericObjects) $(GenericLibrary) $(BlogObjects) $(BlogLibrary) $(SuNyangiObjects) $(SuNyangiBinary)

.PHONY: all clean
