# coding=utf-8
"""Test Accelize DRM systemd service"""
import pytest
from tests.conftest import perform_once


def test_systemd(conf_json, cred_json, tmpdir):
    """Test Accelize DRM systemd service"""
    perform_once(__name__ + '.test_systemd')

    from os import environ
    from socket import socket, AF_UNIX, SOCK_DGRAM
    from threading import Thread
    from time import sleep, time
    import accelize_drm._systemd as systemd
    from accelize_drm._systemd import AccelizeDrmService

    # Set FPGA slot
    fpga_slot_id = "1"

    # Set configuration files
    conf_env_var = 'ACCELIZE_DRM_CONF_%s' % fpga_slot_id
    cred_env_var = 'ACCELIZE_DRM_CRED_%s' % fpga_slot_id
    conf_file_path = conf_json.path
    cred_file_path = cred_json.path
    environ[conf_env_var] = conf_file_path
    environ[cred_env_var] = cred_file_path
    default_conf_file_path = AccelizeDrmService.DEFAULT_CONF_FILE_PATH
    default_cred_file_path = AccelizeDrmService.DEFAULT_CRED_FILE_PATH

    # Set FPGA driver to use in configuration
    driver_env_var = 'ACCELIZE_DRM_DRIVER_%s' % fpga_slot_id
    fpga_driver_name = 'driver_name'
    environ[driver_env_var] = fpga_driver_name

    # Mock systemd notify socket
    socket_file = tmpdir.join('sd_notify')
    socket_address = environ['NOTIFY_SOCKET'] = str(socket_file)
    socket_received = []
    socket_stop = False

    class SdNotifySocketThread(Thread):
        """A thread running a UDP socket server"""

        def run(self):
            """Create a connection"""
            with socket(AF_UNIX, SOCK_DGRAM) as sock:
                sock.bind(socket_address)
                sock.setblocking(False)
                t0 = time()
                while True:
                    # Stop loop
                    if socket_stop:
                        break

                    # Timeout to stop test if something is wrong
                    elif time() - t0 > 20:
                        raise TimeoutError()

                    # Wait until error
                    try:
                        socket_received.append(sock.recv(4096))
                    except OSError:
                        continue

    # Mock driver and DRM library

    class Driver:
        """Mocked accelize_drm.fpga_drivers Driver"""
        def __init__(self, **kwargs):
            assert kwargs['fpga_slot_id'] == fpga_slot_id, \
                "Driver: Slot ID"
            assert 'drm_ctrl_base_addr' in kwargs, "Driver: Base address"

        read_register_callback = None
        write_register_callback = None

    def _get_driver(name):
        """Mocked accelize_drm.fpga_drivers.get_driver"""
        assert name == fpga_driver_name, 'Get driver: Driver name'
        return Driver

    class DrmManager:
        """Mocked accelize_drm.fpga_drivers Driver"""
        def __init__(self, **kwargs):
            assert kwargs['conf_file_path'] == conf_file_path, \
                'DRM manager: conf.json'
            assert kwargs['cred_file_path'] == cred_file_path, \
                'DRM manager: cred.json'
            assert 'read_register' in kwargs, 'DRM manager: read_register'
            assert 'write_register' in kwargs, 'DRM manager: write_register'

        @staticmethod
        def activate():
            """Do nothing"""

        @staticmethod
        def deactivate():
            """Do nothing"""

    systemd_get_driver = systemd._get_driver
    systemd._get_driver = _get_driver
    systemd_drm_manager = systemd._DrmManager
    systemd._DrmManager = DrmManager

    # Tests
    try:
        sd_notify_thread = SdNotifySocketThread()
        try:
            sd_notify_thread.start()

            # Test: Start and stop service
            with AccelizeDrmService() as service:
                # Checks some parameters
                assert service._sd_notify_address == socket_address
                assert list(service._fpga_slots) == [fpga_slot_id]
                assert service._fpga_slots[fpga_slot_id][
                    'conf_file_path'] == conf_json.path
                assert service._fpga_slots[fpga_slot_id][
                    'cred_file_path'] == cred_json.path
                assert service._fpga_slots[fpga_slot_id][
                    'fpga_driver_name'] == fpga_driver_name

            # Test: systemd notification
            sleep(0.1)
            assert socket_received[0].startswith(b'READY=1\nSTATUS=')
            assert socket_received[-1].startswith(b"STOPPING=1")

            # Test: Bad path and error handling
            environ[conf_env_var] = str(tmpdir.join('not_exists'))

            with pytest.raises(KeyboardInterrupt):
                AccelizeDrmService()

            sleep(0.1)
            assert socket_received[-1].startswith(b"STATUS=Error")

        finally:
            socket_stop = True
            sd_notify_thread.join()
            del environ['NOTIFY_SOCKET']
            environ[conf_env_var] = conf_json.path
            socket_file.remove()

        # Test: Socket address starting by @
        fake_address = tmpdir.join('sd_notify_not_exists')
        environ['NOTIFY_SOCKET'] = '@%s' % fake_address
        try:
            assert AccelizeDrmService._get_sd_notify_socket() == \
                   '\0%s' % fake_address
        finally:
            del environ['NOTIFY_SOCKET']

        # Test: Broken socket should not break service
        AccelizeDrmService()

        # Test: Default values
        for key in (conf_env_var, cred_env_var, driver_env_var):
            del environ[key]
        fpga_slot_id = AccelizeDrmService.DEFAULT_FPGA_SLOT_ID
        fpga_driver_name = AccelizeDrmService.DEFAULT_FPGA_DRIVER_NAME
        AccelizeDrmService.DEFAULT_CONF_FILE_PATH = conf_file_path
        AccelizeDrmService.DEFAULT_CRED_FILE_PATH = cred_file_path

        with AccelizeDrmService() as service:
            # Checks some parameters
            assert list(service._fpga_slots) == [fpga_slot_id]
            assert service._fpga_slots[fpga_slot_id] == {}

    except KeyboardInterrupt:
        pytest.fail('Service stopped by "KeyboardInterrupt"')

    finally:
        systemd._get_driver = systemd_get_driver
        systemd._DrmManager = systemd_drm_manager
        AccelizeDrmService.DEFAULT_CONF_FILE_PATH = default_conf_file_path
        AccelizeDrmService.DEFAULT_CRED_FILE_PATH = default_cred_file_path
