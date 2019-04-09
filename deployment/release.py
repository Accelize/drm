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
            'Release Accelize DRM library. This tool performs sanity checks and'
            ' add a Git tag for the release, then CI start and perform release '
            'steps (Tests, packaging, deployment).'
        )
        parser.add_argument(
            'version', help=
            'Version to release in semantic versioning format. '
            'Expected format: '
            'Stable: "1.0.0"; '
            'Alpha: "1.0.0-alpha.1"; '
            'Beta: "1.0.0-beta.1"; '
            'Release candidate: "1.0.0-rc.1"'
        )
        parser.add_argument('--force', '-f', action='store_true',
                            help='Force overwriting exiting release.')
        parser.add_argument('--remove', '-r', action='store_true',
                            help='Remove release tag if exist.')
        parser.add_argument('--dry', '-d', action='store_true',
                            help='Dry run. Do not modify Git repository')
        args = parser.parse_args()

        from os import chdir
        from os.path import realpath, dirname
        from subprocess import run, PIPE

        run_args = dict(stdout=PIPE, stderr=PIPE, universal_newlines=True)
        chdir(dirname(dirname(realpath(__file__))))

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
            parser.exit(1, 'Release cancelled\n')

        # Build metadata is automatically generated
        if '+' in base_version:
            print('Do not add build metadata (Value after "+"). '
                  'It is automatically generated.')
            parser.exit(1, 'Release cancelled\n')
            return

        # Check prerelease on non master branch
        result = run(['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                     **run_args)
        result.check_returncode()
        branch = result.stdout.strip()

        if branch != 'master' and '-' not in version:
            print('A non prerelease version must be released only on master '
                  'branch.')
            parser.exit(1, 'Release cancelled\n')
            return

        # Check CMake file
        with open('CMakeLists.txt', 'rt') as cmakelists:
            for line in cmakelists:
                if line.startswith('set(ACCELIZEDRM_VERSION '):
                    cpack_package_version = line.split(
                        ' ')[1].strip().strip(')')
                    break
            else:
                print('ACCELIZEDRM_VERSION not found in CMakeLists.txt')
                parser.exit(1, 'Release cancelled\n')
                return

        if cpack_package_version != base_version:
            print('ACCELIZEDRM_VERSION in CMakeLists.txt does not match '
                  'release version (CPack package version: '
                  f'"{cpack_package_version}")')
            parser.exit(1, 'Release cancelled\n')
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
                parser.exit(1, 'Release cancelled\n')
                return

        # Checks if tag not already exists on origin or locally
        result = run(['git', 'ls-remote', '--tags', 'origin'], **run_args)
        result.check_returncode()
        remote_existing_tags = set(
            line.rsplit('/', 1)[1] for line in result.stdout.splitlines())

        result = run(['git', 'tag'], **run_args)
        result.check_returncode()
        existing_tags = set(result.stdout.splitlines())

        if version in remote_existing_tags or version in existing_tags:
            if not (args.force or args.remove):
                print(f'A release with version {version} already exist !')
                answer = None
                while answer not in ('y', 'n'):
                    answer = input(
                        f'Move release {version} to current commit ("y"/"n"): '
                    ).lower().strip()

                # Cancel release
                if answer == 'n':
                    parser.exit(1, 'Release cancelled\n')
                    return

            # Remove previously existing tag
            print(f"Removing previous {version} tag...")
            if version in remote_existing_tags and not args.dry:
                run('git push --delete origin ' + version, shell=True,
                    **run_args).check_returncode()
            if version in existing_tags and not args.dry:
                run(['git', 'tag', '--delete', version],
                    **run_args).check_returncode()

        # Add tag and push
        if not args.remove:
            print(f'Adding {version} tag...')
            if not args.dry:
                run(['git', 'tag', '-a', version, '-m', f'Release {version}'],
                    **run_args).check_returncode()

            print('Pushing tag...')
            if not args.dry:
                run(['git', 'push', '--tags'], **run_args).check_returncode()

            parser.exit(message='Release successful\n')

    _release()
