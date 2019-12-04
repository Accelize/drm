DRM Library installation
========================

This section explains how to install the Accelize DRM library API from packages and from source.

.. _supported_os:

Supported OS
------------

Accelize supports the following "long time support" OS and provides fully tested
packages for them:

* RHEL/Centos 8
* RHEL/Centos 7 [#f1]_
* Ubuntu 18.04 LTS Bionic
* Ubuntu 16.04 LTS Xenial
* Debian 10 Buster
* Debian 9 Stretch

Following OS have been tested but are not supported by Accelize:

* Debian Testing [#f2]_
* Fedora (2 latest stable versions)
* Ubuntu (2 latest stable versions)

.. [#f1] With Python 3.6 from EPEL repository for Accelize DRM Python library.
.. [#f2] No packages are provided.


Software requirements
---------------------

* The Accelize DRM C/C++ library requires a compiler with full C++11 support.
* The Accelize DRM Python library requires Python >= 3.5.


Installation from packages
--------------------------

The DRM library packages are available only for the **amd64** / **x86_64**
architecture.


Install Accelize repository
```````````````````````````

Packages are hosted on the Accelize repository.

.. note:: Packages and repositories metadata are signed for security.

Accelize provides *stable* and a *prerelease* channels.

To install the prerelease channel simply replace ``stable`` by ``prerelease`` in the rest of this document.

.. warning:: No support is provided for *prerelease* packages.


Debian, Ubuntu: DEB repository
::::::::::::::::::::::::::::::

To install the repository, run the following commands:

.. code-block:: bash

    # Ensure common system utilities are installed
    sudo apt update
    sudo apt install -y apt-transport-https software-properties-common lsb-release gnupg curl

    # Add Accelize GPG public key for package signature verification
    curl -fsSL https://accelize.s3.amazonaws.com/gpg | sudo apt-key add -

    # Install repository
    sudo add-apt-repository "deb https://accelize.s3.amazonaws.com/deb $(lsb_release -cs) stable"
    sudo apt update


RHEL, CentOS, Fedora: RPM repository
::::::::::::::::::::::::::::::::::::

To install the Accelize repository, run the following commands:

.. code-block:: bash
    :caption: On Fedora, RHEL 8, CentOS 8

    # Ensure config manager is installed
    sudo dnf install -y 'dnf-command(config-manager)'

    # Install repository
    sudo dnf config-manager --add-repo https://accelize.s3.amazonaws.com/rpm/accelize_stable.repo

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure that config manager is installed
    sudo yum install -y yum-utils

     # Install Accelize repository:
    sudo yum-config-manager --add-repo https://accelize.s3.amazonaws.com/rpm/accelize_stable.repo


Python Library and systemd service package
``````````````````````````````````````````

This section explains how to install the DRM library as a python module or systemd service.
It is available for Python 3 only.

This package provides 3 levels of integration:

* The Accelize DRM library Python
* Cython headers to the C/C++ Accelize DRM library.
* A systemd service that provides a generic Accelize DRM
  implementation as a background service (the service is not started by
  default). Refer to :doc:`drm_library_as_service` for more details,
  especially to see the supported environments.

Depending on your OS, use the following command to install the Python package:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y python3-accelize-drm

.. code-block:: bash
    :caption: On RHEL 8, CentOS 8

    # Ensure EPEL repository is installed
    sudo dnf install -y epel-release

    # Install package
    sudo dnf install -y python3-accelize-drm

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure EPEL repository is installed
    sudo yum install -y epel-release

    # Install package
    sudo yum install -y python3-accelize-drm

.. code-block:: bash
    :caption: On Fedora

    sudo dnf install -y python3-accelize-drm

.. note:: The python DRM library is a wrapper of the C/C++ DRM library.
          The package will install automatically the C/C++ library.

Installation has been completed. To verify your installation,
refer to the section 'Validate your integration' in :doc:`drm_getting_started`.

C/C++ Library package
`````````````````````

This section explains how to install the DRM library as a C/C++ library API.

Run the following command:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y libaccelize-drm

.. code-block:: bash
    :caption: On RHEL 8, CentOS 8

    # Ensure EPEL repository is installed
    sudo dnf install -y epel-release

    # Install package
    sudo dnf install -y libaccelize-drm

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure EPEL repository is installed
    sudo yum install -y epel-release

    # Install package
    sudo yum install -y libaccelize-drm

.. code-block:: bash
    :caption: On Fedora

    sudo dnf install -y libaccelize-drm


C/C++ Library Development package
`````````````````````````````````

This section explains how to install the DRM library as a C/C++ library API.
It provides the C/C++ header files and the documentation.

Run the following command:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y libaccelize-drm-dev

.. code-block:: bash
    :caption: On RHEL 8, CentOS 8

    # Ensure EPEL repository is installed
    sudo dnf install -y epel-release

    # Install package
    sudo dnf install -y libaccelize-drm-devel

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure EPEL repository is installed
    sudo yum install -y epel-release

    # Install package
    sudo yum install -y libaccelize-drm-devel

.. code-block:: bash
    :caption: On Fedora

    sudo dnf install -y libaccelize-drm-devel


.. note:: The development package also install an offline version of this
          documentation that can be found read by opening
          ``/usr/share/accelize/drm/doc/html/index.html``.


Installation from source
------------------------

The installation from source is only recommended if there is no package
available for your configuration or to contribute to the DRM library.

This is equivalent to install the C/C++ library package, the C/C++ library
development package and optionally the Python library package.

If you need the full detail of building and testing options, refer to
:doc:`drm_library_build`.

Minimal requirements:

 * git
 * cmake >= 3.12
 * make (Or any CMake supported build tool)
 * GCC, G++ >= 4.8 (Or any compatible C++11 compiler)
 * libcurl-devel
 * jsoncpp-devel

Python library requirements:

 * Python >= 3.5
 * Python-devel
 * setuptools
 * wheel
 * cython >= 0.28

Run following commands to install all requirements:

.. code-block:: bash
    :caption: On Debian >= 10, Ubuntu >= 18.10

    # Minimal requirements
    sudo apt update
    sudo apt install -y git make g++ libcurl4-openssl-dev libjsoncpp-dev cmake

    # Python library requirements
    sudo apt install -y python3-dev python3-wheel python3-setuptools cython3

.. code-block:: bash
    :caption: On Debian < 10, Ubuntu < 18.10

    # Minimal requirements
    sudo apt update
    sudo apt install -y git make g++ libcurl4-openssl-dev libjsoncpp-dev python3-pip
    pip3 install --user -U cmake

    # Python library requirements
    sudo apt install -y python3-dev
    pip3 install --user -U cmake setuptools wheel cython

.. code-block:: bash
    :caption: On RHEL 8, CentOS 8

    # Ensure EPEL repository is installed
    sudo dnf install -y epel-release

    # Minimal requirements
    sudo dnf install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel python3-pip
    pip3 install --user -U cmake

    # Python library requirements
    sudo dnf install -y python3-devel
    pip3 install --user -U setuptools wheel cython

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure EPEL repository is installed
    sudo yum install -y epel-release

    # Minimal requirements
    sudo yum install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel python3-pip
    pip3 install --user -U cmake

    # Python library requirements
    sudo yum install -y python3-devel
    pip3 install --user -U setuptools wheel cython

.. code-block:: bash
    :caption: On Fedora

    # Minimal requirements
    sudo dnf install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel cmake

    # Python library requirements
    sudo dnf install -y python3-devel python3-setuptools python3-Cython python3-wheel

Then run the following commands to build and install the library:

.. code-block:: bash

    git clone https://github.com/Accelize/drmlib.git --recursive --depth 1
    mkdir -p drmlib/build
    cd drmlib/build

    # The "-DPYTHON3=ON" option is required only to build the Python library
    cmake -DPYTHON3=ON ..

    make -j
    sudo make install

Build packages
``````````````

If you need to build packages to easily deploy the library on your
production environment, you need to install additional requirements:

For DEB Packages:

* pkg-config
* dpkg-dev
* file

For RPM packages:

* rpm-build

Run following commands to install requirements:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y pkg-config dpkg-dev file

.. code-block:: bash
    :caption: On Fedora, RHEL 8, CentOS 8

    sudo dnf install -y rpm-build

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    sudo yum install -y rpm-build

Once dependencies are installed, simply run the previous section build and
install commands but replace "`sudo make install`" by:

.. code-block:: bash

    make package

Packages will be generated in the `drmlib/build/packages` directory.


Installation with Ansible
-------------------------

We provides an Ansible role to install the Accelize DRM. The role is available
on Ansible galaxy and can be installed using the following command:

.. code-block:: bash

    ansible-galaxy install accelize.accelize_drm

Once installed, the role can be used in your Ansible playbooks:

.. code-block:: yaml

    - hosts: servers
      become: true
      roles:
         - role: accelize.accelize_drm

For more information on the role and its variables. See the
`role Ansible Galaxy page <https://galaxy.ansible.com/accelize/accelize_drm>`_.
