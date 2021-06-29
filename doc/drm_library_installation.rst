DRM Library installation
========================

This section explains how to install the Accelize DRM library API from packages and from source.

.. _supported_os:

Supported OS
------------

Accelize provides the DRM library packages for a wide variety of Linux OS.

Following OS are fully tested, including FPGA based hardware and end to end
tests:

* RHEL/Centos 7
* Ubuntu 18.04 LTS Bionic

Following OS are minimally tested only (without hardware):

* RHEL/Centos 8
* Ubuntu 20.04 LTS Focal
* Fedora (2 latest stable versions)
* Ubuntu (2 latest stable versions)
* Debian 10 Buster

.. note:: We limit hardware tested OS to those which are supported by FPGA
          vendors. But, we do minimal tests on more OS to ensure to our
          customers that they will be able to install the library on almost any
          Linux OS and that our library is future-proof and will be compatible
          without efforts to any OS that will be supported in the future by FPGA
          vendors.

Software requirements
---------------------

* The Accelize DRM C/C++ library requires a compiler with full C++11 support.
* The Accelize DRM Python library requires Python >= 3.6.


Installation from packages
--------------------------

The DRM library packages are available only for the **amd64** / **x86_64**
architecture.


Install Accelize repository
```````````````````````````

Packages are hosted on the Accelize repository.

.. note:: Packages and repositories metadata are signed for security.

Debian, Ubuntu: DEB repository
::::::::::::::::::::::::::::::

To install the repository, run the following commands:

.. code-block:: bash

    # Ensure common system utilities are installed
    sudo apt update
    sudo apt install -y apt-transport-https software-properties-common lsb-release gnupg curl

    # Add Accelize GPG public key for package signature verification
    curl -fsSL https://tech.accelize.com/gpg | sudo apt-key add -

    # Install repository
    sudo add-apt-repository "deb https://tech.accelize.com/deb $(lsb_release -cs) stable"
    sudo apt update


RHEL, CentOS, Fedora: RPM repository
::::::::::::::::::::::::::::::::::::

To install the Accelize repository, run the following commands:

.. code-block:: bash
    :caption: On Fedora, RHEL 8, CentOS 8

    # Ensure config manager is installed
    sudo dnf install -y 'dnf-command(config-manager)'

    # Install repository
    sudo dnf config-manager --add-repo https://tech.accelize.com/rpm/accelize_stable.repo

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure that config manager is installed
    sudo yum install -y yum-utils

     # Install Accelize repository:
    sudo yum-config-manager --add-repo https://tech.accelize.com/rpm/accelize_stable.repo


Python Library package
``````````````````````

This section explains how to install the DRM library as a python module.
It is available for Python 3 only.

This package provides 3 levels of integration:

* The Accelize DRM library Python
* Cython headers to the C/C++ Accelize DRM library.

Depending on your OS, use the following command to install the Python package:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y python3-accelize-drm

.. code-block:: bash
    :caption: On RHEL 8, CentOS 8

    # Ensure EPEL repository is installed
    sudo dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

    # Install package
    sudo dnf install -y python3-accelize-drm

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure EPEL repository is installed
    sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm

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
    sudo dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

    # Install package
    sudo dnf install -y libaccelize-drm

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure EPEL repository is installed
    sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm

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
    sudo dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

    # Install package
    sudo dnf install -y libaccelize-drm-devel

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure EPEL repository is installed
    sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm

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

.. warning:: The installation from source is only recommended if there
             is no package available for your configuration.

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

 * Python >= 3.6
 * Python-devel
 * setuptools
 * wheel
 * cython >= 0.28

Run following commands to install all requirements:

.. code-block:: bash
    :caption: On Debian >= 10, Ubuntu >= 20.04

    # Minimal requirements
    sudo apt update
    sudo apt install -y git make g++ libcurl4-openssl-dev libjsoncpp-dev pkg-config cmake

    # Python library requirements
    sudo apt install -y python3-dev python3-wheel python3-setuptools cython3

.. code-block:: bash
    :caption: On Debian < 10, Ubuntu < 20.04

    # Minimal requirements
    sudo apt update
    sudo apt install -y git make g++ libcurl4-openssl-dev libjsoncpp-dev pkg-config python3-pip
    python3 -m pip install --user -U pip
    pip3 install --user -U cmake

    # Python library requirements
    sudo apt install -y python3-dev python3-wheel python3-setuptools
    pip3 install --user -U cython

.. code-block:: bash
    :caption: On RHEL 8, CentOS 8

    # Ensure EPEL repository is installed
    sudo dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

    # Minimal requirements
    sudo dnf install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel python3-pip
    python3 -m pip install --user -U pip
    pip3 install --user -U cmake

    # Python library requirements
    sudo dnf config-manager --set-enabled PowerTools
    sudo dnf install -y python3-devel python3-setuptools python3-Cython python3-wheel

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure EPEL repository is installed
    sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm

    # Minimal requirements
    sudo yum install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel python3-pip
    python3 -m pip install --user -U pip
    pip3 install --user -U cmake

    # Python library requirements
    sudo yum install -y python3-devel python3-setuptools python3-wheel
    pip3 install --user -U cython

.. code-block:: bash
    :caption: On Fedora

    # Minimal requirements
    sudo dnf install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel cmake

    # Python library requirements
    sudo dnf install -y python3-devel python3-setuptools python3-Cython python3-wheel

Then run the following commands to build and install the library:

.. code-block:: bash

    git clone https://github.com/Accelize/drm.git --recursive --depth 1
    mkdir -p drm/build
    cd drm/build

    # The "-DPYTHON3=ON" option is required only to build the Python library
    cmake -DPYTHON3=ON ..

    make -j
    sudo make install

Build packages
``````````````

If you need to build packages to easily deploy the library on your
production environment, you need to install additional requirements:

For DEB Packages:

* dpkg-dev
* file

For RPM packages:

* rpm-build

Run following commands to install requirements:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y dpkg-dev file

.. code-block:: bash
    :caption: On Fedora, RHEL 8, CentOS 8

    sudo dnf install -y rpm-build

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    sudo yum install -y rpm-build

Once dependencies are installed, simply run the following section:

.. code-block:: bash

    git clone https://github.com/Accelize/drm.git --recursive --depth 1
    mkdir -p drm/build
    cd drm/build

    # The "-DPKG=ON" option is required to build the package
    cmake -DPYTHON3=ON -DPKG=ON ..

    make -j
    sudo make package

Packages will be generated in the `drm/build/packages` directory.

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

Uninstallation
--------------

This section explains how to uninstall the Accelize DRM library.

From packages
`````````````

