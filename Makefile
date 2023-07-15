MODULE_big = pgha
OBJS = src/routines.o src/ha_main.o src/config_parser.o src/glib_hashtable_wrapper.o

EXTENSION = pgha
DATA = sql/pgha--1.0.sql

PG_CFLAGS=-Wall `pkg-config --cflags glib-2.0`

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


#??как указать зависимость от внешней библиотеки glib??
