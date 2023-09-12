TARGET		= GoodBye051

BUILD		= build
INCLUDES	= include
SOURCES		= source

%.o: %.cpp
	@echo $(notdir $<)
	@g++ $(INCLUDE) -c -o $@ $< 

ifneq ($(notdir $(CURDIR)), $(BUILD))

CXXFILES	= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

export VPATH		= $(CURDIR)/$(SOURCES)
export OFILES		= $(CXXFILES:.cpp=.o)
export OUTPUT		= $(CURDIR)/$(TARGET)
export INCLUDE		= $(foreach dir,$(INCLUDES),-I $(CURDIR)/$(dir))

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -rf $(BUILD)
	@rm -f $(TARGET)

re: clean all

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

else

DEPENDS	= $(OFILES:.o=.d)

$(OUTPUT): $(OFILES)
	@echo linking...
	@g++ $(INCLUDE) -o $@ $^

-include $(DEPENDS)

endif