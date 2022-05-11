
openssl genrsa -out rootca.key 2048
echo 'Generate ROOT CA certificate:'
openssl req -x509 -new -nodes -key rootca.key -days 365 -out rootca.crt
openssl genrsa -out user.key 2048
echo 'Generate user certificate:'
openssl req -new -key user.key -out user.csr
openssl x509 -req -in user.csr -CA rootca.crt -CAkey rootca.key -CAcreateserial -out user.crt -days 365

