from click.testing import CliRunner

from pylox.scripts.cli import cli


def test_cli_count():
    runner = CliRunner()
    result = runner.invoke(cli, ['3'])
    assert result.exit_code == 0
    assert result.output == "False\nFalse\nFalse\n"
