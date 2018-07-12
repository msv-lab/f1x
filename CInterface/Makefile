CFLAGS=-L../build2/repair -lf1xRepair -L../build2/tools -lF1X -lrt -lboost_program_options -lboost_log -lboost_log_setup -lboost_date_time -lboost_system -lboost_filesystem -lboost_thread -lboost_chrono -lboost_atomic -lpthread

main: main.c
	gcc -o main main.c $(CFLAGS)

f1x-aflgo: f1x-aflgo.c
	gcc -o f1x-aflgo f1x-aflgo.c $(CFLAGS)
