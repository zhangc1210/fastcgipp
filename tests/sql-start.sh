#!/bin/sh

createuser -e fastcgipp_test
createdb -eO fastcgipp_test fastcgipp_test
psql -eU fastcgipp_test -d fastcgipp_test -c "
CREATE TABLE fastcgipp_test (
    zero    serial2 PRIMARY KEY,
    one     integer,
    two     bigint,
    three   text,
    four    real,
    five    double precision,
    six     bytea,
    seven   bytea,
    eight   text
);"
