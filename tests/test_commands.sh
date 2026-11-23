#!/bin/bash

echo "Running Mini Redis Tests..."

response=$(printf '*1\r\n$4\r\nPING\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == "+PONG" ]; then
    echo "PASS: PING command"
else
    echo "FAIL: PING command"
    echo "Expected: +PONG"
    echo "Received: $response"
fi
# SET Command Test

response=$(printf '*3\r\n$3\r\nSET\r\n$7\r\ntestkey\r\n$5\r\nAnmol\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == "+OK" ]; then
    echo "PASS: SET command"
else
    echo "FAIL: SET command"
    echo "Received: $response"
fi
# GET Command Test

response=$(printf '*2\r\n$3\r\nGET\r\n$7\r\ntestkey\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

expected=$(printf '$5\nAnmol')

if [[ "$response" == "$expected" ]]; then
    echo "PASS: GET command"
else
    echo "FAIL: GET command"
    echo "Received: $response"
fi
# DEL Command Test

response=$(printf '*2\r\n$3\r\nDEL\r\n$7\r\ntestkey\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":1" ]; then
    echo "PASS: DEL command"
else
    echo "FAIL: DEL command"
    echo "Received: $response"
fi
# EXPIRE Command Test

printf '*3\r\n$3\r\nSET\r\n$4\r\ntemp\r\n$5\r\nHello\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*3\r\n$6\r\nEXPIRE\r\n$4\r\ntemp\r\n$2\r\n60\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":1" ]; then
    echo "PASS: EXPIRE command"
else
    echo "FAIL: EXPIRE command"
    echo "Received: $response"
fi
# TTL Command Test

response=$(printf '*2\r\n$3\r\nTTL\r\n$4\r\ntemp\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [[ "$response" =~ ^:[0-5][0-9]$ ]]; then
    echo "PASS: TTL command"
else
    echo "FAIL: TTL command"
    echo "Received: $response"
fi
# EXISTS Command Test

response=$(printf '*2\r\n$6\r\nEXISTS\r\n$4\r\ntemp\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":1" ]; then
    echo "PASS: EXISTS command"
else
    echo "FAIL: EXISTS command"
    echo "Received: $response"
fi
# DBSIZE Command Test

response=$(printf '*1\r\n$6\r\nDBSIZE\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [[ "$response" =~ ^:[0-9]+$ ]]; then
    echo "PASS: DBSIZE command"
else
    echo "FAIL: DBSIZE command"
    echo "Received: $response"
fi
# MGET Command Test

printf '*5\r\n$4\r\nMSET\r\n$4\r\nkey1\r\n$5\r\nAnmol\r\n$4\r\nkey2\r\n$7\r\nGwalior\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*3\r\n$4\r\nMGET\r\n$4\r\nkey1\r\n$4\r\nkey2\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

expected=$(printf '*2\n$5\nAnmol\n$7\nGwalior')

if [[ "$response" == "$expected" ]]; then
    echo "PASS: MGET command"
else
    echo "FAIL: MGET command"
    echo "Received: $response"
fi
# INCR Command Test

printf '*3\r\n$3\r\nSET\r\n$7\r\ncounter\r\n$2\r\n10\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*2\r\n$4\r\nINCR\r\n$7\r\ncounter\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":11" ]; then
    echo "PASS: INCR command"
else
    echo "FAIL: INCR command"
    echo "Received: $response"
fi
# DECR Command Test

printf '*3\r\n$3\r\nSET\r\n$7\r\ncounter2\r\n$2\r\n10\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*2\r\n$4\r\nDECR\r\n$7\r\ncounter2\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":9" ]; then
    echo "PASS: DECR command"
else
    echo "FAIL: DECR command"
    echo "Received: $response"
fi
# INCRBY Command Test

printf '*3\r\n$3\r\nSET\r\n$8\r\ncounter3\r\n$2\r\n10\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*3\r\n$6\r\nINCRBY\r\n$8\r\ncounter3\r\n$1\r\n5\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":15" ]; then
    echo "PASS: INCRBY command"
else
    echo "FAIL: INCRBY command"
    echo "Received: $response"
fi
# DECRBY Command Test

printf '*3\r\n$3\r\nSET\r\n$8\r\ncounter4\r\n$2\r\n20\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*3\r\n$6\r\nDECRBY\r\n$8\r\ncounter4\r\n$1\r\n5\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":15" ]; then
    echo "PASS: DECRBY command"
else
    echo "FAIL: DECRBY command - $response"
fi


# STRLEN Command Test

printf '*3\r\n$3\r\nSET\r\n$7\r\ntestkey\r\n$5\r\nAnmol\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*2\r\n$6\r\nSTRLEN\r\n$7\r\ntestkey\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":5" ]; then
    echo "PASS: STRLEN command"
else
    echo "FAIL: STRLEN command - $response"
fi


# APPEND Command Test

printf '*3\r\n$3\r\nSET\r\n$7\r\nappend1\r\n$5\r\nHello\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*3\r\n$6\r\nAPPEND\r\n$7\r\nappend1\r\n$5\r\nWorld\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":10" ]; then
    echo "PASS: APPEND command"
else
    echo "FAIL: APPEND command - $response"
fi


# TYPE Command Test

response=$(printf '*2\r\n$4\r\nTYPE\r\n$7\r\nappend1\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == "+string" ]; then
    echo "PASS: TYPE command"
else
    echo "FAIL: TYPE command - $response"
fi


# RENAME Command Test

printf '*3\r\n$3\r\nSET\r\n$7\r\noldname\r\n$5\r\nAnmol\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*3\r\n$6\r\nRENAME\r\n$7\r\noldname\r\n$7\r\nnewname\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == "+OK" ]; then
    echo "PASS: RENAME command"
else
    echo "FAIL: RENAME command - $response"
fi


# RENAMENX Command Test

printf '*3\r\n$3\r\nSET\r\n$5\r\nfirst\r\n$3\r\nOne\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*3\r\n$7\r\nRENAMENX\r\n$5\r\nfirst\r\n$6\r\nsecond\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":1" ]; then
    echo "PASS: RENAMENX command"
else
    echo "FAIL: RENAMENX command - $response"
fi


# PERSIST Command Test

printf '*3\r\n$3\r\nSET\r\n$5\r\npersist\r\n$5\r\nHello\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

printf '*3\r\n$6\r\nEXPIRE\r\n$5\r\npersist\r\n$2\r\n60\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*2\r\n$7\r\nPERSIST\r\n$5\r\npersist\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":1" ]; then
    echo "PASS: PERSIST command"
else
    echo "FAIL: PERSIST command - $response"
fi


# GETSET Command Test

printf '*3\r\n$3\r\nSET\r\n$5\r\ngetst\r\n$5\r\nHello\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*3\r\n$6\r\nGETSET\r\n$5\r\ngetst\r\n$5\r\nWorld\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

expected=$(printf '$5\nHello')

if [[ "$response" == "$expected" ]]; then
    echo "PASS: GETSET command"
else
    echo "FAIL: GETSET command"
fi


# SETNX Command Test

response=$(printf '*3\r\n$5\r\nSETNX\r\n$5\r\nnxkey\r\n$3\r\nOne\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":1" ]; then
    echo "PASS: SETNX command"
else
    echo "FAIL: SETNX command - $response"
fi


# GETDEL Command Test

printf '*3\r\n$3\r\nSET\r\n$6\r\ndelkey\r\n$5\r\nHello\r\n' | nc -w 2 127.0.0.1 6379 > /dev/null

response=$(printf '*2\r\n$6\r\nGETDEL\r\n$6\r\ndelkey\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

expected=$(printf '$5\nHello')

if [[ "$response" == "$expected" ]]; then
    echo "PASS: GETDEL command"
else
    echo "FAIL: GETDEL command"
fi


# FLUSHALL Command Test

response=$(printf '*1\r\n$8\r\nFLUSHALL\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == "+OK" ]; then
    echo "PASS: FLUSHALL command"
else
    echo "FAIL: FLUSHALL command - $response"
fi


# DBSIZE After FLUSHALL Test

response=$(printf '*1\r\n$6\r\nDBSIZE\r\n' | nc -w 2 127.0.0.1 6379 | tr -d '\r')

if [ "$response" == ":0" ]; then
    echo "PASS: DBSIZE after FLUSHALL"
else
    echo "FAIL: DBSIZE after FLUSHALL - $response"
fi