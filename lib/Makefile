TARGET= \
	inih \
	death_handler

.PHONY: all $(TARGET)

all: $(TARGET)
	@for dir in $(TARGET); do \
		$(MAKE) -C $$dir; \
	done

clean:
	@for dir in $(TARGET); do \
		$(MAKE) clean -C $$dir; \
	done
