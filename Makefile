.PHONY: run
run: circd
	$(info :Running)
	./circd

circd: $(wildcard src/*.[ch])
	$(info :Compiling)
	clang -o $@ -Wall -Wextra -Wpedantic -Werror -O0 -g $(filter %.c, $^)

.PHONY: clean
clean:
	$(info :Cleaning)
	rm circd