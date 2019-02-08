# Accelize DRMlib demo

Here is a demo of the C-API of the Accelize DRMlib

## Run this demo on AWS F1 instance

### Prerequisite

As prerequisite to this demo you need:
- a AWS F1 instance running Centos7.
- the [AWS FPGA SDK](https://github.com/aws/aws-fpga/blob/master/sdk/README.md) installed
- the DRMlib RPM (https://github.com/Accelize/drmlib/releases/latest) installed
- create an Accelize.com account and generate ``cred.json``.

### Make
From the untar provided archive build with :

```console
$make
```
This will create a demo executable

### Create your credentials file
cred.json that looks like this:
```json
{
  "client_id": "## your client id from Accelstore ##",
  "client_secret": "## your client id from Accelstore ##"
}
```

### Load demo design AGFI and XDMA drivers
```console
$sudo fpga-load-local-image -S0 -Iagfi-0e1caebbfe7ad747d
```

### Run

First run:

```console
$./demo
Found device: AWS FPGA Slot #0
[INFO] Starting metering session...
[INFO] Started new metering session with sessionId A876FD1EDE47765B and set first license with duration of 15 seconds
Sent packet of size 1048576 bytes with xor=0xFFE6
Received packet of size 1048576 bytes with xor=0xFFE6
...
Sent packet of size 1048576 bytes with xor=0xFF98
Received packet of size 1048576 bytes with xor=0xFF98
[INFO] Pausing metering session...
[INFO] Uploaded metering for sessionId A876FD1EDE47765B and set next license with duration of 15 seconds
```

Next run :
```console
$./demo
Found device: AWS FPGA Slot #0
[INFO] Resuming metering session...
[INFO] Resumed metering session with sessionId A876FD1EDE47765B and set first license with duration of 15 seconds
Sent packet of size 1048576 bytes with xor=0x26
Received packet of size 1048576 bytes with xor=0x26
...
Sent packet of size 1048576 bytes with xor=0xFF9C
Received packet of size 1048576 bytes with xor=0xFF9C
[INFO] Pausing metering session...
```

Last run (closing session):
```console
$./demo --stopSession
Found device: AWS FPGA Slot #0
[INFO] Resuming metering session...
[INFO] Resumed metering session with sessionId A876FD1EDE47765B and set first license with duration of 15 seconds
Sent packet of size 1048576 bytes with xor=0xFFE5
Received packet of size 1048576 bytes with xor=0xFFE5
...
Sent packet of size 1048576 bytes with xor=0x4F
Received packet of size 1048576 bytes with xor=0x4F
[INFO] Stopping metering session...
[INFO] Stopped metering session with sessionId A876FD1EDE47765B and uploaded last metering data

```

### Check your coin usage

Now you should see your coin usage on your AccelStore account on this [page](https://accelstore.accelize.com/user/metering).
This demo is designed to use 10 coins per execution.
Note that the usage will appear immediately only if you closed the session otherwise it will updated on regular period that corresponds to the licenses duration you see in the logs.
