# coding=utf-8
"""
This library provides some DMR library specific CI/CD pipeline utilities.
Used in the "Release" stage of "azure-pipeline.yml"
"""
import os as _os

DEPLOYMENT_DIR = _os.path.realpath(_os.path.dirname(__file__))
PROJECT_DIR = _os.path.dirname(DEPLOYMENT_DIR)


def get_drm_version():
    """
    Return DRM library version.

    Returns:
        str: DRM library version.
    """
    path = _os.path.join(PROJECT_DIR, 'CMakeLists.txt')
    with open(path, 'rt') as cmakelists:
        for line in cmakelists:
            if line.startswith('set(ACCELIZEDRM_VERSION '):
                version = f"v{line.split(' ')[1].strip().strip(')')}"
                print(f'Detected DRM library version: {version}')
                return version
    raise ValueError(f'ACCELIZEDRM_VERSION not found in "{path}"')


def get_next_package_release(versions_json):
    """
    Return next release number for current DRM library version.

    Args:
        versions_json (str): Path to "versions.json" published release manifest.

    Returns:
        int: Next package release number.
    """
    from json import load
    version = get_drm_version()
    with open(versions_json) as file:
        versions = load(file)
    try:
        release = versions['accelize_drm'][version]
    except KeyError:
        print(f'No published release for DRM library version {version}, '
              'setting next release number to "1"')
        return 1
    next_release = release + 1
    print(f'DRM library version {version} was already published with release '
          f'number "{release}", setting next release number to '
          f'"{next_release}"')
    return next_release


