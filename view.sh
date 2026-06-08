#!/bin/sh
llvm-as < view.ll | opt -passes=view-cfg