#!/bin/bash
echo "build proto"
filepath=$(cd "$(dirname "$0")"; pwd)
protoc -I=. --cpp_out=. ConnectMsg.proto