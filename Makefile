circd: $(wildcard src/*.[ch])
	$(info :Compiling)
	clang -o $@ \
		-lpthread \
		-Wall -Wextra -Wpedantic -O0 -g \
		$(filter %.c, $^)

.PHONY: run
run: circd
	$(info :Running)
	./circd

.PHONY: clean
clean:
	$(info :Cleaning)
	test -f circd && rm circd