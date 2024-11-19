SRC_DIR := src
POLICY_DIR := policies
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
POLICY_FILES := $(wildcard $(POLICY_DIR)/*.cpp)

# 실행 파일 이름 설정
OUTPUT := cacheSim

# 기본 컴파일러 플래그
CXX := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -O2

# 최종 실행 파일 생성
all: $(OUTPUT)

$(OUTPUT): $(SRC_FILES) $(POLICY_FILES)
	$(CXX) $(CXXFLAGS) $(SRC_FILES) $(POLICY_FILES) -o $(OUTPUT)

# clean 타겟
clean:
	rm -f $(OUTPUT)
