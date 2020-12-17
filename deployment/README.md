# Deployment guidelines

This document explains Accelize DRM library new release deployment.

## Version number format

The Accelize DRM library use [semantic versioning](https://semver.org/).

Versions need to match the following format:

* Stable: `1.0.0`
* Prerelease, Alpha: `1.0.0-alpha.1`
* Prerelease, Beta: `1.0.0-beta.1`
* Prerelease, Release candidate: `1.0.0-rc.1`

Build metadata must not be added to the version. It is generated automatically 
with the following values:

* Git Commit ID for the version that appears in the library headers.
* Automatically generated package release number and distribution specific tag
  for packages versions.

## Deploying a new release

The project maintainer needs to perform the following steps to deploy a new
release:

*All paths are relative to the project root.*

* Checkout the relevant Git branch: Stable release must be on the
  `master` branch and prereleases on the `dev` branch.
* Set `ACCELIZEDRM_VERSION` value to this release version number in the
  `./CMakeLists.txt` file.
* For a stable release, the `./CHANGELOG` file must be completed with new
  release changes (the changelog format must not be changed to allow proper RPM
  package builds).
* Commit the changes.
* Run the release script with the specified version (Replacing `$VERSION` by the
  version number):
  
```bash
./deployment/release.py $VERSION
```

If the script terminates successfully, a new tag will appear on Github for this
version. Then the continuous integration will run and generate the following:

* Packages on the Accelize repository.
* GitHub release.
* Documentation on the Accelize repository (Is generated if a doc file as
  changed since the previous commit)

### Continuous integration failure

In case of continuous integration failure. Fix issues, commit changes and
run the release script with overwriting the previous release attempt:

```bash
./deployment/release.py $VERSION --force
```

This will restart move the tag to the new commit and restart continuous
integration.

# Continuous integration

Continuous integration performs the following sequence:

*Any operation that fails stops immediately the sequence*

If a documentation file changed:
* Build the documentation.
* If the build is successful, Update the documentation on Accelize website
  (`master` and `dev` branches generate different documentation pages).

If a code file changed:
* Run the build and full tests sequence with enabled coverage.

If the current commit is tagged (Meaning that it is a release):
* Build packages for all target OS.
* Run unit tests on all packages and all target OS.
* If tests are successful, uploads packages on Accelize repository (Prerelease
  and stable versions are uploaded on different repositories)
* If package upload is successful, generate a GitHub release (prereleases
  versions are tagged as prereleases on GitHub).

# Packages updates and OS lifecycle

Supported OS evolves, new version are released and old are deprecated. Provided
packages needs to follow this lifecycle.

Supported OS release cycles:
- CentOS: ~5 years, aligned on RHEL releases
- Debian: Every 2 odd years in June or July
- Fedora: Every 6 month, April/October
- Ubuntu LTS: Every 2 even year in April
- Ubuntu rolling: Every 6 month, April/October

Supported OS version end of life:
- CentOS: 10 years
- Debian: 5 years
- Fedora: 1 year
- Ubuntu LTS: 10 years
- Ubuntu rolling: 1 year

#### Actions to perform

For all:
- In the file `deployment/azure-pipelines.yml`, the increment the versions in
  the `matrix` section of the job `buildPackages` of the stage `release`.
- Eventually update `deployment/Dockerfile.j2` if installation instruction 
  changed.

For CentOS, Debian and Ubuntu LTS only:
- Update supported OS list in documentation (And eventually installation
  instructions): `doc/drm_library_installation.rst`

In all case, if no library release planned soon, force rebuild of packages with
updated files:
- Commit changes in `master` branch.
- Run `deployment/release.py` specifying the current version.
