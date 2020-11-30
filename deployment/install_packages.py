#! /usr/bin/env python3
"""
Install all packages from a directory
"""
from os import listdir as _listdir
from os.path import join as _join, realpath as _realpath, splitext as _splitext
from subprocess import (
    CalledProcessError as _CalledProcessError,
    DEVNULL as _DEVNULL,
    PIPE as _PIPE,
    run as _run,
)


def detect_package_manager():
    """
    Detect current OS package manager.

    Returns:
        str: Install command.
    """
    for utility, command in (
        (
            "apt-get",
            "apt-get -qq update && " "apt-get install -y --no-install-recommends ",
        ),
        ("dnf", "dnf install -y "),
        ("yum", "yum install -y "),
        ("rpm", "rpm -i "),
    ):
        try:
            if not _run(
                [utility, "--help"], stdout=_DEVNULL, stderr=_DEVNULL
            ).returncode:
                return command
        except FileNotFoundError:
            continue


def install_packages(packages_dir, quiet=False):
    """
    Install packages.

    Args:
        packages_dir (str): Directory containing packages.
        quiet (bool): Hide packages manager output.
    """
    # Get packages
    packages_dir = _realpath(packages_dir)
    packages = [
        _join(packages_dir, package_file)
        for package_file in _listdir(packages_dir)
        if (
            _splitext(package_file)[-1].lower() in (".deb", ".rpm")
            and
            # Skip "-dev"/"-devel" packages
            "-dev" not in package_file
        )
    ]

    # Run command
    if quiet:
        run_kwargs = dict(stdout=_PIPE, stderr=_PIPE, universal_newlines=True)
    else:
        run_kwargs = dict()

    _run(
        detect_package_manager() + " ".join(packages), shell=True, **run_kwargs
    ).check_returncode()


if __name__ == "__main__":

    def run_command():
        """
        Packages installer.
        """
        from argparse import ArgumentParser

        parser = ArgumentParser(
            prog="install_packages", description="Install packages."
        )
        parser.add_argument("packages_dir", help="Input packages directory")
        parser.add_argument(
            "--quiet", "-q", help="Disable verbosity", action="store_true"
        )
        args = parser.parse_args()

        try:
            install_packages(packages_dir=args.packages_dir, quiet=args.quiet)
        except _CalledProcessError as exception:
            parser.exit(exception.returncode, exception.stdout)

        if not args.quiet:
            parser.exit(message="Installation successful")

    run_command()
