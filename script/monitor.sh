#!/bin/bash

source ../config/pg_leader.config

lines=$(tput lines)
cols=$(tput cols)

echo $lines $cols

tmux split-windown -h -l 