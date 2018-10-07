#!/bin/sh

echo "Setting up test database"
psql -d "postgres" -c "
CREATE ROLE fastcgipp_test NOSUPERUSER NOCREATEDB NOCREATEROLE INHERIT LOGIN PASSWORD 'fastcgipp_test';"
createdb -O fastcgipp_test fastcgipp_test
psql -U fastcgipp_test -d fastcgipp_test -c "
CREATE TABLE fastcgipp_test (
    zero    serial PRIMARY KEY,
    one     smallint,
    two     bigint,
    three   text,
    four    real,
    five    double precision,
    six     bytea,
    seven   text
);"
