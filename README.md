# CDA5106-Project
Repository for Spring 2023 - CDA 5106 Group 11 Project

Type "make" to build. Type "make clean" to remove simulator.

In simulator.h, change OPT to 1 to get improvement changes to the code. Change DEBUG to 1 for debug print outs.

Test Validation Runs:

Trace #0: ./sim_cache 16 1024 2 0 0 0 0 traces/gcc_trace.txt  
Trace #1: ./sim_cache 16 1024 1 0 0 0 0 traces/perl_trace.txt  
Trace #2: ./sim_cache 16 1024 1 0 0 0 0 traces/perl_trace.txt  
Trace #3: ./sim_cache 16 1024 2 0 0 1 0 traces/gcc_trace.txt  
Trace #4: ./sim_cache 16 1024 2 0 0 2 0 traces/vortex_trace.txt  
Trace #5: ./sim_cache 16 1024 2 8192 4 0 0 traces/gcc_trace.txt  
Trace #6: ./sim_cache 16 1024 1 8192 4 0 0 traces/go_trace.txt  
Trace #7: ./sim_cache 16 1024 2 8192 4 0 1 traces/gcc_trace.txt  
Trace #8: ./sim_cache 16 1024 1 8192 4 0 1 traces/compress_trace.txt  
