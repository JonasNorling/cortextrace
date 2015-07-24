BUILDDIR = build

LIB_SRCS += src/log.cpp
LIB_SRCS += src/GdbConnection.cpp
LIB_SRCS += src/GdbConnectionState.cpp
LIB_SRCS += src/TraceEvent.cpp
LIB_SRCS += src/TraceEventListener.cpp
LIB_SRCS += src/TraceFileParser.cpp

LIB_OBJS := $(LIB_SRCS:%.cpp=$(BUILDDIR)/%.o)

OBJS += $(LIB_OBJS)

CFLAGS += -Werror -Wall -Wpedantic -Weffc++ -std=c++11 -g -O1
CPPFLAGS += -Iinclude/ -Isrc/ -I3pp/

ifeq ($(shell uname), Darwin)
LIBMI := libmi.dylib
else
LIBMI := libmi.so
endif

.PHONY: all
all: $(BUILDDIR)/libcortextrace.a tools

# ---------------------------------------------------------------------

.PHONY: test
test: $(BUILDDIR)/testTraceFileParser
	$(BUILDDIR)/testTraceFileParser
 
OBJS += $(BUILDDIR)/src/test/TestTraceFileParser.o
$(BUILDDIR)/testTraceFileParser: $(BUILDDIR)/src/test/TestTraceFileParser.o $(BUILDDIR)/libcortextrace.a
	@echo CXX $<
	@$(CXX) $(CFLAGS) -o $@ $< -L $(BUILDDIR) -lcortextrace

# ---------------------------------------------------------------------

.PHONY: tools
tools: $(BUILDDIR)/cortextrace $(BUILDDIR)/cortexwatch

OBJS += $(BUILDDIR)/tools/cortextrace.o
$(BUILDDIR)/cortextrace: $(BUILDDIR)/tools/cortextrace.o $(BUILDDIR)/libcortextrace.a
	@echo CXX $@
	@$(CXX) $(CFLAGS) -o $@ $< -L $(BUILDDIR) -lcortextrace

OBJS += $(BUILDDIR)/tools/cortexwatch.o
$(BUILDDIR)/cortexwatch: $(BUILDDIR)/tools/cortexwatch.o $(BUILDDIR)/libcortextrace.a $(BUILDDIR)/$(LIBMI)
	@echo CXX $@
	@$(CXX) $(CFLAGS) -o $@ $< -L $(BUILDDIR) -lcortextrace -lmi

# ---------------------------------------------------------------------

$(BUILDDIR)/$(LIBMI):
	echo Building $(LIBMI)
	mkdir -p $(BUILDDIR)/libmi
	cd $(BUILDDIR)/libmi && ../../3pp/libmi/configure
	$(MAKE) -j1 -C $(BUILDDIR)/libmi/
	cp $(BUILDDIR)/libmi/.libs/$(LIBMI) $@
	touch $@

# ---------------------------------------------------------------------

DEPFILES := $(patsubst %.o,%.d,$(OBJS))
-include $(DEPFILES)


# mi_gdb.h doesn't pass all GCC's pedantery
#$(BUILDDIR)/src/GdbConnection.o: CCFLAGS += -Wno-pedantic -Wno-effc++

$(BUILDDIR)/libcortextrace.a: $(LIB_OBJS) Makefile
	$(AR) cr $@ $(LIB_OBJS)

$(BUILDDIR)/%.o: %.cpp Makefile
	@echo CXX $<
	@mkdir -p $(dir $@)
	@$(CXX) -MMD -MP $(CPPFLAGS) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR)
	