#!/usr/bin/env bash

cd src
eosio-cpp auditor.cpp -o ../auditor.wasm -abigen -I ../include -I "."
