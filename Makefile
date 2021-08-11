.PHONY: all check clean fmt test

all: check test fmt

check:
	esy dune build @check

clean:
	esy dune clean

fmt:
	esy dune build @fmt --auto-promote

test:
	esy dune runtest --auto-promote