def publish_packages(pkg_source, versions_json, deb_repo, rpm_repo, deb_s3,
                     gpg_private_key, gpg_key_id):
    """
    Publish Accelize DRM library packages.

    Args:
        pkg_source (str): Path to packages source directory.
        versions_json (str): Path to versions.json file that store last release
            number for all versions.
        deb_repo (str): Path to local DEB repository.
        rpm_repo (str): Path to local RPM repository.
        deb_s3 (str): S3 DEB repository.
        gpg_private_key (str): Path to GPG key to use to sign packages.
        gpg_key_id (str): ID of the GPG key to use to sign packages.
    """
    from json import dump, load
    from os import makedirs, listdir, walk, symlink
    from os.path import splitext, join, isfile, realpath
    from subprocess import run, STDOUT
    from tempfile import TemporaryDirectory

    run_kwargs = dict(stderr=STDOUT, check=True)

    # Define repositories information
    pkg_version = None
    tag = get_drm_version()
    prerelease = '-' in tag
    component = 'prerelease' if prerelease else 'stable'
    repo_base_url = "https://tech.accelize.com"

    if prerelease:
        assert deb_s3.endswith("deb_prerelease/")

    deb_conf = {
        'Origin': 'Accelize',
        'Label': 'Accelize',
        'Codename': None,
        'Architectures': 'amd64',
        'Components': component,
        'Description': 'Accelize DEB repository',
        'SignWith': gpg_key_id
    }

    rpm_conf = {
        'name': 'Accelize RPM repository',
        'baseurl':
            f'{repo_base_url}/rpm/{component}/$releasever/$basearch/',
        'enabled': '1',
        'gpgkey': f'{repo_base_url}/gpg',
        'gpgcheck': '1'
    }

    # Import GPG key
    print("GETTING GPG KEY...")
    run(['gpg', '--batch', '--no-tty', '--import', gpg_private_key],
        **run_kwargs)

    # Retrieve packages
    packages = dict(rpm=dict(), deb=dict())

    print("GETTING PACKAGES...")
    for root, dirs, files in walk(pkg_source):
        for file_name in files:
            pkg_name, ext = splitext(file_name)

            # DEB with format:
            # <name>_<version>-<release>+<os_codename>_<cpu_arch>.deb
            if ext == '.deb':
                # Get:
                # - OS codename, example: stretch, xenial
                # - Package version
                parts = pkg_name.split('_')
                pkg_version, codename = parts[1].split('+', 1)

                # Add package to list
                packages['deb'].setdefault(codename, list())
                pkg_path = join(root, file_name)
                packages['deb'][codename].append(pkg_path)
                print(f'Found DEB (codename="{codename}", path="{pkg_path}")')

            # RPM with format:
            # <name>-<version>-<release>.<os_release>.<cpu_arch>.rpm
            elif ext == '.rpm':
                parts = pkg_name.rsplit('.', 2)

                # Get CPU architecture, example: x86_64, noarch
                basearch = parts[-1]

                # Get OS release, example: el7, fc30
                releasever = ''.join(c for c in parts[-2] if c.isdigit())

                # Add package to list
                pkg_path = join(root, file_name)
                packages['rpm'].setdefault(releasever, dict())
                packages['rpm'][releasever].setdefault(basearch, list())
                packages['rpm'][releasever][basearch].append(
                    (pkg_path, file_name))
                print(f'found RPM (releasever="{releasever}", '
                      f'basearch="{basearch}", path="{pkg_path}")')

    if not packages['deb'] and not packages['rpm']:
        raise FileNotFoundError(f'No packages in "{realpath(pkg_source)}"')

    # Update DEB repository
    print("UPDATING DEB REPOSITORY...")
    with TemporaryDirectory() as base_dir:
        reprepro = ['reprepro', '--outdir', deb_repo, '--basedir', base_dir]

        # Create configuration for each codename
        conf_dir = join(base_dir, 'conf')
        makedirs(conf_dir, exist_ok=True)
        conf_file_path = join(conf_dir, 'distributions')
        for codename in packages['deb']:
            deb_conf['Codename'] = codename
            with open(conf_file_path, 'at') as conf_file:
                for key, value in deb_conf.items():
                    conf_file.write(f'{key}: {value}\n')
                conf_file.write('\n')
        print(f'Created configuration file: "{conf_file_path}"')

        # Add package to repository and update it
        for codename, pkg_set in packages['deb'].items():
            for pkg_path in pkg_set:
                run(reprepro + [
                    '--component', component,
                    'includedeb',  codename, pkg_path], **run_kwargs)
                print(f'Included package: "{pkg_path}"')

        # Check repository
        if isfile(conf_file_path):
            run(reprepro + ['check'] + list(packages['deb']), **run_kwargs)
            run(reprepro + ['checkpool'], **run_kwargs)

    # Create RPM repository configuration file
    print("UPDATING RPM REPOSITORY...")
    conf_file_path = join(rpm_repo, f'accelize_{component}.repo')
    with open(conf_file_path, 'wt') as conf_file:
        conf_file.write(f'[accelize_{component}]\n')
        for key, value in rpm_conf.items():
            conf_file.write(f'{key}={value}\n')
    print(f'Created configuration file: "{conf_file_path}"')

    # Update .rpm repository
    for releasever, basearchs in packages['rpm'].items():
        # Move "noarch" in other architectures.
        try:
            pkg_set = basearchs.pop('noarch')
        except KeyError:
            continue
        for basearch in tuple(basearchs):
            basearchs[basearch] += pkg_set

        # Update repository
        for basearch, pkg_set in basearchs.items():
            repo_path = join(rpm_repo, component, releasever, basearch)
            makedirs(repo_path, exist_ok=True)

            for pkg in pkg_set:
                pkg_src = pkg[0]
                pkg_dst = join(repo_path, pkg[1])
                symlink(pkg_src, pkg_dst)
                print(f'Moved package "{pkg_src}" to "{pkg_dst}"')

            with TemporaryDirectory() as cache_dir:
                run(['createrepo', '--update', '--cachedir', cache_dir,
                     '--deltas', repo_path], **run_kwargs)

            # Sign metadata XML
            for name in listdir(repo_path):
                path = join(repo_path, name, 'repodata/repomd.xml')
                if isfile(path):
                    run(['gpg', '--detach-sign', '--batch', '--yes',
                         '--no-tty', '--armor', path], **run_kwargs)
                    print(f'Signed "{path}"')

    # Get release number from packages and check it match with next
    # excepted release in published release manifest
    pkg_release = int(pkg_version.rsplit('-', 1)[1].split('+', 1)[0])
    next_release = get_next_package_release(versions_json)
    if next_release != pkg_release:
        raise RuntimeError(
            f'Package release number "{pkg_release}" does not match with '
            f'current repository next package number "{next_release}".')

    # Update published release manifest
    with open(versions_json, 'rt') as file:
        versions = load(file)

    versions.setdefault('accelize_drm', dict())
    versions['accelize_drm'][tag] = pkg_release

    with open(versions_json, 'wt') as file:
        dump(versions, file)

    print(f'Saved release "{pkg_release}" for version "{tag}".')
