BUILDDIR = build

SRCS += src/libcortextrace.cpp
OBJS := $(SRCS:%.cpp=$(BUILDDIR)/%.o)

CFLAGS += -Werror -Wall -Wpedantic -std=c++11 -g -O1
CPPFLAGS += -Iinclude/

DEPFILES := $(patsubst %.o,%.d,$(OBJS))
-include $(DEPFILES)

all: $(BUILDDIR)/libcortextrace.a

$(BUILDDIR)/libcortextrace.a: $(OBJS) Makefile
	$(AR) cr $@ $(OBJS)

$(BUILDDIR)/%.o: %.cpp Makefile
	@echo CC $<
	@mkdir -p $(dir $@)
	@$(CC) -MMD -MP $(CPPFLAGS) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR)
	