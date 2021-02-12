# Skeleton of a CLI

import click

import pylox


@click.command('pylox')
@click.argument('count', type=int, metavar='N')
def cli(count):
    """Echo a value `N` number of times"""
    for i in range(count):
        click.echo(pylox.has_legs)
