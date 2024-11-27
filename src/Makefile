# 컴파일러와 플래그 설정
CC = clang
CFLAGS = -Wall -g -Wno-sometimes-uninitialized

OUTPUT_DIR = ../output

INCLUDES = -I./

# 정적 라이브러리 설정
GenericLibrary = $(OUTPUT_DIR)/static_lib/Generic.a
GenericSources = Generic/ErrorCode/ErrorCode.c \
                 Generic/Logger/Logger.c \
                 Generic/Network/Network.c
GenericObjects = $(GenericSources:.c=.o)

# 동적 라이브러리 설정
ManagerLibrary = $(OUTPUT_DIR)/dynamic_lib/Manager.so
ManagerSources = Manager/ConfigManager.c \
				 Manager/Parser/INIParser.c
ManagerObjects = $(ManagerSources:.c=.o)

# 바이너리 설정
SuNyangiBinary = $(OUTPUT_DIR)/bin/SuNyangi
SuNyangiSources = Main.c
SuNyangiObjects = $(SuNyangiSources:.c=.o)

# 기본 타겟
all: $(OUTPUT_DIR) $(GenericLibrary) $(SuNyangiBinary)

# 출력 디렉터리 생성
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)/static_lib $(OUTPUT_DIR)/dynamic_lib $(OUTPUT_DIR)/bin

# 정적 라이브러리 생성
$(GenericLibrary): $(GenericObjects)
	ar rcs $(GenericLibrary) $(GenericObjects)

# 동적 라이브러리 생성
	$(CC) $(CFLAGS) $(INCLUDES) -shared -o $(ManagerLibrary) -fPIC $(ManagerSources) -L$(OUTPUT_DIR)/static_lib -l:Generic.a

# 바이너리 생성
$(SuNyangiBinary): $(GenericLibrary) $(SuNyangiObjects)
	$(CC) $(CFLAGS) $(INCLUDES) $(SuNyangiObjects) -L$(OUTPUT_DIR)/static_lib -l:Generic.a -o $(SuNyangiBinary)

# 오브젝트 파일 컴파일
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 클린업 타겟
clean:
	rm -f $(GenericObjects) $(GenericLibrary) $(SuNyangiObjects) $(SuNyangiBinary)

.PHONY: all clean
