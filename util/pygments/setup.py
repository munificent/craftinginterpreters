"""
A Pygments lexer for Lox.
"""
from setuptools import setup

__author__ = 'Robert Nystrom'

setup(
    name='Lox',
    version='1.0',
    description=__doc__,
    author=__author__,
    packages=['lox'],
    entry_points='''
    [pygments.lexers]
    loxlexer = lox:LoxLexer
    '''
)