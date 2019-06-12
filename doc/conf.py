# -*- coding: utf-8 -*-
"""Sphinx documentation"""
# -- Path setup --------------------------------------------------------------

from os.path import abspath, dirname, join
from datetime import datetime
import sys

SETUP_PATH = abspath(dirname(dirname(__file__)))
sys.path.insert(0, join(SETUP_PATH, '@PYTHON3_BDIST_RELATIVE@'))


# -- Project information -----------------------------------------------------

project = "Accelize DRM"
author = "@CPACK_PACKAGE_VENDOR@"
copyright = '2018-%s, %s' % (datetime.now().year, author)
version = release = "@ACCELIZEDRM_VERSION@"


# -- General configuration ---------------------------------------------------

extensions = ['sphinx.ext.autodoc', 'sphinx.ext.napoleon', 'breathe',
              'sphinx.ext.todo']
source_suffix = '.rst'
master_doc = 'index'
language = 'en'
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']
pygments_style = 'default'
breathe_projects = {"accelize_drm": "_doxygen"}
templates_path = ['_templates']
todo_emit_warnings = True

# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_theme_options = {
    'prev_next_buttons_location': None
}

# Enable Accelize Theme
html_favicon = '_static/favicon.ico'
html_logo = '_static/logo.png'
html_show_sourcelink = False
html_show_sphinx = False
html_context = {'css_files': ['_static/accelize.css']}


# -- Options for HTMLHelp output ---------------------------------------------

htmlhelp_basename = '%sdoc' % project


# -- Options for LaTeX output ------------------------------------------------

latex_elements = {}
latex_documents = [(
    master_doc, '%s.tex' % project, '%s Documentation' % project, author,
    'manual')]


# -- Options for manual page output ------------------------------------------

man_pages = [(
    master_doc, project, '%s Documentation' % project,
    [author], 1)]


# -- Options for Texinfo output ----------------------------------------------

texinfo_documents = [(
    master_doc, project, '%s Documentation' % project, author, project,
    project, 'Miscellaneous')]
