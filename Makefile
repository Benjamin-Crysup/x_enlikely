
# figure out what flavor of processor this is running on

uname_p := $(shell uname -p)
ifeq ($(uname_p),i386)
PROC_NAME = x86
else ifeq ($(uname_p),i686)
PROC_NAME = x86
else ifeq ($(uname_p),x86_64)
PROC_NAME = x64
else ifeq ($(uname_p),amd64)
PROC_NAME = x64
else
PROC_NAME = oth
endif

COMP_OPTS = -g -O0 -Wall -pthread
BASIC_PROG_LIBS = -ldl -lz
GPU_PROG_LIBS = -lvulkan
OBJDIR = builds/obj_$(PROC_NAME)_linux
BINDIR = builds/bin_$(PROC_NAME)_linux

# *******************************************************************
# super targets

all : allgpu alllibs allexes alldoc allexper allgdoc

alllibs : $(BINDIR)/libwhodun.a $(BINDIR)/libwhodunext.a

allexes : $(BINDIR)/whodun $(BINDIR)/whodunu

allgpu : $(BINDIR)/libwhodungpu.a $(BINDIR)/whodung

alldoc : doc/html/index.html doc/whodun/main.html doc/whodunu/main.html

allgdoc :  doc/whodung/main.html

allexper : $(BINDIR)/experiments/exp_test $(BINDIR)/experiments/2023/exp_genlike

