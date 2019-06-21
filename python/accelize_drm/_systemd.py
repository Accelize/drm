#! /usr/bin/env python3
# coding=utf-8
"""Accelize DRM systemd Service"""
from concurrent.futures import (
    ThreadPoolExecutor as _ThreadPoolExecutor, as_completed as _as_completed)
from contextlib import contextmanager as _contextmanager
from json import load as _load
from os import environ as _environ, getenv as _getenv
from os.path import (
    expanduser as _expanduser, realpath as _realpath, isfile as _isfile)
import signal as _signal
from socket import (
    socket as _socket, AF_UNIX as _AF_UNIX, SOCK_DGRAM as _SOCK_DGRAM)
from time import sleep as _sleep

from accelize_drm import DrmManager as _DrmManager
from accelize_drm.exceptions import DRMException as _DRMException
from accelize_drm.fpga_drivers import get_driver as _get_driver

__all__ = ['AccelizeDrmService', 'run_service']


class AccelizeDrmService:
    """
    Accelize DRM systemd service

    Not intended to be called directly, use "systemctl" to manage the
    "accelize_drm_service" service.
    """

    #: Default configuration file path
    DEFAULT_CONF_FILE_PATH = '/etc/accelize_drm/conf.json'

    #: Default credentials file path
    DEFAULT_CRED_FILE_PATH = '~/.accelize_drm/cred.json'

    #: Default FPGA driver
    DEFAULT_FPGA_DRIVER_NAME = 'aws_f1'

    #: Default FPGA slot ID
    DEFAULT_FPGA_SLOT_ID = '0'

    #: Default DRM controller base address in FPGA design
    DEFAULT_DRM_CTRL_BASE_ADDR = 0

    def __init__(self):
        # Avoid double __exit__ / __del__ call
        self._activated = False

        # Handle SIGTERM like SIGINT (systemd stop services using SIGTERM)
        _signal.signal(_signal.SIGTERM, self._interrupt)

        # Get Systemd notify socket
        self._sd_notify_address = self._get_sd_notify_socket()

        # Get FPGA slots configuration
        self._fpga_slots = dict()

        for env_key in _environ:
            for env, key in (('ACCELIZE_DRM_DRIVER_', 'fpga_driver_name'),
                             ('ACCELIZE_DRM_CRED_', 'cred_file_path'),
                             ('ACCELIZE_DRM_CONF_', 'conf_file_path'),
                             ('ACCELIZE_DRM_IMAGE_', 'fpga_image'),):

                if env_key.startswith(env):
                    slot = env_key.rsplit('_', maxsplit=1)[1]

                    try:
                        slot_dict = self._fpga_slots[slot]
                    except KeyError:
                        # Add slot to configuration
                        self._fpga_slots[slot] = slot_dict = dict()

                    slot_dict[key] = _environ[env_key]

        if not self._fpga_slots:
            # If no configuration passed by environment, activate default slot
            self._fpga_slots[self.DEFAULT_FPGA_SLOT_ID] = dict()

        # Initialize DRM manager
        self._drivers = []
        self._drm_managers = []

        futures = []
        with _ThreadPoolExecutor() as executor:
            for fpga_slot_id in self._fpga_slots:
                futures.append(executor.submit(
                    self._init_drm_manager, int(fpga_slot_id)))

        with self._handle_exception((RuntimeError, OSError, _DRMException)):
            for future in _as_completed(futures):
                future.result()

    def _init_drm_manager(self, fpga_slot_id):
        """
        Initialize a DRM manager for the specified FPGA slot.

        Args:
            fpga_slot_id (int): FPGA slot
        """
        # Get slot configuration
        slot_config = self._fpga_slots[fpga_slot_id]

        conf_file_path = self._check_path(slot_config.get(
            'conf_file_path', self.DEFAULT_CONF_FILE_PATH))
        cred_file_path = self._check_path(slot_config.get(
            'cred_file_path', self.DEFAULT_CRED_FILE_PATH))
        fpga_driver_name = slot_config.get(
            'fpga_driver_name', self.DEFAULT_FPGA_DRIVER_NAME)
        fpga_image = slot_config.get('fpga_image')

        self._sd_log('FPGA slot %s, configuration file: %s',
                     fpga_slot_id, conf_file_path)
        self._sd_log('FPGA slot %s, credential file: %s',
                     fpga_slot_id, cred_file_path)
        self._sd_log('FPGA slot %s, Driver: %s',
                     fpga_slot_id, fpga_driver_name)

        # Get configuration files
        with open(conf_file_path, 'rt') as conf_file:
            conf = _load(conf_file)

        drm_conf = conf.get('drm', dict())

        # Get FPGA parameters from configuration
        drm_ctrl_base_addr = drm_conf.get(
            'drm_ctrl_base_addr', self.DEFAULT_DRM_CTRL_BASE_ADDR)

        # Get FPGA driver
        driver = _get_driver(name=fpga_driver_name)(
            fpga_slot_id=fpga_slot_id,
            fpga_image=fpga_image,
            drm_ctrl_base_addr=drm_ctrl_base_addr)
        self._drivers.append(driver)

        # Initialize DRM manager
        self._drm_managers.append(_DrmManager(
            conf_file_path=conf_file_path,
            cred_file_path=cred_file_path,
            read_register=driver.read_register_callback,
            write_register=driver.write_register_callback))

    def __enter__(self):
        self._activated = True

        # Activate DRM manager for all slots
        futures = []
        with _ThreadPoolExecutor() as executor:
            for drm_manager in self._drm_managers:
                futures.append(executor.submit(drm_manager.activate))

        with self._handle_exception(_DRMException):
            for future in _as_completed(futures):
                future.result()

        # Notify systemd
        self._sd_notify(b"READY=1\nSTATUS=Licensing FPGA slot(s) %s" %
                        str(sorted(self._fpga_slots)).strip('[]').encode())

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self._activated:
            self._activated = False
            self._sd_notify(b"STOPPING=1")

            # Deactivate DRM manager for all slots
            futures = []
            with _ThreadPoolExecutor() as executor:
                for drm_manager in self._drm_managers:
                    futures.append(executor.submit(drm_manager.deactivate))
            try:
                with self._handle_exception(_DRMException):
                    for future in _as_completed(futures):
                        future.result()

            finally:
                self._drivers.clear()
                self._drm_managers.clear()

    def __del__(self):
        self.__exit__(None, None, None)

    def _check_path(self, path):
        """
        Check path and ensure it is absolute.

        Args:
            path (str): path

        Returns:
            str: Absolute path
        """
        with self._handle_exception(OSError):
            path = _realpath(_expanduser(path))
            if not _isfile(path):
                raise FileNotFoundError('No such file: ' + path)
        return path

    @staticmethod
    def _interrupt(*_, **__):
        """
        Interrupt.

        Raises:
            KeyboardInterrupt
        """
        raise KeyboardInterrupt()

    @_contextmanager
    def _handle_exception(self, exception_types):
        """
        Handle exception and exit with parser.

        Args:
            exception_types (Exception subclass or tuple of Exception subclass):
                Exceptions types to handle.
        """
        try:
            yield
        except exception_types as exception:
            self._sd_notify(b"STATUS=Error")
            self._sd_log(str(exception), level=3)
            self._interrupt()

    @staticmethod
    def _get_sd_notify_socket():
        """
        Get systemd notify socket address.

        Returns:
            str: Systemd notify socket address.
        """
        address = _getenv('NOTIFY_SOCKET', '')
        if address.startswith('@'):
            address = '\0%s' % address[1:]
        return address

    def _sd_notify(self, status):
        """
        Notify systemd.

        Args:
            status (bytes): Notification message.
        """
        try:
            with _socket(_AF_UNIX, _SOCK_DGRAM) as notify_socket:
                notify_socket.connect(self._sd_notify_address)
                notify_socket.sendall(status)
        except OSError:
            # Send nothing if socket disabled
            pass

    @staticmethod
    def _sd_log(message, *args, level=6):
        """
        Log in systemd

        Args:
            message (str): Message to log
            args: Message args.
            level (int): Systemd log level

        Levels:
            7: DEBUG
            6: INFO
            5: NOTICE
            4: WARNING
            3: ERR
            2: CRIT
            1: ALERT
            0: EMERG
        """
        # Systemd log messages must be one single line
        print('<%d>%s' % (level, (message % args).replace('\n', '\\n')))


def run_service():
    """
    Run the service
    """
    with AccelizeDrmService():
        try:
            while True:
                _sleep(1)
        except KeyboardInterrupt:
            # Exit Gracefully with __exit__
            return


if __name__ == '__main__':
    run_service()
