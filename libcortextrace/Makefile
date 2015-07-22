BUILDDIR = build

LIB_SRCS += src/log.cpp
LIB_SRCS += src/TraceEvent.cpp
LIB_SRCS += src/TraceEventListener.cpp
LIB_SRCS += src/TraceFileParser.cpp

LIB_OBJS := $(LIB_SRCS:%.cpp=$(BUILDDIR)/%.o)

OBJS += $(LIB_OBJS)

CFLAGS += -Werror -Wall -Wpedantic -Weffc++ -std=c++11 -g -O1
CPPFLAGS += -Iinclude/ -Isrc/

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
tools: $(BUILDDIR)/cortextrace

OBJS += $(BUILDDIR)/tools/cortextrace.o
$(BUILDDIR)/cortextrace: $(BUILDDIR)/tools/cortextrace.o $(BUILDDIR)/libcortextrace.a
	@echo CXX $@
	@$(CXX) $(CFLAGS) -o $@ $< -L $(BUILDDIR) -lcortextrace

# ---------------------------------------------------------------------

$(BUILDDIR)/libmi.so:
	mkdir -p $(BUILDDIR)/libmi
	cd $(BUILDDIR)/libmi && ../../3pp/libmi/configure
	$(MAKE) -C $(BUILDDIR)/libmi/
	cp $(BUILDDIR)/libmi/.libs/libmi.so $@
	touch $@

# ---------------------------------------------------------------------

DEPFILES := $(patsubst %.o,%.d,$(OBJS))
-include $(DEPFILES)


$(BUILDDIR)/libcortextrace.a: $(LIB_OBJS) Makefile
	$(AR) cr $@ $(LIB_OBJS)

$(BUILDDIR)/%.o: %.cpp Makefile
	@echo CXX $<
	@mkdir -p $(dir $@)
	@$(CXX) -MMD -MP $(CPPFLAGS) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR)
	