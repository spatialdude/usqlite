#!/bin/sh

CONFIG=../../../micropython/tools/uncrustify.cfg
uncrustify -c ${CONFIG} --no-backup --replace ../*.c ../*.h
