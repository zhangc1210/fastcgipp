#!/bin/sh

psql -d "postgres" -c "CREATE ROLE fastcgipp_example NOSUPERUSER NOCREATEDB NOCREATEROLE INHERIT LOGIN PASSWORD 'fastcgipp_example';" &&
createdb -O fastcgipp_example fastcgipp_example &&
export PGPASSWORD="fastcgipp_example"
psql -U fastcgipp_example -d fastcgipp_example -c "CREATE TABLE fastcgipp_example (stamp timestamptz, address inet, string text);"
