"""
This library provides some DMR library specific CI/CD pipeline utilities.
"""
from os.path import (
    basename as _basename,
    dirname as _dirname,
    realpath as _realpath,
    join as _join,
)

DEPLOYMENT_DIR = _realpath(_dirname(__file__))
PROJECT_DIR = _dirname(DEPLOYMENT_DIR)


def setvariable(name, value, is_output=True):
    """
    Set a Azure pipeline variable.

    Args:
        name (str): Variable name.
        value (str): Variable value.
        is_output (bool): Make variable available to future jobs.
    """
    print(
        f"##vso[task.setvariable variable={name}"
        f'{";isOutput=true" if is_output else ""}]{value}'
    )


def render_template(src, dst, show=True, **kwargs):
    """
    Render a file from a template using Jinja2.

    Args:
        src (str): Source template.
        dst (str): Destination file.
        show (bool): If true, print result.
        kwargs: Template arguments.
    """
    from jinja2 import Environment, FileSystemLoader

    env = Environment(loader=FileSystemLoader(_dirname(src)))
    template = env.get_template(_basename(src))
    rendered = template.render(**kwargs)
    if show:
        print(
            "\033[34m== START RENDERED ==\033[30m\n"
            f"{rendered}"
            "\n\033[34m== END RENDERED ==\033[30m"
        )
    with open(dst, "wt") as file:
        file.write(rendered)


def get_drm_version():
    """
    Return DRM library version.

    Returns:
        str: DRM library version.
    """
    path = _join(PROJECT_DIR, "CMakeLists.txt")
    with open(path, "rt") as cmakelists:
        for line in cmakelists:
            if line.startswith("set(ACCELIZEDRM_VERSION "):
                version = f"v{line.split(' ')[1].strip().strip(')')}"
                print(f"Detected DRM library version: {version}")
                return version
    raise ValueError(f'ACCELIZEDRM_VERSION not found in "{path}"')
