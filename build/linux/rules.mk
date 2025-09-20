include build/linux/colors

export SRC_PATH  := src
export OBJ_DIR   := obj_$(TIM_PLATFORM)

ifeq ($(TIM_SILENT), 1)
	export AT := @
else
	export AT :=
endif

export SUBDIRS := $(shell find $(SRC_PATH) -type d -execdir test -f {}/IGNORE ';' -prune -o \( -type d -print \))

export INCLUDES := $(addprefix -I, $(SUBDIRS)) $(TIM_INCLUDES)

ifdef TIM_DEBUG_MODE
	TIM_DEFINES  := -DTIM_OS_LINUX -DTIM_DEBUG
	TIM_CFLAGS   := -g -O0 -Wall -Werror
	TIM_CPPFLAGS := -g -O0 -Wall -Werror
	TIM_STRIP    := touch
else
	TIM_DEFINES  := -DTIM_OS_LINUX -DNDEBUG
	TIM_CFLAGS   := -O3 -Wall -Werror
	TIM_CPPFLAGS := -O3 -Wall -Werror
	TIM_STRIP    := $(STRIP)
endif

TIM_LIBS := -lm

PARTCL_DEFINES := -DTCL_DISABLE_PUTS

DEFINES := $(TIM_DEFINES) $(PARTCL_DEFINES)
CFLAGS   := $(TIM_CFLAGS)
CPPFLAGS := $(TIM_CPPFLAGS)

export LIBS := -Xlinker --start-group $(TIM_LIBS) $(SDL_LIBS) -Xlinker --end-group

C_SRCS := $(shell find $(SRC_PATH) -type f -name *.c -execdir test ! -f IGNORE ';' -print)
CPP_SRCS := $(shell find $(SRC_PATH) -type f -name *.cpp -execdir test ! -f IGNORE ';' -print)

VPATH := $(SUBDIRS)

all:	tim

### Load dependencies
### -----------------
DEPS := $(wildcard $(OBJ_DIR)/*.d)
ifneq ($(strip $(DEPS)),)
include $(DEPS)
endif

### Compilation and dependencies generation
### ---------------------------------------

define COMPILE_C
@echo $(TEXT_BG_BLUE)$(TEXT_FG_BOLD_YELLOW)" C "$(TEXT_NORM)$(TEXT_FG_BOLD_CYAN)$< $(TEXT_NORM)
$(AT)$(CC) -c -MD $(CFLAGS) $(if $(findstring 3rdparty,$(dir $<)),$(CFLAGS_3RD),) $(INCLUDES) $(DEFINES) -o $@ $<
endef

define COMPILE_CPP
@echo $(TEXT_BG_BLUE)$(TEXT_FG_BOLD_YELLOW)"C++"$(TEXT_NORM)$(TEXT_FG_BOLD_CYAN)$< $(TEXT_NORM)
$(AT)$(CPP) -c -MD $(CPPFLAGS) $(if $(findstring 3rdparty,$(dir $<)),$(CFLAGS_3RD),) $(INCLUDES) $(DEFINES) -o $@ $<
endef

define DEPENDENCIES_C
@if [ ! -f $(@D)/$(<F:.c=.d) ]; then \
    sed 's/^$(@F):/$(@D)\/$(@F):/g' < $(<F:.c=.d) > $(@D)/$(<F:.c=.d); \
    rm -f $(<F:.c=.d); \
fi
endef

define DEPENDENCIES_CPP
@if [ ! -f $(@D)/$(<F:.cpp=.d) ]; then \
    sed 's/^$(@F):/$(@D)\/$(@F):/g' < $(<F:.cpp=.d) > $(@D)/$(<F:.cpp=.d); \
    rm -f $(<F:.cpp=.d); \
fi
endef

C_OBJS   := $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(C_SRCS)))
CPP_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(CPP_SRCS)))

.SECONDARY:	$(C_OBJS) $(CPP_OBJS)
.PHONY:	clean build-time

### Target rules
### ------------

build-time:
		$(AT)printf '"%s"' $(shell date '+%Y%m%d-%H%M') > BUILD_TIME

$(OBJ_DIR)/%.o: %.cpp
		$(COMPILE_CPP)
		$(DEPENDENCIES_CPP)

$(OBJ_DIR)/%.o: %.c
		$(COMPILE_C)
		$(DEPENDENCIES_C)

tim: build-time $(OBJ_DIR) $(C_OBJS) $(CPP_OBJS)
		@echo $(TEXT_BG_GREEN)$(TEXT_FG_BLACK)" L "$(TEXT_NORM)$(TEXT_FG_BOLD_GREEN)$@ $(TEXT_NORM)
		@rm -f $@
		$(AT)$(CC) $(CFLAGS) $(DEFINES) -o $@ $(C_OBJS) $(CPP_OBJS) $(LIBS)
		$(AT)$(TIM_STRIP) $@

$(OBJ_DIR):
		$(AT)mkdir $(OBJ_DIR)

clean:
		@echo $(TEXT_FG_LIGHT_GREEN)"> Cleaning ... "$(TEXT_NORM)
		$(AT)rm -rf $(OBJ_DIR)
		$(AT)rm -f *~ *.d *.gdb core* *.so *.a BUILD_TIME .DS_Store tim
		@echo $(TEXT_FG_LIGHT_GREEN)"> Done :) "$(TEXT_NORM)
