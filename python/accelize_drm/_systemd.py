#! /usr/bin/env python3
# coding=utf-8
"""
Accelize DRM systemd Service

Global configuration environment variables:
    ACCELIZE_DRM_DEFAULT_FPGA_SLOT_ID: Default FPGA slot ID if no slot specific
        configuration is passed. Default to 0. Can be disabled by passing an
        empty environment variable.
    ACCELIZE_DRM_LOG_FILE_BASE_PATH: Template path for log files. must contain
        one "%d" that will be replaced by slot ID. Default to
        "/var/log/accelize_drm/service_slot_%d.log"
    ACCELIZE_DRM_CACHE_DIR: Service cache dir.
        Used to store cached FPGA images.
        Default to "/var/cache/accelize_drm".
"""
from concurrent.futures import (
    ThreadPoolExecutor as _ThreadPoolExecutor, as_completed as _as_completed,
    wait as _wait)
from contextlib import contextmanager as _contextmanager
from json import load as _load
from os import (environ as _environ, getenv as _getenv, makedirs as _makedirs,
                chmod as _chmod)
from os.path import (expanduser as _expanduser, realpath as _realpath,
                     isfile as _isfile, dirname as _dirname, join as _join)
import signal as _signal
from socket import (
    socket as _socket, AF_UNIX as _AF_UNIX, SOCK_DGRAM as _SOCK_DGRAM)
