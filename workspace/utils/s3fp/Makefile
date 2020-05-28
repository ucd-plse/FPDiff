CC = g++

BOOST_INC = -I /home/wfchiang/lib/boost_1_54_0
QD_HOME=/home/wfchiang/lib/qd-2.3.14/install

CORE_CFLAG = -std=c++0x -g
S3FP_EXE = s3fp
S3FP_EXE_DD = s3fp-dd


all: core-fp128

core-fp128: parser 
	$(CC) $(CORE_CFLAG) $(BOOST_INC) -c S3FP_Core.cpp
	$(CC) -o $(S3FP_EXE) S3FP_ParseArgs.o S3FP_Core.o -lpthread -lquadmath

core-dd: parser
	$(CC) $(CORE_CFLAG) $(BOOST_INC) -I $(QD_HOME)/include -DS3FP_DDIO -c S3FP_Core.cpp
	$(CC) -I $(QD_HOME)/include S3FP_ParseArgs.o S3FP_Core.o $(QD_HOME)/lib/libqd.a -o $(S3FP_EXE_DD) -lpthread -lquadmath

parser: 
	$(CC) -c S3FP_ParseArgs.cpp 

clean:
	-rm $(S3FP_EXE) $(S3FP_EXE_DD) *.o 
