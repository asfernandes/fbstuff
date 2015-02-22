MODULES	:= ods perf perf/udr test test/v3api

TARGET	:= release

CXX	:= g++
LD	:= $(CXX)

SRC_DIR		:= src
BUILD_DIR	:= build
OUT_DIR		:= output
SHRLIB_EXT	:= so

OBJ_DIR := $(BUILD_DIR)/$(TARGET)
BIN_DIR := $(OUT_DIR)/$(TARGET)/bin
LIB_DIR := $(OUT_DIR)/$(TARGET)/lib

SRC_DIRS := $(addprefix $(SRC_DIR)/,$(MODULES))
OBJ_DIRS := $(addprefix $(OBJ_DIR)/,$(MODULES))

SRCS := $(foreach sdir,$(SRC_DIRS),$(wildcard $(sdir)/*.cpp))
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

CXX_FLAGS := -ggdb -fPIC -MMD -MP -DBOOST_TEST_DYN_LINK

ifeq ($(TARGET),release)
	CXX_FLAGS += -O3
endif

vpath %.cpp $(SRC_DIRS)

define compile
$1/%.o: %.cpp
	$(CXX) -c $$(CXX_FLAGS) $$< -o $$@
endef

.PHONY: all mkdirs clean

all: mkdirs \
	$(BIN_DIR)/fbods	\
	$(BIN_DIR)/fbinsert \
	$(BIN_DIR)/fbtest	\
	$(LIB_DIR)/libperfudr.$(SHRLIB_EXT) \

mkdirs: $(OBJ_DIRS) $(BIN_DIR) $(LIB_DIR)

$(OBJ_DIRS) $(BIN_DIR) $(LIB_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD_DIR) $(OUT_DIR)

$(foreach bdir,$(OBJ_DIRS),$(eval $(call compile,$(bdir))))

-include $(addsuffix .d,$(basename $(OBJS)))

$(BIN_DIR)/fbods: \
	$(OBJ_DIR)/ods/FullScanStream.o \
	$(OBJ_DIR)/ods/Main.o \

	$(LD) $^ -o $@

$(BIN_DIR)/fbinsert: $(OBJ_DIR)/perf/fbinsert.o
	$(LD) $^ -o $@ -lboost_program_options -lboost_system -lboost_thread -lfbclient

$(BIN_DIR)/fbtest: \
	$(OBJ_DIR)/test/FbTest.o \
	$(OBJ_DIR)/test/v3api/V3Util.o \
	$(OBJ_DIR)/test/v3api/AffectedRecordsTest.o \
	$(OBJ_DIR)/test/v3api/BlobTest.o \
	$(OBJ_DIR)/test/v3api/CursorTest.o \
	$(OBJ_DIR)/test/v3api/DescribeTest.o \
	$(OBJ_DIR)/test/v3api/DescribeCTest.o \
	$(OBJ_DIR)/test/v3api/DynamicMessageTest.o \
	$(OBJ_DIR)/test/v3api/EventsTest.o \
	$(OBJ_DIR)/test/v3api/MultiDbTransTest.o \
	$(OBJ_DIR)/test/v3api/StaticMessageTest.o \

	$(LD) $^ -o $@ -lboost_unit_test_framework -lboost_system -lboost_thread -lfbclient

$(LIB_DIR)/libperfudr.$(SHRLIB_EXT): $(OBJ_DIR)/perf/udr/Message.o
	$(LD) -shared $^ -o $@ -lfbclient -ludr_engine
