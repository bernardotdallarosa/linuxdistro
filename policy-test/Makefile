BUILDROOT_DIR := ..
COMPILER := $(BUILDROOT_DIR)/output/host/bin/i686-buildroot-linux-gnu-gcc

all:
	$(COMPILER) -o setpriority setpriority.c -lpthread -Wall
	$(COMPILER) -o deadline deadline.c -lpthread -Wall
	$(COMPILER) -o sched_probe sched_probe.c -lpthread -Wall
	$(COMPILER) -o sched_profiler sched_profiler.c -lpthread -Wall
	$(COMPILER) -o sched_profiler_t3 sched_profiler_t3.c -lpthread -Wall
	cp setpriority $(BUILDROOT_DIR)/output/target/bin
	cp deadline $(BUILDROOT_DIR)/output/target/bin
	cp sched_probe $(BUILDROOT_DIR)/output/target/bin
	cp sched_profiler $(BUILDROOT_DIR)/output/target/bin
	cp sched_profiler_t3 $(BUILDROOT_DIR)/output/target/bin


clean:
	rm setpriority deadline sched_probe sched_profiler sched_profiler_t3