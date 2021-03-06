TARGET=lib common coordinator client server application
MAKEOPT= -j8 # speed up compiile
USE_ISAL= 0 # change to 1 for compiling with ISAL instead of Jearasure

.PHONY: all $(TARGET)

all: $(TARGET)
	@for dir in $(TARGET); do \
		echo "\033[1;33m>> Compiling binaries for '$$dir'...\033[0;0m"; \
		if $(MAKE) $(MAKEOPT) USE_ISAL=$(USE_ISAL) -C $$dir; then \
			echo "\033[1;32mSuccess\033[0;0m\n"; \
		else \
			echo "\033[1;31mFail\n\n** Compilation terminated. **\033[0;0m\n"; \
			break; \
		fi; \
	done

clean:
	@for dir in $(TARGET); do \
		echo "\033[1;33m>> Cleaning binaries for '$$dir'...\033[0;0m"; \
		if $(MAKE) clean -C $$dir; then \
			echo "\033[1;32mSuccess\033[0;0m\n"; \
		else \
			echo "\033[1;31mFail\033[0;0m\n"; \
		fi; \
	done

reset:
	rm -f data/*/*