from threading import Lock as _Lock
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
    DEFAULT_FPGA_DRIVER_NAME = 'xilinx_xrt'

    #: Default FPGA slot ID
    DEFAULT_FPGA_SLOT_ID = _environ.get(
        'ACCELIZE_DRM_DEFAULT_FPGA_SLOT_ID', '0') or None

    #: Default DRM controller base address in FPGA design
    DEFAULT_DRM_CTRL_BASE_ADDR = 0

    #: Default DRM library log verbosity
    DEFAULT_LOG_VERBOSITY = 2

    #: DRM library log files directory
    LOG_DIR = _environ.get('ACCELIZE_DRM_LOG_DIR',
                           '/var/log/accelize_drm')

    #: Service cache directory
    CACHE_DIR = _environ.get('ACCELIZE_DRM_CACHE_DIR',
                             '/var/cache/accelize_drm')

    def __init__(self):
        # Avoid double __exit__ / __del__ call
        self._activated = False

        # Threading locks
        self._locks = dict()

        # Handle SIGTERM like SIGINT (systemd stop services using SIGTERM)
        _signal.signal(_signal.SIGTERM, self._interrupt)

        # Get Systemd notify socket
        self._sd_notify_address = self._get_sd_notify_socket()

        # Ensure directories exists
        for dir_path in (self.CACHE_DIR, _dirname(self.LOG_DIR)):
            _makedirs(dir_path, exist_ok=True)
            _chmod(dir_path, 0o740)

        # Get FPGA slots configuration
        self._fpga_slots = dict()

        for env_key in _environ:
            for env, key in (('ACCELIZE_DRM_DRIVER_', 'fpga_driver_name'),
                             ('ACCELIZE_DRM_CRED_', 'cred_file_path'),
                             ('ACCELIZE_DRM_CONF_', 'conf_file_path'),
                             ('ACCELIZE_DRM_IMAGE_', 'fpga_image'),
                             ('ACCELIZE_DRM_DISABLED_', 'drm_disabled')):

                if env_key.startswith(env):
                    slot = int(env_key.rsplit('_', maxsplit=1)[1])

                    try:
                        slot_dict = self._fpga_slots[slot]
                    except KeyError:
                        # Add slot to configuration
                        self._fpga_slots[slot] = slot_dict = dict()

                    slot_dict[key] = _environ[env_key]

        if not self._fpga_slots and self.DEFAULT_FPGA_SLOT_ID is not None:
            # If no configuration passed by environment, activate default slot
            self._fpga_slots[int(self.DEFAULT_FPGA_SLOT_ID)] = dict()

        # Initialize DRM manager
        self._drivers = []
        self._drm_managers = []
        self._lisenced_slots = []

        futures = []
        with _ThreadPoolExecutor() as executor:
            for fpga_slot_id in self._fpga_slots:
                futures.append(executor.submit(
                    self._init_drm_manager, int(fpga_slot_id)))

        with self._handle_exception((RuntimeError, OSError, _DRMException)):
            for future in _as_completed(futures):
                future.result()

    def _get_image(self, fpga_image):
        """
        Return FPGA image.

        If the image is a remote URL, cache the image locally and return cached
        local path.

        Args:
            fpga_image (str): FPGA image.

        Returns:
            str: If fpga_image is an URL, return a local path to the file
                else, return fpga_image.
        """
        if not fpga_image or not (fpga_image.lower().startswith('http://') or
                                  fpga_image.lower().startswith('https://')):
            # FPGA Image is not an URL
            return fpga_image

        # Note: Lazy import "urllib" because may be never used
        from urllib.parse import urlparse
        from urllib.request import urlopen

        # Get URL and define local path
        url = urlparse(fpga_image)
        local_image = _join(self.CACHE_DIR, url.netloc, url.path.strip('/'))

        # Ensure remote FPGA image is cached locally
        with self._locks.setdefault(fpga_image, _Lock()):
            if not _isfile(local_image):

                self._sd_log('Caching remote FPGA image "%s" to "%s"',
                             fpga_image, local_image)

                _makedirs(_dirname(local_image), exist_ok=True)

                with open(local_image, 'wb') as local_file:
                    with urlopen(fpga_image) as remote_file:
                        local_file.write(remote_file.read())

        return local_image

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
        fpga_image = self._get_image(slot_config.get('fpga_image'))

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
            drm_ctrl_base_addr=drm_ctrl_base_addr,
            log_dir=self.LOG_DIR)
        self._drivers.append(driver)

        # Initialize DRM manager
        if not slot_config.get('drm_disabled'):
            drm_manager = _DrmManager(
                conf_file_path=conf_file_path,
                cred_file_path=cred_file_path,
                read_register=driver.read_register_callback,
                write_register=driver.write_register_callback)

            drm_manager.set(
                # Set rotating log file for each slot
                log_service_path=_join(
                    self.LOG_DIR, "slot_%d_drm_service.log" % fpga_slot_id),
                log_service_type=2,
                log_service_verbosity=int(_environ.get(
                    'ACCELIZE_DRM_LOG_VERBOSITY', self.DEFAULT_LOG_VERBOSITY)),

                # Disable Stdout log output
                log_verbosity=6,
            )
            drm_manager.set(log_service_create=True)

            self._lisenced_slots.append(str(fpga_slot_id))
            self._drm_managers.append(drm_manager)

            self._sd_log('FPGA slot %s, configuration file: %s',
                         fpga_slot_id, conf_file_path)
            self._sd_log('FPGA slot %s, credential file: %s',
                         fpga_slot_id, cred_file_path)

        self._sd_log('FPGA slot %s, driver: %s', fpga_slot_id,
                     fpga_driver_name)
        self._sd_log('FPGA slot %s, image: %s', fpga_slot_id, fpga_image)
        self._sd_log('FPGA slot %s, licensing: %s', fpga_slot_id, 'disabled'
                     if slot_config.get('drm_disabled') else 'enabled')

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
                        ', '.join(self._lisenced_slots).encode())

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self._activated:
            self._activated = False
            self._sd_notify(b"STOPPING=1")

            with _ThreadPoolExecutor() as executor:
                # Deactivate DRM manager for all slots
                futures = []
                for drm_manager in self._drm_managers:
                    futures.append(executor.submit(drm_manager.deactivate))
                _wait(futures)
                self._drm_managers.clear()

                # Reset all slots
                futures.clear()
                for driver in self._drivers:
                    futures.append(executor.submit(driver.reset_fpga))
                _wait(futures)
                self._drivers.clear()

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
    def _handle_exception(self, exc_types):
        """
        Handle exception and exit with parser.

        Args:
            exc_types (Exception subclass or tuple of Exception subclass):
                Exceptions types to handle.
        """
        try:
            yield
        except exc_types as exception:
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
    try:
        with AccelizeDrmService():
            while True:
                _sleep(1)
    except KeyboardInterrupt:
        # Exit Gracefully with __exit__
        return


if __name__ == '__main__':
    run_service()
