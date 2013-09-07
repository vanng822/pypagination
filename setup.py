
import os
from setuptools import setup, find_packages
from distutils.core import Extension

pagination = Extension('pagination', sources = ['src/pagintation.c']) 

here = os.path.abspath(os.path.dirname(__file__))


setup(name='pagintation',
      version='0.1',
      author='',
      author_email='',
      url='',
      packages=find_packages(),
      test_suite='tests',
      ext_modules = [pagination],
)