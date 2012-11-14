# TODO: add checks here for variables, flags etc.

################################################################################
# print debug information if needed
ifdef MAKEFILE_DEBUG
    $(info ===================compile.core.make================================)
endif

################################################################################
# CFLAGS
################################################################################

# clean it
ALL_CFLAGS =

# add the core flags (platform flags are aggregated in here)
ALL_CFLAGS += $(OF_CORE_BASE_CFLAGS)
# add the defines
ALL_CFLAGS += $(OF_CORE_DEFINES_CFLAGS)
# add the include cflags
ALL_CFLAGS += $(OF_CORE_INCLUDES_CFLAGS)
# clean up all extra whitespaces in the CFLAGS
CFLAGS = $(strip $(ALL_CFLAGS))

################################################################################
# COMPILER OPTIMIZATIONS and TARGET GENERATION
################################################################################
#
# $(TARGET_NAME) is the name of the target defined for the use of the 
# CLEANTARGET target. $(TARGET) is the name of the library that will be created.
#
# If TARGET has not already been defined in a platform-based make file, we use 
# this selection system to define our TARGET.  Platform specific targets should
# define the following and respect the "CLEANTARGET system below."
#  
#  $(TARGET)
#  $(TARGET_NAME)
#  #(OPTIMIZATION_CFLAGS)
#

# check to see if our target has been defined elsewhere
ifndef TARGET
	# check to see if this is a "pure" clean target
	ifeq ($(MAKECMDGOALS),clean)
	    TARGET =
	    TARGET += $(OF_CORE_LIB_PATH)/libopenFrameworks.a
	    TARGET += $(OF_CORE_LIB_PATH)/libopenFrameworksDebug.a
	    TARGET_NAME = 

	# check to see if any part of our target includes the String "Debug"
	# this will happen if we call Debug OR CleanDebug
	else ifeq ($(findstring Debug,$(MAKECMDGOALS)),Debug)
	    OPTIMIZATION_CFLAGS = $(PLATFORM_OPTIMIZATION_CFLAGS_DEBUG)
	    TARGET_NAME = Debug
	    TARGET = $(OF_CORE_LIB_PATH)/libopenFrameworksDebug.a

	# check to see if any part of our target includes the String "Release"
	# this will happen if we call Release OR CleanRelease
	else ifeq ($(findstring Release,$(MAKECMDGOALS)),Release)
	    OPTIMIZATION_CFLAGS = $(PLATFORM_OPTIMIZATION_CFLAGS_RELEASE)
	    TARGET_NAME = Release
	    TARGET = $(OF_CORE_LIB_PATH)/libopenFrameworks.a
	else ## why doesn't make allow for easy logical operators?
	    OPTIMIZATION_CFLAGS = $(PLATFORM_OPTIMIZATION_CFLAGS_RELEASE)
	    TARGET_NAME = Release
	    TARGET = $(OF_CORE_LIB_PATH)/libopenFrameworks.a
	 endif
 endif

# we only get a CLEAN_TARGET if a TARGET_NAME has been defined
# Like TARGET, this must be defined above or in a platform file.
ifdef TARGET_NAME
	CLEANTARGET = $(addprefix Clean,$(TARGET_NAME))
endif

################################################################################
# OBJECT AND DEPENDENCY FILES DEFINITIONS
#	Object file paths are generated here (as opposed to with the rest of the 
#   flags) because we want to place them in target-specific folders. We
#   determine targets above. We –could– determine the target info earlier if we
#   wanted to.  It's here because that's approximately where it was in the 
#   legacy makefiles.
################################################################################

# define the location for our intermediate object files
OF_CORE_OBJ_BASE_PATH = $(PLATFORM_LIB_SUBPATH)/obj

# define the subdirectory for our target name
OF_CORE_OBJ_OUPUT_PATH =$(OF_CORE_OBJ_BASE_PATH)/$(TARGET_NAME)

# create a named list of dependency files
# 1. create a list of .d dependency files based on the current list of 
#  OF_CORE_SOURCE_FILES $(patsubst $(OF_ROOT)/%.cpp,%.d,$(OF_CORE_SOURCE_FILES))
# 2. Add the OF_CORE_OBJ_OUPUT_PATH as a prefix 
#  $(addprefix $(OF_CORE_OBJ_OUPUT_PATH), ...)
OF_CORE_DEPENDENCY_FILES = $(addprefix $(OF_CORE_OBJ_OUPUT_PATH),$(patsubst $(OF_ROOT)/%.cpp,%.d,$(OF_CORE_SOURCE_FILES)))

