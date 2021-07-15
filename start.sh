#!/bin/bash

./build/ChatServer/ChatServer -d
sleep 1
./build/FileServer/FileServer -d
sleep 1
./build/ImgServer/ImgServer -d
