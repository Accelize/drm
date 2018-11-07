# -*- coding: utf-8 -*-
"""Sphinx documentation """

# -- Path setup --------------------------------------------------------------

import os
import subprocess
from os.path import abspath, dirname
import sys



# -- Project information -----------------------------------------------------

project = "drmlib"
copyright = "Accelize"
author = "jeydoux@accelize.com"
version = "v1.1"
release = "v1.1"


sys.path.append( "/home/me/docproj/ext/breathe/" )


read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

if read_the_docs_build:

    subprocess.call('pip install breathe', shell=True)
    subprocess.call('doxygen Doxyfile.in', shell=True)  
   
breathe_projects = {
    "drmlib":"xml/"
    }

extensions = [ 'breathe']

source_suffix = '.rst'
master_doc = 'index'
language = 'en'
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', '.settings']
pygments_style = 'default'


# -- Options for HTML output -------------------------------------------------


html_theme = 'sphinx_rtd_theme'

html_favicon = '_static/favicon.ico'

html_context = {
    'css_files': ['_static/accelize.css'],  # Overwrite them style
}

html_logo = '_static/logo.png'

html_show_sourcelink = False

html_show_sphinx = False

# -- Options for HTMLHelp output ---------------------------------------------

htmlhelp_basename = '%sdoc' % project

# -- Options for LaTeX output ------------------------------------------------

latex_elements = {}
latex_documents = [(
    master_doc, '%s.tex' % project, '%s Documentation' % project, author,
    'manual')]

latex_logo = '_static/logo.png'
# -- Options for manual page output ------------------------------------------

man_pages = [(
    master_doc, "my_name", '%s Documentation' % project,
    [author], 1)]


# -- Options for Texinfo output ----------------------------------------------

texinfo_documents = [(
    master_doc, project, '%s Documentation' % project, author, project,
    "my_description", 'Miscellaneous')]
