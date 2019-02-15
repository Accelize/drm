#! /usr/bin/env python3
# coding=utf-8
"""
Install Accelize DRM library packages
"""
from os.path import realpath as _realpath, join as _join, splitext as _splitext
from os import listdir as _listdir
from subprocess import (
    run as _run, PIPE as _PIPE, DEVNULL as _DEVNULL,
    CalledProcessError as _CalledProcessError)


def detect_package_manager():
    """
    Detect current OS package manager.

    Returns:
        str: Package manager utility
    """
    for utility in ('apt', 'dnf', 'yum'):
        try:
            if not _run([utility, '--help'],
                        stdout=_DEVNULL, stderr=_DEVNULL).returncode:
                return utility
        except FileNotFoundError:
            continue


def install_accelize_drm_library(packages_dir, quiet=False):
    """
    Install Accelize DRM library packages.

    Args:
        packages_dir (str): Directory containing packages.
        quiet (bool): Hide packages manager output.
    """
    # Get packages
    packages_dir = _realpath(packages_dir)
    packages = [
        _join(packages_dir, package_file)
        for package_file in _listdir(packages_dir)
        if (_splitext(package_file)[-1].lower() in ('.deb', '.rpm') and
            '-dev' not in package_file)]

    # Run command
    if quiet:
        run_kwargs = dict(stdout=_PIPE, stderr=_PIPE, universal_newlines=True)
    else:
        run_kwargs = dict()

    _run([detect_package_manager(), 'install', '-y'] + packages,
         **run_kwargs).check_returncode()


if __name__ == '__main__':

    def run_command():
        """
        Accelize DRM library packages installer.
        """
        from argparse import ArgumentParser
        parser = ArgumentParser(
            prog='install_packages',
            description='Install Accelize DRM library packages.')
        parser.add_argument('packages_dir', help='Input packages directory')
        parser.add_argument('--quiet', '-q', help='Disable verbosity',
                            action='store_true')
        args = parser.parse_args()

        try:
            install_accelize_drm_library(
                packages_dir=args.packages_dir, quiet=args.quiet)
        except _CalledProcessError as exception:
            parser.exit(exception.returncode, exception.stdout)

        if not args.quiet:
            parser.exit(message='Installation successful')

    run_command()
