CC=g++
CFLAGS=-std=c++11 -Wall -O2
MEMEC_SRC_ROOT=..
LIBS=-pthread
BIN=$(MEMEC_SRC_ROOT)/bin/client

OBJS= \
	backup/backup.o \
	config/client_config.o \
	ds/pending.o \
	event/application_event.o \
	event/coordinator_event.o \
	event/client_event.o \
	event/server_event.o \
	protocol/degraded_protocol.o \
	protocol/fault_protocol.o \
	protocol/load_protocol.o \
	protocol/normal_protocol.o \
	protocol/register_protocol.o \
	protocol/remap_protocol.o \
	protocol/replay_protocol.o \
	remap/basic_remap_scheme.o \
	remap/remap_msg_handler.o \
	socket/application_socket.o \
	socket/coordinator_socket.o \
	socket/client_socket.o \
	socket/server_socket.o \
	worker/worker.o \
	worker/application_worker.o \
	worker/coordinator_worker.o \
	worker/degraded_worker.o \
	worker/recovery_worker.o \
	worker/remap_worker.o \
	worker/server_worker.o \
	main/client.o \
	main/main.o

EXTERNAL_LIB= \
	$(MEMEC_SRC_ROOT)/common/config/config.o \
	$(MEMEC_SRC_ROOT)/common/config/global_config.o \
	$(MEMEC_SRC_ROOT)/common/config/server_addr.o \
	$(MEMEC_SRC_ROOT)/common/ds/bitmask_array.o \
	$(MEMEC_SRC_ROOT)/common/ds/instance_id_generator.o \
	$(MEMEC_SRC_ROOT)/common/ds/key_value.o \
	$(MEMEC_SRC_ROOT)/common/ds/latency.o \
	$(MEMEC_SRC_ROOT)/common/ds/load.o \
	$(MEMEC_SRC_ROOT)/common/ds/sockaddr_in.o \
	$(MEMEC_SRC_ROOT)/common/protocol/protocol.o \
	$(MEMEC_SRC_ROOT)/common/protocol/ack_protocol.o \
	$(MEMEC_SRC_ROOT)/common/protocol/address_protocol.o \
	$(MEMEC_SRC_ROOT)/common/protocol/degraded_protocol.o \
	$(MEMEC_SRC_ROOT)/common/protocol/fault_protocol.o \
	$(MEMEC_SRC_ROOT)/common/protocol/heartbeat_protocol.o \
	$(MEMEC_SRC_ROOT)/common/protocol/load_protocol.o \
	$(MEMEC_SRC_ROOT)/common/protocol/normal_protocol.o \
	$(MEMEC_SRC_ROOT)/common/protocol/remap_protocol.o \
	$(MEMEC_SRC_ROOT)/common/remap/remap_msg_handler.o \
	$(MEMEC_SRC_ROOT)/common/socket/socket.o \
	$(MEMEC_SRC_ROOT)/common/socket/epoll.o \
	$(MEMEC_SRC_ROOT)/lib/death_handler/death_handler.o \
	$(MEMEC_SRC_ROOT)/lib/inih/ini.o

LD_LIB= \
	-lrt \
	-ldl \
	-ltspread

.PHONY: clean

$(BIN): $(OBJS) $(EXTERNAL_LIB)
	$(CC) $(CFLAGS) $(LIBS) -o $(BIN) $(OBJS) $(EXTERNAL_LIB) $(LD_LIB)

%.o: %.cc %.hh
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -c -o $@ $<

%.o: %.cc
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(BIN)