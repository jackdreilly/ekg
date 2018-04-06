#!/bin/sh

docker login

docker build -t tmp .

docker push tmp jackdreilly/ekg
