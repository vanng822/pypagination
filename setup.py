
import os
from setuptools import setup, find_packages
from distutils.core import Extension

pagination = Extension('pagination', sources = ['src/pagination.c']) 

here = os.path.abspath(os.path.dirname(__file__))


setup(name='pagination',
      version='0.1',
      author='',
      author_email='',
      url='',
      packages=find_packages(),
      test_suite='tests',
      ext_modules = [pagination],
)