clean : 
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
	rm -rf doc/*

$(BINDIR) : 
	mkdir -p $(BINDIR)

# *******************************************************************
# stable

STABLE_OBJDIR = $(OBJDIR)/stable

STABLE_HEADERS = stable/whodun_args.h stable/whodun_compress.h stable/whodun_container.h stable/whodun_ermac.h stable/whodun_math_constants.h stable/whodun_oshook.h stable/whodun_parse.h stable/whodun_regex.h stable/whodun_sort.h stable/whodun_stat_data.h stable/whodun_stat_randoms.h stable/whodun_stat_table.h stable/whodun_stat_util.h stable/whodun_streams.h stable/whodun_string.h stable/whodun_thread.h

$(STABLE_OBJDIR) : 
	mkdir -p $(STABLE_OBJDIR)

$(STABLE_OBJDIR)/w_args.o : stable/w_args.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_args.o stable/w_args.cpp
$(STABLE_OBJDIR)/w_compress.o : stable/w_compress.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_compress.o stable/w_compress.cpp
$(STABLE_OBJDIR)/w_container.o : stable/w_container.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_container.o stable/w_container.cpp
$(STABLE_OBJDIR)/w_ermac.o : stable/w_ermac.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_ermac.o stable/w_ermac.cpp
$(STABLE_OBJDIR)/w_oshook_com.o : stable/w_oshook_com.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_oshook_com.o stable/w_oshook_com.cpp
$(STABLE_OBJDIR)/w_oshook_linux.o : stable/w_oshook_linux.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_oshook_linux.o stable/w_oshook_linux.cpp
$(STABLE_OBJDIR)/w_parse.o : stable/w_parse.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_parse.o stable/w_parse.cpp
$(STABLE_OBJDIR)/w_regex.o : stable/w_regex.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_regex.o stable/w_regex.cpp
$(STABLE_OBJDIR)/w_sort.o : stable/w_sort.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_sort.o stable/w_sort.cpp
$(STABLE_OBJDIR)/w_stat_data.o : stable/w_stat_data.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_stat_data.o stable/w_stat_data.cpp
$(STABLE_OBJDIR)/w_stat_randoms.o : stable/w_stat_randoms.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_stat_randoms.o stable/w_stat_randoms.cpp
$(STABLE_OBJDIR)/w_stat_table.o : stable/w_stat_table.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_stat_table.o stable/w_stat_table.cpp
$(STABLE_OBJDIR)/w_stat_util_com.o : stable/w_stat_util_com.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_stat_util_com.o stable/w_stat_util_com.cpp
$(STABLE_OBJDIR)/w_streams.o : stable/w_streams.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_streams.o stable/w_streams.cpp
$(STABLE_OBJDIR)/w_string_com.o : stable/w_string_com.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_string_com.o stable/w_string_com.cpp
$(STABLE_OBJDIR)/w_string_$(PROC_NAME).o : stable/w_string_$(PROC_NAME).cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_string_$(PROC_NAME).o stable/w_string_$(PROC_NAME).cpp
$(STABLE_OBJDIR)/w_thread.o : stable/w_thread.cpp $(STABLE_HEADERS) | $(STABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -c -o $(STABLE_OBJDIR)/w_thread.o stable/w_thread.cpp

#TODO

$(BINDIR)/libwhodun.a : \
			$(STABLE_OBJDIR)/w_args.o \
			$(STABLE_OBJDIR)/w_compress.o \
			$(STABLE_OBJDIR)/w_container.o \
			$(STABLE_OBJDIR)/w_ermac.o \
			$(STABLE_OBJDIR)/w_oshook_com.o $(STABLE_OBJDIR)/w_oshook_linux.o \
			$(STABLE_OBJDIR)/w_parse.o \
			$(STABLE_OBJDIR)/w_regex.o \
			$(STABLE_OBJDIR)/w_sort.o \
			$(STABLE_OBJDIR)/w_stat_data.o \
			$(STABLE_OBJDIR)/w_stat_randoms.o \
			$(STABLE_OBJDIR)/w_stat_table.o \
			$(STABLE_OBJDIR)/w_stat_util_com.o \
			$(STABLE_OBJDIR)/w_streams.o \
			$(STABLE_OBJDIR)/w_string_com.o $(STABLE_OBJDIR)/w_string_$(PROC_NAME).o \
			$(STABLE_OBJDIR)/w_thread.o \
			| $(BINDIR)
	ar rcs $(BINDIR)/libwhodun.a $(STABLE_OBJDIR)/*.o

# *******************************************************************
# unstable

UNSTABLE_OBJDIR = $(OBJDIR)/unstable

UNSTABLE_HEADERS = unstable/whodun_cryptdar.h unstable/whodun_gen_graph.h unstable/whodun_gen_graph_align.h unstable/whodun_gen_seqdata.h unstable/whodun_gen_weird.h unstable/whodun_math_matrix.h unstable/whodun_math_optim.h unstable/whodun_stat_dist.h unstable/whodun_suffix.h

$(UNSTABLE_OBJDIR) : 
	mkdir -p $(UNSTABLE_OBJDIR)

$(UNSTABLE_OBJDIR)/w_cryptdar.o : unstable/w_cryptdar.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(UNSTABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(UNSTABLE_OBJDIR)/w_cryptdar.o unstable/w_cryptdar.cpp
$(UNSTABLE_OBJDIR)/w_gen_graph.o : unstable/w_gen_graph.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(UNSTABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(UNSTABLE_OBJDIR)/w_gen_graph.o unstable/w_gen_graph.cpp
$(UNSTABLE_OBJDIR)/w_gen_graph_align.o : unstable/w_gen_graph_align.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(UNSTABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(UNSTABLE_OBJDIR)/w_gen_graph_align.o unstable/w_gen_graph_align.cpp
$(UNSTABLE_OBJDIR)/w_gen_seqdata.o : unstable/w_gen_seqdata.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(UNSTABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(UNSTABLE_OBJDIR)/w_gen_seqdata.o unstable/w_gen_seqdata.cpp
$(UNSTABLE_OBJDIR)/w_gen_weird.o : unstable/w_gen_weird.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(UNSTABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(UNSTABLE_OBJDIR)/w_gen_weird.o unstable/w_gen_weird.cpp
$(UNSTABLE_OBJDIR)/w_math_matrix.o : unstable/w_math_matrix.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(UNSTABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(UNSTABLE_OBJDIR)/w_math_matrix.o unstable/w_math_matrix.cpp
$(UNSTABLE_OBJDIR)/w_math_optim.o : unstable/w_math_optim.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(UNSTABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(UNSTABLE_OBJDIR)/w_math_optim.o unstable/w_math_optim.cpp
$(UNSTABLE_OBJDIR)/w_stat_dist.o : unstable/w_stat_dist.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(UNSTABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(UNSTABLE_OBJDIR)/w_stat_dist.o unstable/w_stat_dist.cpp
$(UNSTABLE_OBJDIR)/w_suffix.o : unstable/w_suffix.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(UNSTABLE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(UNSTABLE_OBJDIR)/w_suffix.o unstable/w_suffix.cpp

#TODO

$(BINDIR)/libwhodunext.a : \
			$(UNSTABLE_OBJDIR)/w_cryptdar.o \
			$(UNSTABLE_OBJDIR)/w_gen_graph.o \
			$(UNSTABLE_OBJDIR)/w_gen_graph_align.o \
			$(UNSTABLE_OBJDIR)/w_gen_seqdata.o \
			$(UNSTABLE_OBJDIR)/w_gen_weird.o \
			$(UNSTABLE_OBJDIR)/w_math_matrix.o \
			$(UNSTABLE_OBJDIR)/w_math_optim.o \
			$(UNSTABLE_OBJDIR)/w_stat_dist.o \
			$(UNSTABLE_OBJDIR)/w_suffix.o \
			| $(BINDIR)
	ar rcs $(BINDIR)/libwhodunext.a $(UNSTABLE_OBJDIR)/*.o

# *******************************************************************
# gpu

GPU_OBJDIR = $(OBJDIR)/gpu

GPU_HEADERS = gpu/whodun_vulkan.h gpu/whodun_vulkan_spirv.h gpu/whodun_vulkan_compute.h

$(GPU_OBJDIR) : 
	mkdir -p $(GPU_OBJDIR)

$(GPU_OBJDIR)/w_vulkan.o : gpu/w_vulkan.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(GPU_HEADERS) | $(GPU_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -c -o $(GPU_OBJDIR)/w_vulkan.o gpu/w_vulkan.cpp
$(GPU_OBJDIR)/w_vulkan_compute.o : gpu/w_vulkan_compute.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(GPU_HEADERS) | $(GPU_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -c -o $(GPU_OBJDIR)/w_vulkan_compute.o gpu/w_vulkan_compute.cpp
$(GPU_OBJDIR)/w_vulkan_spirv.o : gpu/w_vulkan_spirv.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(GPU_HEADERS) | $(GPU_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -c -o $(GPU_OBJDIR)/w_vulkan_spirv.o gpu/w_vulkan_spirv.cpp

#TODO

$(BINDIR)/libwhodungpu.a : \
			$(GPU_OBJDIR)/w_vulkan.o \
			$(GPU_OBJDIR)/w_vulkan_spirv.o \
			$(GPU_OBJDIR)/w_vulkan_compute.o \
			| $(BINDIR)
	ar rcs $(BINDIR)/libwhodungpu.a $(GPU_OBJDIR)/*.o

# *******************************************************************
# experiments

EXP_TEST_OBJDIR = $(OBJDIR)/experiments/test
EXP_TEST_BINDIR = $(BINDIR)/experiments

$(EXP_TEST_OBJDIR) : 
	mkdir -p $(EXP_TEST_OBJDIR)
$(EXP_TEST_BINDIR) : 
	mkdir -p $(EXP_TEST_BINDIR)

$(EXP_TEST_OBJDIR)/main.o : experiments/test/main.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) | $(EXP_TEST_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -c -o $(EXP_TEST_OBJDIR)/main.o experiments/test/main.cpp

$(EXP_TEST_BINDIR)/exp_test : \
			$(EXP_TEST_OBJDIR)/main.o \
			$(BINDIR)/libwhodunext.a \
			$(BINDIR)/libwhodun.a \
			| $(EXP_TEST_BINDIR)
	g++ $(COMP_OPTS) -o $(EXP_TEST_BINDIR)/exp_test -L$(BINDIR) $(EXP_TEST_OBJDIR)/*.o -lwhodunext -lwhodun $(BASIC_PROG_LIBS)



EXP_GENLIKE_OBJDIR = $(OBJDIR)/experiments/2023/genlike
EXP_GENLIKE_BINDIR = $(BINDIR)/experiments/2023

EXP_GENLIKE_HEADERS = experiments/2023/genlike/genlike_data.h experiments/2023/genlike/genlike_progs.h

$(EXP_GENLIKE_OBJDIR) : 
	mkdir -p $(EXP_GENLIKE_OBJDIR)
$(EXP_GENLIKE_BINDIR) : 
	mkdir -p $(EXP_GENLIKE_BINDIR)

$(EXP_GENLIKE_OBJDIR)/main.o : experiments/2023/genlike/main.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(GPU_HEADERS) $(EXP_GENLIKE_HEADERS) | $(EXP_GENLIKE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -c -o $(EXP_GENLIKE_OBJDIR)/main.o experiments/2023/genlike/main.cpp
$(EXP_GENLIKE_OBJDIR)/g_data.o : experiments/2023/genlike/g_data.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(GPU_HEADERS) $(EXP_GENLIKE_HEADERS) | $(EXP_GENLIKE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -c -o $(EXP_GENLIKE_OBJDIR)/g_data.o experiments/2023/genlike/g_data.cpp
$(EXP_GENLIKE_OBJDIR)/g_prototsv.o : experiments/2023/genlike/g_prototsv.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(GPU_HEADERS) $(EXP_GENLIKE_HEADERS) | $(EXP_GENLIKE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -c -o $(EXP_GENLIKE_OBJDIR)/g_prototsv.o experiments/2023/genlike/g_prototsv.cpp
$(EXP_GENLIKE_OBJDIR)/g_readprob.o : experiments/2023/genlike/g_readprob.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(GPU_HEADERS) $(EXP_GENLIKE_HEADERS) | $(EXP_GENLIKE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -c -o $(EXP_GENLIKE_OBJDIR)/g_readprob.o experiments/2023/genlike/g_readprob.cpp
$(EXP_GENLIKE_OBJDIR)/g_sampload.o : experiments/2023/genlike/g_sampload.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(GPU_HEADERS) $(EXP_GENLIKE_HEADERS) | $(EXP_GENLIKE_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -c -o $(EXP_GENLIKE_OBJDIR)/g_sampload.o experiments/2023/genlike/g_sampload.cpp

$(EXP_GENLIKE_BINDIR)/exp_genlike : \
			$(EXP_GENLIKE_OBJDIR)/main.o \
			$(EXP_GENLIKE_OBJDIR)/g_data.o \
			$(EXP_GENLIKE_OBJDIR)/g_prototsv.o \
			$(EXP_GENLIKE_OBJDIR)/g_readprob.o \
			$(EXP_GENLIKE_OBJDIR)/g_sampload.o \
			$(BINDIR)/libwhodunext.a \
			$(BINDIR)/libwhodun.a \
			$(BINDIR)/libwhodungpu.a \
			| $(EXP_GENLIKE_BINDIR)
	g++ $(COMP_OPTS) -o $(EXP_GENLIKE_BINDIR)/exp_genlike -L$(BINDIR) $(EXP_GENLIKE_OBJDIR)/*.o -lwhodungpu -lwhodunext -lwhodun $(GPU_PROG_LIBS) $(BASIC_PROG_LIBS)

#TODO

# *******************************************************************
# programs
# *************************************
# whodun

PROG_WHODUN_OBJDIR = $(OBJDIR)/programs/whodun

PROG_WHODUN_HEADERS = programs/whodun/whodunmain_programs.h

$(PROG_WHODUN_OBJDIR) : 
	mkdir -p $(PROG_WHODUN_OBJDIR)

$(PROG_WHODUN_OBJDIR)/whodunmain.o : programs/whodun/whodunmain.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUN_HEADERS) | $(PROG_WHODUN_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iprograms/whodun -c -o $(PROG_WHODUN_OBJDIR)/whodunmain.o programs/whodun/whodunmain.cpp
$(PROG_WHODUN_OBJDIR)/whodun_main_d2t.o : programs/whodun/whodun_main_d2t.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUN_HEADERS) | $(PROG_WHODUN_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iprograms/whodun -c -o $(PROG_WHODUN_OBJDIR)/whodun_main_d2t.o programs/whodun/whodun_main_d2t.cpp
$(PROG_WHODUN_OBJDIR)/whodun_main_dconv.o : programs/whodun/whodun_main_dconv.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUN_HEADERS) | $(PROG_WHODUN_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iprograms/whodun -c -o $(PROG_WHODUN_OBJDIR)/whodun_main_dconv.o programs/whodun/whodun_main_dconv.cpp
$(PROG_WHODUN_OBJDIR)/whodun_main_t2d.o : programs/whodun/whodun_main_t2d.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUN_HEADERS) | $(PROG_WHODUN_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iprograms/whodun -c -o $(PROG_WHODUN_OBJDIR)/whodun_main_t2d.o programs/whodun/whodun_main_t2d.cpp
$(PROG_WHODUN_OBJDIR)/whodun_main_tconv.o : programs/whodun/whodun_main_tconv.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUN_HEADERS) | $(PROG_WHODUN_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iprograms/whodun -c -o $(PROG_WHODUN_OBJDIR)/whodun_main_tconv.o programs/whodun/whodun_main_tconv.cpp

#TODO

$(BINDIR)/whodun : \
			$(PROG_WHODUN_OBJDIR)/whodunmain.o \
			$(PROG_WHODUN_OBJDIR)/whodun_main_d2t.o \
			$(PROG_WHODUN_OBJDIR)/whodun_main_dconv.o \
			$(PROG_WHODUN_OBJDIR)/whodun_main_t2d.o \
			$(PROG_WHODUN_OBJDIR)/whodun_main_tconv.o \
			$(BINDIR)/libwhodun.a \
			| $(BINDIR)
	g++ $(COMP_OPTS) -o $(BINDIR)/whodun -L$(BINDIR) $(PROG_WHODUN_OBJDIR)/*.o -lwhodun $(BASIC_PROG_LIBS)

# *************************************
# whodunu

PROG_WHODUNU_OBJDIR = $(OBJDIR)/programs/whodunu

PROG_WHODUNU_HEADERS = programs/whodunu/whodunumain_programs.h

$(PROG_WHODUNU_OBJDIR) : 
	mkdir -p $(PROG_WHODUNU_OBJDIR)

$(PROG_WHODUNU_OBJDIR)/whodunumain.o : programs/whodunu/whodunumain.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUNU_HEADERS) | $(PROG_WHODUNU_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Iprograms/whodunu -c -o $(PROG_WHODUNU_OBJDIR)/whodunumain.o programs/whodunu/whodunumain.cpp
$(PROG_WHODUNU_OBJDIR)/whodun_main_fqconv.o : programs/whodunu/whodun_main_fqconv.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUNU_HEADERS) | $(PROG_WHODUNU_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Iprograms/whodunu -c -o $(PROG_WHODUNU_OBJDIR)/whodun_main_fqconv.o programs/whodunu/whodun_main_fqconv.cpp
$(PROG_WHODUNU_OBJDIR)/whodun_main_fq2sg.o : programs/whodunu/whodun_main_fq2sg.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUNU_HEADERS) | $(PROG_WHODUNU_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Iprograms/whodunu -c -o $(PROG_WHODUNU_OBJDIR)/whodun_main_fq2sg.o programs/whodunu/whodun_main_fq2sg.cpp
$(PROG_WHODUNU_OBJDIR)/whodun_main_sconv.o : programs/whodunu/whodun_main_sconv.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUNU_HEADERS) | $(PROG_WHODUNU_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Iprograms/whodunu -c -o $(PROG_WHODUNU_OBJDIR)/whodun_main_sconv.o programs/whodunu/whodun_main_sconv.cpp
$(PROG_WHODUNU_OBJDIR)/whodun_main_s2sg.o : programs/whodunu/whodun_main_s2sg.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUNU_HEADERS) | $(PROG_WHODUNU_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Iprograms/whodunu -c -o $(PROG_WHODUNU_OBJDIR)/whodun_main_s2sg.o programs/whodunu/whodun_main_s2sg.cpp

#TODO

$(BINDIR)/whodunu : \
			$(PROG_WHODUNU_OBJDIR)/whodunumain.o \
			$(PROG_WHODUNU_OBJDIR)/whodun_main_fqconv.o \
			$(PROG_WHODUNU_OBJDIR)/whodun_main_fq2sg.o \
			$(PROG_WHODUNU_OBJDIR)/whodun_main_sconv.o \
			$(PROG_WHODUNU_OBJDIR)/whodun_main_s2sg.o \
			$(BINDIR)/libwhodunext.a \
			$(BINDIR)/libwhodun.a \
			| $(BINDIR)
	g++ $(COMP_OPTS) -o $(BINDIR)/whodunu -L$(BINDIR) $(PROG_WHODUNU_OBJDIR)/*.o -lwhodunext -lwhodun $(BASIC_PROG_LIBS)

# *************************************
# whodung

PROG_WHODUNG_OBJDIR = $(OBJDIR)/programs/whodung

PROG_WHODUNG_HEADERS = programs/whodung/whodungmain_programs.h

$(PROG_WHODUNG_OBJDIR) : 
	mkdir -p $(PROG_WHODUNG_OBJDIR)

$(PROG_WHODUNG_OBJDIR)/whodungmain.o : programs/whodung/whodungmain.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUNG_HEADERS) | $(PROG_WHODUNG_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -Iprograms/whodung -c -o $(PROG_WHODUNG_OBJDIR)/whodungmain.o programs/whodung/whodungmain.cpp
$(PROG_WHODUNG_OBJDIR)/whodun_main_gpuinfo.o : programs/whodung/whodun_main_gpuinfo.cpp $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(PROG_WHODUNG_HEADERS) | $(PROG_WHODUNG_OBJDIR)
	g++ $(COMP_OPTS) -Istable -Iunstable -Igpu -Iprograms/whodung -c -o $(PROG_WHODUNG_OBJDIR)/whodun_main_gpuinfo.o programs/whodung/whodun_main_gpuinfo.cpp

#TODO

$(BINDIR)/whodung : \
			$(PROG_WHODUNG_OBJDIR)/whodungmain.o \
			$(PROG_WHODUNG_OBJDIR)/whodun_main_gpuinfo.o \
			$(BINDIR)/libwhodungpu.a \
			$(BINDIR)/libwhodunext.a \
			$(BINDIR)/libwhodun.a \
			| $(BINDIR)
	g++ $(COMP_OPTS) -o $(BINDIR)/whodung -L$(BINDIR) $(PROG_WHODUNG_OBJDIR)/*.o -lwhodungpu -lwhodunext -lwhodun $(BASIC_PROG_LIBS) $(GPU_PROG_LIBS)

# *******************************************************************
# documentation

doc/html/index.html : $(STABLE_HEADERS) $(UNSTABLE_HEADERS) $(GPU_HEADERS)
	doxygen Doxyfile > /dev/null

doc/whodun/main.html : $(BINDIR)/whodun
	mkdir -p doc/whodun/
	python3 tools/ArgMang.py htmls $(BINDIR)/whodun > doc/whodun/main.html
	python3 tools/ArgMang.py mans $(BINDIR)/whodun ./doc/whodun/ whodun

doc/whodunu/main.html : $(BINDIR)/whodunu
	mkdir -p doc/whodunu/
	python3 tools/ArgMang.py htmls $(BINDIR)/whodunu > doc/whodunu/main.html
	python3 tools/ArgMang.py mans $(BINDIR)/whodunu ./doc/whodunu/ whodunu

doc/whodung/main.html : $(BINDIR)/whodung
	mkdir -p doc/whodung/
	python3 tools/ArgMang.py htmls $(BINDIR)/whodung > doc/whodung/main.html
	python3 tools/ArgMang.py mans $(BINDIR)/whodung ./doc/whodung/ whodung



