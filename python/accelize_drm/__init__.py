# coding=utf-8
"""Accelize DRM Python Library


Copyright 2018 Accelize

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

__version__ = "@ACCELIZEDRM_LONG_VERSION@"
__copyright__ = "Copyright 2019 Accelize"
__licence__ = "Apache 2.0"
__all__ = ['DrmManager', 'exceptions', 'get_api_version']

from os import environ as _environ
from collections import namedtuple as _namedtuple


import accelize_drm.exceptions

if _environ.get('ACCELIZE_DRM_PYTHON_USE_C'):
    # Bind Python Accelize DRM on libaccelize_drmc (C variant)
    from accelize_drm._accelize_drmc import DrmManager, _get_api_version
    _library = 'libaccelize_drmc'
else:
    # Bind Python Accelize DRM on libaccelize_drm (C++ variant)
    from accelize_drm._accelize_drm import DrmManager, _get_api_version
    _library = 'libaccelize_drm'

del _environ

_ApiVersion = _namedtuple('ApiVersion', [
    'major', 'minor', 'revision', 'prerelease', 'build', 'version',
    'py_major', 'py_minor', 'py_revision', 'py_prerelease', 'py_build',
    'py_version', 'backend'
])
del _namedtuple


def get_api_version():
    """
    Get version as named tuple containing :

    - C/C++ library version as major, minor, revision, prerelease, build.
    - Python library version as py_major, py_minor, py_revision, py_prerelease,
      py_build
    - C/C++ backend library name.

    Returns:
        namedtuple: Containing following values:
        major, minor, revision, prerelease, build, version,
        py_major, py_minor, py_revision, py_prerelease, py_build,
        backend
    """
    api_version = list()

    # Parse version string
    for version in (_get_api_version().decode(), __version__):
        try:
            _ver, build = version.split('+', 1)
        except ValueError:
            _ver = version
            build = ''

        # Get prerelease
        try:
            _ver, prerelease = _ver.split('-', 1)
        except ValueError:
            prerelease = ''

        major, minor, revision = _ver.split('.', 2)

        # Convert to int if possible
        version_list = [major, minor, revision, prerelease, build, version]
        for i in range(len(version_list)):
            try:
                version_list[i] = int(version_list[i])
            except ValueError:
                pass

        api_version.extend(version_list)

    # Add library backend
    api_version.append(_library)

    # Return named tuple
    return _ApiVersion(*api_version)
