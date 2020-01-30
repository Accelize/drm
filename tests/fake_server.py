# -*- coding: utf-8 -*-
import os
import sys
import logging
import argparse
from flask import Flask

app = Flask(__name__)


@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def catch_all(path):
    print('You want path: %s' % path)
    return 'You want path: %s' % path

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--url', required=True, default='https://master.devmetering.accelize.com',
            help='Specify which url to fake')
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose mode')
    args = parser.parse_args()

    level = logging.DEBUG if args.verbose else logging.INFO

    logging.basicConfig(
        level=level,
        format="%(asctime)s - %(levelname)-7s, %(lineno)-4d : %(message)s",
        handlers=[
            logging.StreamHandler()
        ])

    app.run()