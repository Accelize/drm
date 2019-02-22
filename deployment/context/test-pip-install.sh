#!/usr/bin/env bash

python3 -m pip install -U -q --no-cache-dir pip setuptools wheel
pip3 install -U -q --no-cache-dir tox pytest
