#!/bin/bash
echo "build proto"
filepath=$(cd "$(dirname "$0")"; pwd)
mkdir -p ./proto
protoc -I=. --cpp_out=./proto *.proto

mkdir -p ../masterServer/MasterServer/proto
cp -rf proto/*  ../masterServer/MasterServer/proto

