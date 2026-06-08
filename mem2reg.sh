#!/bin/sh
llvm-as < mem2reg.ll | opt -passes=mem2reg | llvm-dis