WINDRES ?= windres

monitor: monitor.c monitor_res.o
	$(CC) -O2 -s -o $@ $^ -ldwmapi

monitor_res.o: monitor_res.rc monitor.manifest
	$(WINDRES) -i $< -o $@

clean:
	$(RM) monitor.exe monitor_res.o

.PHONY: clean