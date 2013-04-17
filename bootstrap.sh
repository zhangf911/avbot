#! /bin/sh

mkdir -p m4

( cd libxmpp/gloox && autoreconf -i)

autoreconf -i

