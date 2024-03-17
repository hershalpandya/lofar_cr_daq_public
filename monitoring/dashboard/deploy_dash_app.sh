#!/bin/bash
gunicorn index:server -b :8010 -w 1 --timeout 120
#https://docs.gunicorn.org/en/stable/design.html#how-many-workers
#gunicorn dashboard:server -b :8010
