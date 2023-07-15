MODULE_big = pgha
OBJS = ../src/routines.o ../src/ha_main.o

EXTENSION = pgha
DATA = pgha--1.0.sql

#PG_CFLAGS=-Wall -I/usr/include/ntirpc/

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


#??как указать зависимость от внешней библиотеки rpc??