# create a named list of object files
# 1. create a list of object files based on the current list of
#   OF_CORE_SOURCE_FILES $(patsubst $(OF_ROOT)/%.cpp,%.o,$(OF_CORE_SOURCE_FILES)
# 2. Add the OF_CORE_OBJ_OUPUT_PATH as a prefix 
#	$(addprefix $(OF_CORE_OBJ_OUPUT_PATH), ...)
OF_CORE_OBJ_FILES = $(addprefix $(OF_CORE_OBJ_OUPUT_PATH),$(patsubst $(OF_ROOT)/%.cpp,%.o,$(OF_CORE_SOURCE_FILES)))

ifdef MAKEFILE_DEBUG
    $(info OF_CORE_OBJ_OUPUT_PATH=$(OF_CORE_OBJ_OUPUT_PATH))
    $(info ---OF_CORE_DEPENDENCY_FILES---)
    $(foreach v, $(OF_CORE_DEPENDENCY_FILES),$(info $(v)))
    $(info ---OF_CORE_OBJ_FILES---)
    $(foreach v, $(OF_CORE_OBJ_FILES),$(info $(v)))
endif

################################################################################
# While most MAKE targets respond to lists of filenames, .PHONY targets are 
# targets that are "recipe" only -- that is recipes that respond to specific
# requests, not filenames or lists of filenames.  .PNONY targets are used to 
# avoid conflict with files of the same name and to improve performance.
.PHONY: all Debug Release after clean CleanDebug CleanRelease help

# Release will pass the library name (i.e. ... libopenFrameworks.a) 
# down the the @(TARGET) target
Release: $(TARGET) after

# Debug will pass the library name (i.e. ... libopenFrameworksDebug.a)
# down the the @(TARGET) target
Debug: $(TARGET) after

# all will first run the debug target, then the release target
all: 
	$(MAKE) Debug
	$(MAKE) Release

#This rule does the compilation
$(OF_CORE_OBJ_OUPUT_PATH)%.o: $(OF_ROOT)/%.cpp 
	@echo "Compiling" $<
	mkdir -p $(@D)
	$(CXX) $(OPTIMIZATION_CFLAGS) $(CFLAGS) -MMD -MP -MF$(OF_CORE_OBJ_OUPUT_PATH)$*.d -MT$(OF_CORE_OBJ_OUPUT_PATH)$*.o -o $@ -c $<

# this target does the linking of the library
# $(TARGET) : $(OF_CORE_OBJ_FILES) means that each of the items in the 
# $(OF_CORE_OBJ_FILES) must be processed first  
$(TARGET) : $(OF_CORE_OBJ_FILES) 
	echo "Creating library " $(TARGET)
	mkdir -p $(@D)
	$(AR) -cr "$@" $(OF_CORE_OBJ_FILES)

-include $(OF_CORE_DEPENDENCY_FILES)

#.PHONY: clean CleanDebug CleanRelease
clean:
	@echo "Removing object files in " $(OF_CORE_OBJ_OUPUT_PATH)
	rm -Rf $(OF_CORE_OBJ_OUPUT_PATH)
	@echo "Removing " $(TARGET)
	rm -f $(TARGET)

$(CLEANTARGET):
	@echo "Removing object files in " $(OF_CORE_OBJ_OUPUT_PATH)
	rm -Rf $(OF_CORE_OBJ_OUPUT_PATH)
	@echo "Removing " $(TARGET)
	rm -f $(TARGET)

after: $(TARGET)
	@echo "Done!"

help:
	@echo 
	@echo openFrameworks compiled library makefile
	@echo
	@echo "Targets:"
	@echo
	@echo "make Debug:		builds the library with debug symbols"
	@echo "make Release:		builds the library with optimizations"
	@echo "make:			= make Release"
	@echo "make all:		= make Debug + make Release"
	@echo "make CleanDebug:	cleans the Debug target"
	@echo "make CleanRelease:	cleans the Release target"
	@echo "make clean:		cleans everything"
	@echo "make help:		this help message"
	@echo
	@echo "Platform Variants:"
	@echo
	@echo "openFrameworks makefiles can be customized for a generic platform"
	@echo "(i.e. Linux or Darwin) and can be further customized with a"
	@echo "platform variant make files. To use a platform variant makefile"
	@echo "set the PLATFORM_VARIANT variable to a valid variant."
	@echo 
	@echo "e.g."
	@echo 
	@echo "make Release PLATFORM_VARIANT=raspberrypi"
	@echo
	@echo "In this case, the makefile will automatically determine architecture"
	@echo "architecture and operating system, and will use the raspberrypi"
	@echo "makefile variant rather than the \"default\" platform variant."
	@echo
	@echo "Debugging:"
	@echo
	@echo "openFrameworks makefiles offer a lot of debugging information"
	@echo "for the curious developer.  To see debugging output define the"
	@echo "MAKEFILE_DEBUG variable."
	@echo
	@echo "e.g."
	@echo
	@echo "make Release MAKEFILE_DEBUG=true"
	@echo
	@echo