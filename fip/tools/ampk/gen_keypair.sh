#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: $0 vendor_priv.pem vendor_pub.pem"
    exit 1
fi

priv=$1 #user_priv.pem
pub=$2  #user_pub.pem

if [ -f "$priv" ]; then
    echo "Error: Private key $priv already exists"
    exit 1
fi
if [ -f "$pub" ]; then
    echo "Error: Public key $pub already exists"
    exit 1
fi

openssl ecparam -name prime256v1 -genkey -noout -out "$priv"
openssl ec -in "$priv"  -pubout -out "$pub"

