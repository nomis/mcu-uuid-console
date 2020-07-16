# -*- coding: utf-8 -*-

import os

travis_ci = os.environ.get("TRAVIS") == 'true'

needs_sphinx = '1.3'
extensions = []
source_suffix = ['.rst']

project = u'mcu-uuid-console'
copyright = u'2019, Simon Arlott'
author = u'Simon Arlott'

master_doc = 'index'

version = u''
release = u''

language = None
exclude_patterns = ['build', 'Thumbs.db', '.DS_Store']
pygments_style = 'sphinx'
highlight_language = 'c++'
todo_include_todos = False

linkcheck_timeout = 60
linkcheck_ignore = [r'https://github.com/.+/.+/(compare|commits)/.+']
