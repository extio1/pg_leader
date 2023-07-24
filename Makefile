MODULE_big = pg_leader
OBJS = src/routines.o src/leader_main.o src/config_parser.o src/error.o log/logger.o src/sql.o

EXTENSION = pg_leader
DATA = sql/pg_leader--1.0.sql

PG_CFLAGS=-Wall `pkg-config --cflags glib-2.0` `pkg-config --libs glib-2.0`

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

#КУДА ПОМЕСТИТЬ КОНФИГУРАЦИОННЫЕ ФАЙЛЫ АВТОМАТИЧЕСКИ??
