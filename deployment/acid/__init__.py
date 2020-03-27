# coding=utf-8
"""
Accelize utilities for CI/CD pipeline
"""
from os.path import basename as _basename, dirname as _dirname, join as _join


def _call(command, **kwargs):
    """
    Call a command with automatic error handling.

    Args:
        command (iterable of str or str):
        kwargs: subprocess.run keyword arguments.

    Returns:
        subprocess.CompletedProcess
    """
    from subprocess import run, PIPE
    process = run(command, universal_newlines=True, stderr=PIPE, **kwargs)

    if process.returncode:
        if process.stderr:
            msg = f'\nStderr messages:\033[30m\n{process.stderr}'
        else:
            msg = '\033[30m'
        raise RuntimeError(
            f"\033[31mError code: {process.returncode}{msg}")

    return process


def ensure_pip_packages(*packages):
    """
    Ensure pip packages are installed.

    Args:
        *packages (str): Package to install.
    """
    _call(['pip', 'install', '--disable-pip-version-check', '-q'] +
          list(packages))


def export(name, value, is_output=True):
    """
    Set a Azure pipeline variable.

    Args:
        name (str): Variable name.
        value (str): Variable value.
        is_output (bool): Make variable available to future jobs.
    """
    print(f'##vso[task.setvariable variable={name}'
          f'{";isOutput=true" if is_output else ""}]{value}')


def render_template(src, dst, show=True, **kwargs):
    """
    Render a file from a template using Jinja2.

    Args:
        src (str): Source template.
        dst (str): Destination file.
        show (bool): If true, print result.
        kwargs: Template arguments.
    """
    ensure_pip_packages('jinja2')

    from jinja2 import Environment, FileSystemLoader
    env = Environment(loader=FileSystemLoader(_dirname(src)))
    template = env.get_template(_basename(src))
    rendered = template.render(**kwargs)
    if show:
        print('\033[34m== START RENDERED ==\033[30m\n'
              f'{rendered}'
              '\n\033[34m== END RENDERED ==\033[30m')
    with open(dst, 'wt') as file:
        file.write(rendered)


def ensure_ansible_roles(path):
    """
    Ensure playbook dependencies roles are presents.

    Args:
        path (str): "playbook.yml" path.
    """
    ensure_pip_packages('pyyaml')
    try:
        from yaml import CSafeLoader as Loader
    except ImportError:
        from yaml import SafeLoader as Loader
    from yaml import load

    # Get dependencies from playbook
    try:
        with open(path, 'rt') as file:
            roles = load(file, Loader=Loader)[0]['roles']
    except KeyError:
        # No roles
        return
    dependencies = set()
    for role in roles:
        try:
            # formatted as - name: role_name
            name = role['name']
        except KeyError:
            # Formatted as - role_name
            name = role
        if "." in name:
            dependencies.add(name)

    # Install dependencies
    if dependencies:
        _call(['ansible-galaxy', 'install'] + list(dependencies))


def define_unique_name(project, repo, branch, build_id):
    """
    Name an unique name based on build information.

    Args:
        project (str): Project name.
        repo (str): Repository name.
        branch (str): Branch name.
        build_id (str): Build ID.

    Returns:
        str: Unique name.
    """
    from secrets import token_hex

    name = ''.join(c for c in ''.join((
        project.capitalize(),
        repo.split('/')[-1].capitalize(),
        branch.capitalize(),
        build_id.capitalize(),
        token_hex(8)
    )) if c.isalnum())

    print('\n'.join((
        f'Project: {project}',
        f'Repository: {repo}',
        f'Branch: {branch}',
        f'Build ID: {build_id}',
        f'Resources Name: {name}')))

    return name


def dump_tfvars(directory, **variables):
    """
    Dump variables in "terraform.tfvars.json".
    Update existing files.

    Args:
        directory (str): Output directory.
        **variables: Variables
    """
    from json import dump, load
    path = _join(directory, 'terraform.tfvars.json')

    try:
        with open(path, 'rt') as json_file:
            tfvars = load(json_file)
    except FileNotFoundError:
        tfvars = dict()

    tfvars.update(variables)
    with open(path, 'wt') as json_file:
        dump(tfvars, json_file)
