#! /usr/bin/env python3
# coding=utf-8
"""
Release Accelize DRM library
"""


if __name__ == '__main__':

    def _release():
        """Make a release"""
        from argparse import ArgumentParser

        parser = ArgumentParser(
            prog='release', description=
            'Release Accelize DRM library after performing verifications.')
        parser.add_argument(
            'version', help='Version to release in semantic versioning format.')
        parser.add_argument('--force', '-f',
                            help='Force overwriting exiting release.')
        args = parser.parse_args()

        from os import chdir
        from os.path import dirname
        from subprocess import run, PIPE

        run_args = dict(stdout=PIPE, stderr=PIPE, universal_newlines=True)
        chdir(dirname(dirname(__file__)))

        # Force version format to start with "v"
        print(f'Checking version format...')
        version = args.version
        version = version.lower()
        if version[0] != 'v':
            version = f'v{version}'

        base_version = version.lstrip('v')

        # Check if version match semantic versioning
        # Regex source: https://github.com/semver/semver/issues/232
        from re import fullmatch

        if not fullmatch(r'^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)'
                         r'(-(0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)'
                         r'(\.(0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*)?'
                         r'(\+[0-9a-zA-Z-]+(\.[0-9a-zA-Z-]+)*)?$',
                         base_version):
            print(
                f'Version {version} does not match semantic versioning format: '
                '"major.minor.path-prerelease" (https://semver.org/)')
            parser.exit(1, 'Release cancelled')

        # Build metadata is automatically generated
        if '+' in base_version:
            print('Do not add build metadata (Value after "+"). '
                  'It is automatically generated.')
            parser.exit(1, 'Release cancelled')
            return

        # Check prerelease on non master branch
        result = run(['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                     **run_args)
        result.check_returncode()
        branch = result.stdout.strip()

        if branch != 'master' and '-' not in version:
            print('A non prerelease version must be released only on master '
                  'branch.')
            parser.exit(1, 'Release cancelled')
            return

        # Check CMake file
        with open('CMakeLists.txt', 'rt') as cmakelists:
            for line in cmakelists:
                if line.startswith('set(CPACK_PACKAGE_VERSION '):
                    cpack_package_version = line.split(
                        ' ')[1].strip().strip(')')
                    break
            else:
                print('CPACK_PACKAGE_VERSION not found in CMakeLists.txt')
                parser.exit(1, 'Release cancelled')
                return

        if cpack_package_version != base_version:
            print('CPACK_PACKAGE_VERSION in CMakeLists.txt does not match '
                  'release version (CPack package version: '
                  f'"{cpack_package_version}")')
            parser.exit(1, 'Release cancelled')
            return

        # Check changelog (Not for prereleases)
        if '-' not in version:
            with open('CHANGELOG', 'rt') as changelog:
                last_change = changelog.readline().strip()

            if not fullmatch(
                    r"\* [a-zA-Z]{3} [a-zA-Z]{3} [0-9]{2} [0-9]{4} Accelize " +
                    version, last_change):
                print('Version not found as last change in "CHANGELOG" file '
                      f'(Last change: "{last_change}")')
                parser.exit(1, 'Release cancelled')
                return

        # Checks if tag not already exists on origin or locally
        result = run(['git', 'ls-remote', '--tags', 'origin'], **run_args)
        result.check_returncode()
        remote_existing_tags = set(result.stdout.splitlines())

        result = run(['git', 'tag'], **run_args)
        result.check_returncode()
        existing_tags = set(result.stdout.splitlines())

        if version in remote_existing_tags or version in existing_tags:
            if not args.force:
                print(f'A release with version {version} already exist !')
                answer = None
                while answer not in ('y', 'n'):
                    answer = input(
                        f'Move release {version} to current commit ("y"/"n"): '
                    ).lower().strip()

                # Cancel release
                if answer == 'n':
                    parser.exit(1, 'Release cancelled')
                    return

            # Remove previously existing tag
            print(f"Removing previous {version} tag...")
            if version in remote_existing_tags:
                run(['git', 'push', '--delete', 'origin ', version],
                    **run_args).check_returncode()
            if version in existing_tags:
                run(['git', 'tag', '--delete', version],
                    **run_args).check_returncode()

        # Add tag and push
        print(f'Adding {version} tag...')
        run(['git', 'tag', '-a', version, '-m', f'Release {version}'],
            **run_args).check_returncode()

        print('Pushing tag...')
        run(['git', 'push', '--tags'], **run_args).check_returncode()

        parser.exit(0, 'Release successful')

    _release()