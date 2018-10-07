#!/bin/sh

echo "Purging test database"
dropdb fastcgipp_test
dropuser fastcgipp_test
