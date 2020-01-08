#! /bin/bash
g++  -o serverbin  main.cpp client_connection.cpp server.cpp -l pthread \
&& ./serverbin