To uninstall the Accelize DRM library when installed from packages,
simply run the following commands:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt-get purge --auto-remove -y libaccelize-drm libaccelize-drm-dev python3-accelize-drm

.. code-block:: bash
    :caption: On Fedora, RHEL 8, CentOS 8

    sudo dnf erase -y libaccelize-drm libaccelize-drm-devel python3-accelize-drm

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    sudo yum erase -y libaccelize-drm libaccelize-drm-devel python3-accelize-drm

From sources
````````````

To uninstall the Accelize DRM library when installed from sources:

* First go back in the directory where you cloned the Accelize DRM repository.

* Then, move in the previously created `build` directory:

.. code-block:: bash

    cd build

* Finally, uninstall files and directories

  - For DRM library version >= 2.5.0, using the uninstall target:

    .. code-block:: bash

        sudo make uninstall

  - For older version, using the CMake installation manifest:

    .. code-block:: bash

       for file in install_manifest*.txt
       do
           for name in $(cat $file)
           do
               sudo rm -f "$name"
               sudo rmdir -p --ignore-fail-on-non-empty "$(dirname "$name")"
           done
       done

You may also uninstall packages you have installed to build the Accelize DRM.

From Ansible
````````````

When installed using Ansible with default parameters, the uninstallation
method is the same as from packages.

If `accelize_drm_from_source` was set to `true` and `accelize_drm_git_clone` was
specified the uninstallation method is the same as from sources.
Commands must be run from the `accelize_drm_git_clone` directory in this case.


Manual clean up
```````````````

.. warning:: This method is only recommanded if the previous methods are not possibles.

To remove the Accelize DRM manually, run the following:

.. code-block:: bash

    # Remove C/C++ library
    sudo rm -f /usr/local/lib/libaccelize_drm*
    sudo rm -f /usr/local/lib64/libaccelize_drm*
    sudo rm -f /usr/lib/libaccelize_drm*
    sudo rm -f /usr/lib64/libaccelize_drm*

    # Remove C/C++ library headers and license
    sudo rm -rf /usr/local/include/accelize
    sudo rm -rf /usr/include/accelize
    sudo rm -rf /usr/local/share/licenses/accelize
    sudo rm -rf /usr/share/licenses/accelize

    # Remove Python package
    for name in $(sudo python3 -c "import sys;print('\\n'.join(path for path in sys.path if path))")
    do
        sudo rm -rf "$name/accelize_drm"
        sudo rm -rf "$name/python_accelize_drm"*
    done

Some parts of this command may fail. This script tries to remove the Accelize DRM at
different possible installation locations.
