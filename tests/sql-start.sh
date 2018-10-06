#!/bin/sh

psql -d "postgres" -ec "
CREATE ROLE fastcgipp_test NOSUPERUSER NOCREATEDB NOCREATEROLE INHERIT LOGIN PASSWORD 'fastcgipp_test';"
createdb -eO fastcgipp_test fastcgipp_test
psql -eU fastcgipp_test -d fastcgipp_test -c "
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
