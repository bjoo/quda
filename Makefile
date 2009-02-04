CUFILES		:= dslash_cuda.cu blas_cuda.cu
CCFILES		:= inv_bicgstab_cuda.cpp inv_cg.cpp util_cuda.cpp field_cuda.cpp

CUDA_INSTALL_PATH = /usr/local/cuda
INCLUDES = -I. -I$(CUDA_INSTALL_PATH)/include
LIB = -L$(CUDA_INSTALL_PATH)/lib -lcudart 

CC = gcc
CFLAGS = -Wall -std=c99 $(INCLUDES) -O3 #-D__DEVICE_EMULATION__
CXX = g++
CXXFLAGS = -Wall $(INCLUDES) -DUNIX -O3 #-D__DEVICE_EMULATION__
NVCC = $(CUDA_INSTALL_PATH)/bin/nvcc 
NVCCFLAGS = $(INCLUDES) -DUNIX -arch=sm_13
LDFLAGS = -fPIC $(LIB) -pg
CCOBJECTS = $(CCFILES:.cpp=.o)
CUOBJECTS = $(CUFILES:.cu=.o)

all: dslash_test invert_test su3_test pack_test

ILIB = libquda.a
ILIB_OBJS = inv_bicgstab_quda.o inv_cg_quda.o dslash_quda.o blas_quda.o util_quda.o \
	dslash_reference.o blas_reference.o invert_quda.o field_quda.o
ILIB_DEPS = $(ILIB_OBJS) blas_quda.h quda.h util_quda.h invert_quda.h field_quda.h enum_quda.h

$(ILIB): $(ILIB_DEPS)
	ar cru $@ $(ILIB_OBJS)

invert_test: invert_test.o $(ILIB)
	$(CXX) $(LDFLAGS) $< $(ILIB) -o $@

dslash_test: dslash_test.o $(ILIB)
	$(CXX) $(LDFLAGS) $< $(ILIB) -o $@

su3_test: su3_test.o $(ILIB)
	$(CXX) $(LDFLAGS) $< $(ILIB) -o $@

pack_test: pack_test.o $(ILIB)
	$(CXX) $(LDFLAGS) $< $(ILIB) -o $@

clean:
	-rm -f *.o dslash_test invert_test su3_test pack_test $(ILIB)

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

%.o: %.cu
	$(NVCC) $(NVCCFLAGS) $< -c -o $@
