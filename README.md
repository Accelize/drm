![Screenshot](images/AccelDRM_lock.png)

* On **Master** branch: ![Build Status](https://codebuild.eu-west-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoiVXRTOGxGaW5hZ1BzVGtoa0pBODhPanFlL3RpbFJPeGo5dHNxN3Uwb1ZuUmZOc1VkejBOeGk2WGVodzZPK2NvdS9WWVZUSlJhZXFMbEgrejN1VDN6TE13PSIsIml2UGFyYW1ldGVyU3BlYyI6IkhtMFRSeW1aRzVWVUpZcVQiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=master)
[![codecov](https://codecov.io/gh/Accelize/drmlib/branch/master/graph/badge.svg)](https://codecov.io/gh/Accelize/drmlib)
* On **Dev** branch: ![Build Status](https://codebuild.eu-west-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoiVXRTOGxGaW5hZ1BzVGtoa0pBODhPanFlL3RpbFJPeGo5dHNxN3Uwb1ZuUmZOc1VkejBOeGk2WGVodzZPK2NvdS9WWVZUSlJhZXFMbEgrejN1VDN6TE13PSIsIml2UGFyYW1ldGVyU3BlYyI6IkhtMFRSeW1aRzVWVUpZcVQiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=dev)
  [![codecov](https://codecov.io/gh/Accelize/drmlib/branch/dev/graph/badge.svg)](https://codecov.io/gh/Accelize/drmlib)

# Accelize DRM library : AccelDRM :closed_lock_with_key:

The Accelize DRM solution protects FPGA applications.

The Accelize DRM library operates the DRM from the software side of the
application.

This library is responsible for activating the DRM, communicating with the
Accelize web service and managing metering sessions.

This library provides **API** for : 
* C
* C++
* Python

Interested in AccelDRM ? Checkout our [Documentation](http://accelize.s3-website-eu-west-1.amazonaws.com/documentation/stable)!

# Accelize licensing solution :key:

The Accelize licensing technology is offered in two distinct modes:

* Static licensing : 
A file-based scheme implemented by statically packaging the license key into an 
encrypted license file, stored locally on the server that hosts the FPGA card.

* Dynamic licensing :
A server-based scheme implemented by delivering license keys from a license server. 
Specifically, the license server delivers a regular stream of time-limited single-use license keys.

Take a look at our [documentation](http://accelize.s3-website-eu-west-1.amazonaws.com/documentation/stable/#licensing-modes) for more information !

## Accelize Distribution Platform :computer:

For a quick tour about the Accelize Platform integration steps, please watch [this video](https://youtu.be/7cb_ksLTcRk)

## Prerequisites :heavy_exclamation_mark:

To access the Accelize Web Service you need an Internet connection which allows 
outbound HTTPS connection to Accelize [server](https://master.metering.accelize.com)

## License :registered:

Please consult [license](LICENSE)

## Changelog :floppy_disk:

Please consult [CHANGELOG](CHANGELOG)
