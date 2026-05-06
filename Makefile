NOZZLE_DIR := deps/nozzle
PLOG_DIR := $(NOZZLE_DIR)/libs/plog/include
BUILD_DIR := .build

CXX := c++
AR := ar

CXXFLAGS := -std=c++17 -fno-exceptions -fno-rtti -O2 -fPIC

UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
	PLATFORM := windows
else ifeq ($(UNAME_S),Darwin)
	PLATFORM := macos
else
	PLATFORM := linux
endif

COMMON_SRCS := \
	$(NOZZLE_DIR)/src/common/ipc.cpp \
	$(NOZZLE_DIR)/src/common/registry.cpp \
	$(NOZZLE_DIR)/src/common/sender.cpp \
	$(NOZZLE_DIR)/src/common/receiver.cpp \
	$(NOZZLE_DIR)/src/common/frame.cpp \
	$(NOZZLE_DIR)/src/common/texture.cpp \
	$(NOZZLE_DIR)/src/common/device.cpp \
	$(NOZZLE_DIR)/src/common/discovery.cpp \
	$(NOZZLE_DIR)/src/common/metadata.cpp \
	$(NOZZLE_DIR)/src/common/pixel_access.cpp \
	$(NOZZLE_DIR)/src/common/channel_swizzle.cpp \
	$(NOZZLE_DIR)/src/common/format_convert.cpp \
	$(NOZZLE_DIR)/src/common/format_convert_sse2.cpp \
	$(NOZZLE_DIR)/src/common/format_convert_neon.cpp \
	$(NOZZLE_DIR)/src/common/format_resolve.cpp \
	$(NOZZLE_DIR)/src/c_api/nozzle_c.cpp \
	$(NOZZLE_DIR)/src/backends/opengl/opengl_backend.cpp

PD_SRCS := \
	src/pix_nozzle_send.cpp \
	src/pix_nozzle_receive.cpp \
	src/pix_nozzle_gl_send.cpp \
	src/pix_nozzle_gl_receive.cpp

ifeq ($(PLATFORM),macos)
	CXXFLAGS += -DNOZZLE_PLATFORM_MACOS=1 -DNOZZLE_HAS_METAL=1 -DNOZZLE_HAS_OPENGL=1
	PLATFORM_SRCS := \
		$(NOZZLE_DIR)/src/backends/metal/metal_backend.mm \
		$(NOZZLE_DIR)/src/backends/metal/metal_texture.mm \
		$(NOZZLE_DIR)/src/backends/metal/metal_channel_swap.mm \
		$(NOZZLE_DIR)/src/backends/metal/metal_sync.mm \
		$(NOZZLE_DIR)/src/common/channel_swizzle_vimage.cpp \
		$(NOZZLE_DIR)/src/common/format_convert_vimage.cpp
	LDFLAGS := -bundle -undefined dynamic_lookup \
		-framework Metal -framework IOSurface -framework Foundation \
		-framework Accelerate -framework OpenGL -framework CoreVideo \
		-lobjc -lstdc++
	PD_CFLAGS := $(shell pkg-config --cflags pd 2>/dev/null || echo "-I/usr/local/include/pd")
	GEM_CFLAGS := $(shell pkg-config --cflags Gem 2>/dev/null || echo "-I/usr/local/include/Gem")
	EXT := .pd_darwin
endif

ifeq ($(PLATFORM),linux)
	CXXFLAGS += -DNOZZLE_PLATFORM_LINUX=1 -DNOZZLE_HAS_DMA_BUF=1 -DNOZZLE_HAS_OPENGL=1
	PLATFORM_SRCS := \
		$(NOZZLE_DIR)/src/backends/linux/linux_texture.cpp
	LDFLAGS := -shared \
		-ldrm -lgbm -lEGL -lGL -lstdc++
	PD_CFLAGS := $(shell pkg-config --cflags pd 2>/dev/null || echo "-I/usr/include/pd")
	GEM_CFLAGS := $(shell pkg-config --cflags Gem 2>/dev/null || echo "-I/usr/include/Gem")
	EXT := .pd_linux
endif

ifeq ($(PLATFORM),windows)
	CXXFLAGS += -DNOZZLE_PLATFORM_WINDOWS=1 -DNOZZLE_HAS_D3D11=1 -DNOZZLE_HAS_OPENGL=1
	PLATFORM_SRCS := \
		$(NOZZLE_DIR)/src/backends/d3d11/d3d11_backend.cpp \
		$(NOZZLE_DIR)/src/backends/d3d11/d3d11_texture.cpp \
		$(NOZZLE_DIR)/src/backends/d3d11/d3d11_sync.cpp
	LDFLAGS := -shared -ld3d11 -ldxgi -lopengl32 -lbcrypt -lstdc++
	PD_CFLAGS := $(shell pkg-config --cflags pd 2>/dev/null)
	GEM_CFLAGS := $(shell pkg-config --cflags Gem 2>/dev/null)
	EXT := .dll
endif

INCLUDES := -I$(NOZZLE_DIR)/include -I$(NOZZLE_DIR)/src -I$(PLOG_DIR) \
	$(PD_CFLAGS) $(GEM_CFLAGS)

ALL_NOZZLE_SRCS := $(COMMON_SRCS) $(PLATFORM_SRCS)
ALL_NOZZLE_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(patsubst %.mm,$(BUILD_DIR)/%.o,$(ALL_NOZZLE_SRCS)))
NOZZLE_LIB := $(BUILD_DIR)/libnozzle.a

TARGETS := $(BUILD_DIR)/pix_nozzle_send$(EXT) \
	$(BUILD_DIR)/pix_nozzle_receive$(EXT) \
	$(BUILD_DIR)/pix_nozzle_gl_send$(EXT) \
	$(BUILD_DIR)/pix_nozzle_gl_receive$(EXT)

.PHONY: all clean

all: $(TARGETS)

$(BUILD_DIR)/pix_nozzle_send$(EXT): src/pix_nozzle_send.cpp $(NOZZLE_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $(BUILD_DIR)/pix_nozzle_send.o
	$(CXX) $(LDFLAGS) -o $@ $(BUILD_DIR)/pix_nozzle_send.o $(NOZZLE_LIB)

$(BUILD_DIR)/pix_nozzle_receive$(EXT): src/pix_nozzle_receive.cpp $(NOZZLE_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $(BUILD_DIR)/pix_nozzle_receive.o
	$(CXX) $(LDFLAGS) -o $@ $(BUILD_DIR)/pix_nozzle_receive.o $(NOZZLE_LIB)

$(BUILD_DIR)/pix_nozzle_gl_send$(EXT): src/pix_nozzle_gl_send.cpp $(NOZZLE_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $(BUILD_DIR)/pix_nozzle_gl_send.o
	$(CXX) $(LDFLAGS) -o $@ $(BUILD_DIR)/pix_nozzle_gl_send.o $(NOZZLE_LIB)

$(BUILD_DIR)/pix_nozzle_gl_receive$(EXT): src/pix_nozzle_gl_receive.cpp $(NOZZLE_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $(BUILD_DIR)/pix_nozzle_gl_receive.o
	$(CXX) $(LDFLAGS) -o $@ $(BUILD_DIR)/pix_nozzle_gl_receive.o $(NOZZLE_LIB)

$(NOZZLE_LIB): $(ALL_NOZZLE_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: %.mm
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
