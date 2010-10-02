#!/bin/bash
echo 'Content-type: text/plain'
echo ''
curl -X POST http://127.0.0.1:8888 -d $QUERY_STRING


