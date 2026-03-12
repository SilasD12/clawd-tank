# firmware/test/Makefile
CC = gcc
CFLAGS = -Wall -Wextra -I../main -Istubs

test: test_notification test_config_store
	./test_notification
	./test_config_store

test_notification: test_notification.c ../main/notification.c
	$(CC) $(CFLAGS) -o $@ $^

test_config_store: test_config_store.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f test_notification test_config_store

.PHONY: test clean
