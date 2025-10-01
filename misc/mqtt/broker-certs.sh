#!/bin/sh

SERVER_HOST=127.0.0.1

CA_SUBJ="/C=RU/ST=Mordovia/L=Saransk/O=MRSU/OU=Math/CN=tim.local"
SERVER_SUBJ="/C=RU/ST=Mordovia/L=Saransk/O=MRSU/OU=Math/CN=$SERVER_HOST"
CLIENT_SUBJ="/C=RU/ST=Mordovia/L=Saransk/O=MRSU/OU=Math/CN=tim-client.local"

MOSQUITTO_TLS_DIR=/etc/tim/tls
TIM_TLS_DIR=~/.config/mrsu13/tim/tls

MOSQUITTO_USER=mosquitto

# CA
openssl req -new -x509 -days 365 -extensions v3_ca -keyout ca.key -out ca.crt -subj "$CA_SUBJ"

# Mosquitto
openssl genrsa -out server.key 2048
openssl req -new -out server.csr -key server.key -subj "$SERVER_SUBJ"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 3650
openssl rsa -in server.key -out server.key

# TIM Server
openssl genrsa -out client.key 2048
openssl req -new -out client.csr -key client.key -subj "$CLIENT_SUBJ"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 3650
openssl rsa -in client.key -out client.key

# TIM Server
rm -rf "$TIM_TLS_DIR"
mkdir -p "$TIM_TLS_DIR"
cp ca.crt "$TIM_TLS_DIR/"
mv client.* "$TIM_TLS_DIR/"

if [ -d "$MOSQUITTO_TLS_DIR" ]; then
    sudo rm -rf "$MOSQUITTO_TLS_DIR"
fi

if [ ! -d "$MOSQUITTO_TLS_DIR" ]; then
    sudo mkdir -p "$MOSQUITTO_TLS_DIR"
fi

sudo rm -f "$MOSQUITTO_TLS_DIR/*"
sudo mv ca.* server.* "$MOSQUITTO_TLS_DIR/"
sudo openssl rehash "$MOSQUITTO_TLS_DIR"
sudo chown -R $MOSQUITTO_USER:$MOSQUITTO_USER "$MOSQUITTO_TLS_DIR"

sudo cp tim.conf /etc/mosquitto/conf.d/

sudo systemctl restart mosquitto
systemctl status mosquitto
