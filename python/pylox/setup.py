from codecs import open as codecs_open
from setuptools import setup, find_packages


# Get the long description from the relevant file
with codecs_open('README.rst', encoding='utf-8') as f:
    long_description = f.read()


setup(name='pylox',
      version='0.0.1',
      description=u"A python implementation of lox.",
      long_description=long_description,
      classifiers=[],
      keywords='',
      author=u"Zhaolong Zhu",
      author_email='zhuzhaolong0@gmail.com',
      url='https://github.com/zzl0/craftinginterpreters/python',
      license='MIT',
      packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
      include_package_data=True,
      zip_safe=False,
      install_requires=[],
      extras_require={
          'test': ['pytest'],
      },
      entry_points="""
      [console_scripts]
      pylox=pylox.lox:main
      """
      )
