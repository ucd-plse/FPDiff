CC=gcc
CPP=g++

all: interval best input_to_errors random_input_generator errors_stream_to_matlab_data

interval: interval_test.cpp
	$(CPP) -o interval_test interval_test.cpp -lquadmath

best: show_best.c
	$(CC) -o show_best show_best.c

input_to_errors: show_input_to_errors.cpp 
	$(CPP) -o show_input_to_errors show_input_to_errors.cpp 

random_input_generator:
	$(CPP) -o random_input_generator random_input_generator.cpp

errors_stream_to_matlab_data:
	$(CPP) -o errors_stream_to_matlab_data errors_stream_to_matlab_data.cpp 

clean-tmp:
	-rm *~

clean: clean-tmp
	-rm random_input_generator
	-rm errors_stream_to_matlab_data
	-rm show_best
	-rm show_input_to_errors 
	-rm interval_test

