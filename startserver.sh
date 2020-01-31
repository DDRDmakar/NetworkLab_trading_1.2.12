#! /bin/bash
g++  -o serverbin  main.cpp client_connection.cpp server.cpp tools.cpp -l pthread \
&& ./serverbin

