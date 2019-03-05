#! /usr/bin/env python3
# coding=utf-8
"""
Sign RPM and DEB packages
"""
from os.path import (
    realpath as _realpath, splitext as _splitext, expanduser as _expanduser)
from os import listdir as _listdir
from subprocess import (
    run as _run_cmd, PIPE as _PIPE, CalledProcessError as _CalledProcessError)


def _run(*arg, quiet=False, check_returncode=True, **kwargs):
    """
    Call command in subprocess

    Args:
        quiet (bool): Hide commands output.
        check_returncode (bool): Check return code.

    Returns:
        subprocess.CompletedProcess: call result.
    """
    if quiet:
        kwargs.update(dict(stdout=_PIPE, stderr=_PIPE, universal_newlines=True))

    result = _run_cmd(*arg, **kwargs)
    if check_returncode:
        result.check_returncode()
    return result


def _init_gpg_configuration(private_key=None, quiet=False):
    """
     Initialize GPG configuration.

     Args:
         private_key (str): Path to GPG private key to use.
             If no private key specified, use current GPG configuration.
        quiet (bool): Hide commands output.
     """
    if private_key is None:
        return

    # Clear GPG configuration
    _run(['gpg', '--delete-keys'], quiet=quiet, check_returncode=False)
    _run(['gpg', '--delete-secret-keys'], quiet=quiet, check_returncode=False)

    # Import private key
    _run(['gpg', '--batch', '--no-tty', '--import', private_key],
         quiet=quiet, check_returncode=False)


def sign_rpm_packages(
        packages_dir, private_key=None, public_key=None, pass_phrase='',
        quiet=False):
    """
    Sign all RPM packages in a directory.

    Args:
        packages_dir (str): Directory containing packages.
        private_key (str): Path to GPG private key to use.
            If no private key specified, use current GPG configuration.
        public_key (str): Path to GPG public key to use.
        pass_phrase (str): GPG key pass phrase.
        quiet (bool): Hide commands output.
    """
    _init_gpg_configuration(private_key)

    # Import public key
    if public_key:
        _run(['rpm', '--import', public_key], quiet=quiet)

    # Sign packages
    packages = [package for package in _listdir(packages_dir)
                if _splitext(package)[1].lower() == '.rpm']

    gpg_info = _run('gpg --export | gpg --list-packets',
                    shell=True, quiet=True).stdout
    for line in gpg_info.strip().splitlines():
        if ':user ID packet' in line:
            gpg_user_id = line.rsplit(':', 1)[1].strip().strip('"')
            break
    else:
        raise RuntimeError('Unable to read GPG User ID')

    macros = (
        '_signature gpg', '_gpg_path %s' % _expanduser("~/.gnupg"),
        '_gpg_name %s' % gpg_user_id, '_gpgbin /usr/bin/gpg', ' '.join((
            '__gpg_sign_cmd %{__gpg}', 'gpg', '--force-v3-sigs', '--batch',
            '--verbose', '--no-armor', '--passphrase "%s"' % pass_phrase,
            '--no-secmem-warning', '-u', '"%{_gpg_name}"', '-sbo',
            '%{__signature_filename}', '--digest-algo', 'sha256',
            '%{__plaintext_filename}')))
    define = []
    for macro in macros:
        define.extend(["--define", macro])

    _run(['rpm', '--addsign'] + define + packages, quiet=True,
         cwd=packages_dir)

    # Verify signatures
    result = _run(['rpm', '--checksig'] + packages, quiet=True,
                  cwd=packages_dir)
    for line in result.stdout.splitlines():
        line = line.rstrip()
        if (not line.endswith('gpg OK') and
                not line.endswith('pgp md5 OK') and
                not line.endswith('digests signatures OK')):
            raise RuntimeError(
                'Package signature verification failure: %s' % line)


def sign_deb_packages(
        packages_dir, private_key=None, public_key=None, pass_phrase='',
        quiet=False):
    """
    Sign all DEB packages in a directory.

    Args:
        packages_dir (str): Directory containing packages.
        private_key (str): Path to GPG private key to use.
            If no private key specified, use current GPG configuration.
        public_key (str): Path to GPG public key to use.
        pass_phrase (str): GPG key pass phrase.
        quiet (bool): Hide packages manager output.
    """
    _init_gpg_configuration(private_key)

    # Sign packages
    packages = [package for package in _listdir(packages_dir)
                if _splitext(package)[1].lower() == '.deb']

    _run(['dpkg-sig', '-g', '--passphrase "%s"' % pass_phrase,
          '--sign', 'builder'] + packages, quiet=quiet, cwd=packages_dir)

    # Verify signatures
    _run(['dpkg-sig', '--verify'] + packages, quiet=quiet, cwd=packages_dir)


if __name__ == '__main__':

    def run_command():
        """Sign packages"""
        from argparse import ArgumentParser
        parser = ArgumentParser(
            prog='sign_packages',
            description='Sign RPM or DEB packages.')
        parser.add_argument('packages_dir', help='Input packages directory')
        parser.add_argument('--private_key', '-k', help='Private GPG key')
        parser.add_argument('--public_key', '-b', help='Public GPG key')
        parser.add_argument('--pass_phrase', '-p', help='GPG key pass phrase')
        parser.add_argument('--quiet', '-q', help='Disable verbosity',
                            action='store_true')
        args = parser.parse_args()

        packages_dir = _realpath(args.packages_dir)

        if args.private_key is not None and not args.private_key:
            # Passed and empty value: command called from build script but
            # not private key is defined. In this case, signature is disabled.
            return

        for file in _listdir(packages_dir):
            ext = _splitext(file)[1].lower()
            if ext == '.rpm':
                sign = sign_rpm_packages
                break
            elif ext == '.deb':
                sign = sign_deb_packages
                break
        else:
            parser.exit(1, 'No package to sign.')
            return

        try:
            sign(packages_dir=packages_dir, private_key=args.private_key,
                 public_key=args.public_key, pass_phrase=args.pass_phrase,
                 quiet=args.quiet)
        except _CalledProcessError as exception:
            parser.exit(exception.returncode, exception.stdout)
        except RuntimeError as exception:
            parser.exit(1, str(exception))

        if not args.quiet:
            parser.exit(message='Signature successful')

    run_command()
