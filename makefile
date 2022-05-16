bin/sandbox : src/sms/sandbox.cpp
	clang++ -std=c++17 src/sms/sandbox.cpp -o bin/sandbox

## this Makefile assumes directory contents, structure:
##   ./bin - contains stanc compiler
##   ./lib - contains rapidjson parser library
##   ./src - contains cmdstan/io/json
##   ./stan - stan repo, contains model-header
##   ./stan/lib/stan_math - stan math library

STAN ?= stan/
MATH ?= $(STAN)lib/stan_math/
STANC ?= bin/stanc
RAPIDJSON ?= lib/rapidjson_1.1.0/

INC_FIRST ?= -I src -I $(STAN)src -I $(RAPIDJSON)

## only runs on Mac, for now
OS ?= $(shell uname -s)
CXX ?= clang++
CXX_TYPE ?= clang

## makefile needed for math library
-include $(MATH)make/compiler_flags
-include $(MATH)make/dependencies
-include $(MATH)make/libraries

ifdef STAN_OPENCL
STANCFLAGS+= --use-opencl
endif


DRIVER_MAIN ?= main.cpp
DRIVER_MAIN_O = $(patsubst %.cpp,%.o,$(DRIVER_MAIN))

$(DRIVER_MAIN_O) : $(DRIVER_MAIN)
	@mkdir -p $(dir $@)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $(LDLIBS) $<

## user requests exe file, makefile target is corresponding stan file
STAN_TARGETS = $(patsubst %.stan,%,$(wildcard $(patsubst %,%.stan,$(MAKECMDGOALS))))

%.hpp : %.stan $(STANC)
	@echo ''
	@echo '--- Translating Stan model to C++ code ---'
	$(STANC) $(STANCFLAGS) --o=$(subst  \,/,$@) $(subst  \,/,$<)

%.d: %.hpp

.PRECIOUS: %.hpp
%$(EXE) : %.hpp $(DRIVER_MAIN_O) $(LIBSUNDIALS) $(MPI_TARGETS) $(TBB_TARGETS)
	@echo ''
	@echo '--- Compiling, linking C++ code ---'
	$(COMPILE.cpp) $(CXXFLAGS_PROGRAM) -x c++ -o $(subst  \,/,$*).o $(subst \,/,$<)
	$(LINK.cpp) $(subst \,/,$*.o) $(DRIVER_MAIN_O) $(LDLIBS) $(LIBSUNDIALS) $(MPI_TARGETS) $(TBB_TARGETS) $(subst \,/,$(OUTPUT_OPTION))
	$(RM) $(subst  \,/,$*).o

## calculate dependencies
ifneq (,$(STAN_TARGETS))
$(patsubst %,%.d,$(STAN_TARGETS)) : DEPTARGETS += -MT $(patsubst %.d,%$(EXE),$@) -include $< -include $(DRIVER_MAIN)
-include $(patsubst %,%.d,$(STAN_TARGETS))
-include $(patsubst %.cpp,%.d,$(DRIVER_MAIN))
endif

.PHONY: build
build: bin/stanc$(EXE) $(LIBSUNDIALS) $(MPI_TARGETS) $(TBB_TARGETS) $(DRIVER_MAIN_O)
	@echo ''

.PHONY: install-tbb
install-tbb: $(TBB_TARGETS)

##
# Clean up.
##
.PHONY: clean clean-deps clean-all clean-program

clean-deps:
	@echo '  removing dependency files'
	$(RM) $(call findfiles,src,*.d) $(call findfiles,src/stan,*.d) $(call findfiles,$(MATH)/stan,*.d) $(call findfiles,$(STAN)/src/stan/,*.d)
	$(RM) $(call findfiles,src,*.d.*) $(call findfiles,src/stan,*.d.*) $(call findfiles,$(MATH)/stan,*.d.*)
	$(RM) $(call findfiles,src,*.dSYM) $(call findfiles,src/stan,*.dSYM) $(call findfiles,$(MATH)/stan,*.dSYM)

clean-all: clean clean-deps clean-libraries
	$(RM) $(DRIVER_MAIN_O)
	$(RM) -r $(wildcard $(BOOST)/stage/lib $(BOOST)/bin.v2 $(BOOST)/tools/build/src/engine/bootstrap/ $(BOOST)/tools/build/src/engine/bin.* $(BOOST)/project-config.jam* $(BOOST)/b2 $(BOOST)/bjam $(BOOST)/bootstrap.log)

clean-program:
ifndef STANPROG
	$(error STANPROG not set)
endif
	$(RM) "$(wildcard $(patsubst %.stan,%.d,$(basename ${STANPROG}).stan))"
	$(RM) "$(wildcard $(patsubst %.stan,%.hpp,$(basename ${STANPROG}).stan))"
	$(RM) "$(wildcard $(patsubst %.stan,%.o,$(basename ${STANPROG}).stan))"
	$(RM) "$(wildcard $(patsubst %.stan,%$(EXE),$(basename ${STANPROG}).stan))"


##
# Debug target that prints compile command
##

.PHONY: compile_info
compile_info:
	@echo '$(LINK.cpp) $(CXXFLAGS_PROGRAM) $(DRIVER_MAIN_O) $(LDLIBS) $(LIBSUNDIALS) $(MPI_TARGETS) $(TBB_TARGETS)'

##
# Debug target that allows you to print a variable
##
.PHONY: print-%
print-%  : ; @echo $* = $($*) ;
