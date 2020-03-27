#! /usr/bin/env python3
# coding=utf-8
"""
Run "terraform apply", but retries in case of Spot instance specific errors,
or use "on-demand" instance if too many spot errors.
"""
from subprocess import PIPE, run
from sys import exit
from time import sleep


def apply():
    """
    Apply with retries.
    """
    retries = 5
    failures = 0
    command = ['terraform', 'apply', '-auto-approve', '-input=false']

    while True:

        # Run "spot" instance
        process = run(command, universal_newlines=True, stderr=PIPE)
        if not process.returncode:
            return

        # Max retries, exit loop to run "on-demand" instance
        if failures > retries:
            print('\033[31mError, trying once with on-demand instead of '
                  'spot.\033[30m')
            # Change variable to not use spot
            from json import dump, load
            with open('terraform.tfvars.json', 'rt') as json_file:
                tfvars = load(json_file)
                tfvars['spot'] = 'false'
            with open('terraform.tfvars.json', 'wt') as json_file:
                dump(tfvars, json_file)

            # Run "on-demand" instance, and exit
            exit(run(command).returncode)

        # Retryable error, retries another spot
        for retryable_error in (
                "Error requesting spot instances: "
                "InvalidSubnetID.NotFound: "
                "No default subnet for availability zone: 'null'",
                'Error while waiting for spot request',
                'HTTP request error.'):
            if retryable_error in str(process.stderr):
                failures += 1
                print(f'\033[31mError, retrying ({failures}/{retries})'
                      f', stderr:\033[30m\n{process.stderr}')
                sleep(1)
                break

        # Not a retryable error, exit with error
        else:
            print(process.stderr)
            exit(process.returncode)


if __name__ == '__main__':
    apply()
