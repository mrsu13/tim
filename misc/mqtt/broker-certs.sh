#!/bin/sh

SERVER_HOST=127.0.0.1

CA_SUBJ="/C=RU/ST=Mordovia/L=Saransk/O=MRSU/OU=Math/CN=tim.local"
SERVER_SUBJ="/C=RU/ST=Mordovia/L=Saransk/O=MRSU/OU=Math/CN=$SERVER_HOST"
CLIENT_SUBJ="/C=RU/ST=Mordovia/L=Saransk/O=MRSU/OU=Math/CN=tim-client.local"

MOSQUITTO_TLS_DIR=/etc/tim/tls
TIM_TLS_DIR=~/.config/mrsu13/tim/tls

MOSQUITTO_USER=mosquitto

# CA
openssl req -new -x509 -days 3650 -extensions v3_ca -keyout ca-key.pem -out ca-cert.pem -subj "$CA_SUBJ"

new_cert()
{
    openssl genrsa -out key.pem 2048
    openssl req -new -out tmp.csr -key key.pem -subj $@
    openssl x509 -req -in tmp.csr -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out cert.pem -days 3650
    openssl rsa -in key.pem -out key.pem
}

clear_cert()
{
    rm -f cert.pem key.pem tmp.csr
}

# Mosquitto
new_cert "$SERVER_SUBJ"

if [ -d "$MOSQUITTO_TLS_DIR" ]; then
    sudo rm -rf "$MOSQUITTO_TLS_DIR"
fi

if [ ! -d "$MOSQUITTO_TLS_DIR" ]; then
    sudo mkdir -p "$MOSQUITTO_TLS_DIR"
fi

sudo rm -f "$MOSQUITTO_TLS_DIR/*"
sudo cp *.pem "$MOSQUITTO_TLS_DIR/"
sudo openssl rehash "$MOSQUITTO_TLS_DIR"
sudo chown -R $MOSQUITTO_USER:$MOSQUITTO_USER "$MOSQUITTO_TLS_DIR"

clear_cert

sudo cp tim.conf /etc/mosquitto/conf.d/

sudo systemctl restart mosquitto
systemctl status mosquitto

# TIM
new_cert "$CLIENT_SUBJ"

rm -rf "$TIM_TLS_DIR"
mkdir -p "$TIM_TLS_DIR"
cp *.pem "$TIM_TLS_DIR/"

clear_cert

rm -f ca-*
